#include <iostream>
#include "kace/source.hpp"
#include "kace/statement.hpp"
#include "kace/semantic.hpp"
#include "kace/loader.hpp"
#include "kasm/diagnostic.hpp"
#include "kasm/kasmparser.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <map>
#include <algorithm>
#include <kace/source.hpp>
#include <string_view>
#include "kace/symbols.hpp"
#include "kace/fixup.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

std::filesystem::path executableDirectory(const char *argv0)
{
#ifdef _WIN32
    std::wstring path(32768, L'\0');
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (size > 0 && size < path.size())
    {
        path.resize(size);
        return std::filesystem::path(path).parent_path();
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string path(size, '\0');
    if (_NSGetExecutablePath(path.data(), &size) == 0)
        return std::filesystem::canonical(path.c_str()).parent_path();
#elif defined(__linux__)
    std::error_code error;
    const auto path = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error)
        return path.parent_path();
#endif
    return std::filesystem::absolute(argv0).parent_path();
}
struct CommandLine
{
    std::vector<std::string_view> arguments;
    std::vector<std::string_view> options;
    std::string_view input;
    std::string_view output;
    std::string_view cpu = "x86";
    bool disassemble = false;
    std::string error;
};

CommandLine parseCommandLine(int argc, char *argv[])
{
    CommandLine result;
    bool optionsEnded = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        if (!optionsEnded && arg == "--")
        {
            optionsEnded = true;
        }
        else if (!optionsEnded && (arg == "-i" || arg == "--input" ||
                                   arg.substr(0, 8) == "--input="))
        {
            if (!result.input.empty())
            {
                result.error = "input file specified more than once";
                return result;
            }
            if (arg.substr(0, 8) == "--input=")
            {
                result.input = arg.substr(8);
            }
            else if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                result.input = argv[++i];
            }
            if (result.input.empty())
            {
                result.error = "missing input file for " + std::string(arg);
                return result;
            }
        }
        else if (!optionsEnded && (arg == "-d" || arg == "--disassemble"))
        {

            if (result.disassemble)
            {
                result.error = "disassemble option specified more than once";
                return result;
            }
            result.disassemble = true;
        }
        else if (!optionsEnded && (arg == "-o" || arg == "--output" ||
                                   arg.substr(0, 9) == "--output="))
        {
            if (!result.output.empty())
            {
                result.error = "output file specified more than once";
                return result;
            }
            if (arg.substr(0, 9) == "--output=")
            {
                result.output = arg.substr(9);
            }
            else if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                result.output = argv[++i];
            }
            if (result.output.empty())
            {
                result.error = "missing output file for " + std::string(arg);
                return result;
            }
        }
        else if (!optionsEnded && (arg == "-c" || arg == "--cpu" ||
                                   arg.substr(0, 6) == "--cpu="))
        {
            if (arg.substr(0, 6) == "--cpu=")
            {
                std::string cpuStr = std::string(arg.substr(6));
                if (cpuStr.empty())
                {
                    result.error = "missing CPU for " + std::string(arg);
                    return result;
                }
                result.cpu = arg.substr(6);
            }
            else if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                result.cpu = argv[++i];
            }
            else
            {
                result.error = "missing CPU for " + std::string(arg);
                return result;
            }
        }
        else if (!optionsEnded && arg.size() > 1 && arg.front() == '-')
        {
            result.options.push_back(arg);
        }
        else
        {
            result.arguments.push_back(arg);
        }
    }
    return result;
}

void showHelpMessage()
{
    std::cout << "Usage: kasm [options] [command] [arguments]\n"
                 "\nCommands:\n"
                 "  help        Show this help message\n"
                 "\nOptions:\n"
                 "  -h, --help          Show this help message\n"
                 "  -i, --input  FILE   Input file to assemble or disassemble (also --input=FILE)\n"
                 "  -o, --output FILE   Store the result in a file (also --output=FILE)\n"
                 "  -c, --cpu    CPU    Select a CPU plugin by name (default: x86)\n"
                 "  -d, --disassemble   Decode binary to a .dism file (based on -o or -i)\n"
                 "  --          Treat remaining tokens as arguments\n"
                 "\nKASM's command-line workflow is under development.\n";
}

enum class ClientMode
{
    Assemble,
    Disassemble
};

bool loadCpu(const CommandLine &commandLine, const char *argv0, Loader &loader, std::string &error)
{
    const std::string cpuName(commandLine.cpu);
    if (cpuName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos)
    {
        std::cerr << "kasm: invalid CPU plugin name: " << cpuName << "\n";
        return false;
    }
    const auto pluginPath = executableDirectory(argv0) /
                            (std::string(KACE_PLUGIN_PREFIX) + cpuName + KACE_PLUGIN_SUFFIX);
    if (!loader.open(pluginPath.string(), error))
    {
        std::cerr << "kasm: could not load CPU plugin '" << cpuName << "' ("
                  << pluginPath.string() << "): " << error << "\n";
        return false;
    }
    return true;
}

bool expandIncludes(const Source &source, std::vector<Statement> &statements,
                    std::vector<std::filesystem::path> &active, std::string &error)
{
    std::error_code pathError;
    const auto sourcePath = std::filesystem::canonical(source.name, pathError);
    if (pathError)
    {
        error = "kasm: could not resolve source file: " + source.name;
        return false;
    }
    if (std::find(active.begin(), active.end(), sourcePath) != active.end())
    {
        error = "Include cycle detected: " + source.name;
        return false;
    }
    if (active.size() >= 128)
    {
        error = "Include nesting limit exceeded: " + source.name;
        return false;
    }
    active.push_back(sourcePath);
    const std::vector<Token> tokens;
    KasmParser parser(tokens);
    std::vector<Statement> parsed;
    if (!parser.parse(source, parsed, error))
        return false;
    for (Statement &statement : parsed)
    {
        if (statement.kind == StatementKind::Directive &&
            statement.directiveKind == DirectiveKind::Include)
        {
            const auto path = sourcePath.parent_path() / std::filesystem::path(statement.includePath);
            Source included;
            if (!Source::load(path.string(), included))
            {
                error = source.name + ":" + std::to_string(statement.line) +
                        ": Could not open include file: " + path.string();
                return false;
            }
            if (!expandIncludes(included, statements, active, error))
                return false;
        }
        else
            statements.push_back(std::move(statement));
    }
    active.pop_back();
    return true;
}

 
int main(int argc, char *argv[])
{
    const auto commandLine = parseCommandLine(argc, argv);
    ClientMode mode = commandLine.disassemble ? ClientMode::Disassemble : ClientMode::Assemble;

    if (!commandLine.error.empty())
    {
        std::cerr << "kasm: " << commandLine.error << "\n";
        return 1;
    }

    bool showHelp = !commandLine.arguments.empty() && commandLine.arguments.front() == "help";
    for (const auto option : commandLine.options)
    {
        if (option == "-h" || option == "--help")
        {
            showHelp = true;
        }
    }
    if (showHelp)
    {
        showHelpMessage();
        return 0;
    }

    if (mode == ClientMode::Disassemble)
    {
        if (commandLine.input.empty())
        {
            std::cerr << "kasm: an input file is required for disassembly\n";
            return 1;
        }
        const std::filesystem::path inputPath{std::string(commandLine.input)};
        std::filesystem::path outputPath{std::string(commandLine.output.empty() ? commandLine.input : commandLine.output)};
        outputPath.replace_extension(".dism");
        std::error_code pathError;
        if (std::filesystem::equivalent(inputPath, outputPath, pathError))
        {
            std::cerr << "kasm: disassembly output must differ from the input file\n";
            return 1;
        }
        std::ifstream input(inputPath, std::ios::binary);
        if (!input)
        {
            std::cerr << "kasm: could not open input file: " << inputPath.string() << "\n";
            return 1;
        }
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        if (input.bad())
        {
            std::cerr << "kasm: could not read input file: " << inputPath.string() << "\n";
            return 1;
        }
        Loader loader;
        std::string error;
        if (!loadCpu(commandLine, argv[0], loader, error))
            return 1;

        auto symbolsPath = inputPath;
        symbolsPath.replace_extension(".sym");
        SymbolTable symbols;
        std::error_code symbolsError;
        const bool hasSymbols = std::filesystem::exists(symbolsPath, symbolsError);
        if (symbolsError || (hasSymbols && !symbols.load_from_file(symbolsPath.string())))
        {
            std::cerr << "kasm: could not load symbol file: " << symbolsPath.string() << "\n";
            return 1;
        }
        if (hasSymbols && std::filesystem::equivalent(symbolsPath, outputPath, pathError))
        {
            std::cerr << "kasm: disassembly output must differ from the symbol file\n";
            return 1;
        }
        std::map<std::size_t, std::vector<std::string>> labels;
        for (const Symbol &symbol : symbols.getSymbolsByKind(SymbolKind::Label))
            labels[symbol.offset].push_back(symbol.name);
        for (auto &entry : labels)
            std::sort(entry.second.begin(), entry.second.end());

        struct DecodedInstruction
        {
            std::size_t offset;
            std::size_t end;
            std::string text;
        };
        std::vector<DecodedInstruction> instructions;
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            const std::size_t start = offset;
            std::string text;
            if (!loader.cpu()->decode(bytes, offset, text) || offset <= start || offset > bytes.size())
            {
                std::cerr << "kasm: could not decode instruction at byte offset " << start << "\n";
                return 1;
            }
            instructions.push_back({start, offset, text});
        }
        for (const auto &label : labels)
        {
            if (label.first != bytes.size() && std::none_of(instructions.begin(), instructions.end(),
                                                            [&](const DecodedInstruction &instruction)
                                                            { return instruction.offset == label.first; }))
            {
                std::cerr << "kasm: symbol offset is not an instruction boundary: " << label.first << "\n";
                return 1;
            }
        }
        std::ostringstream decoded;
        for (const auto &instruction : instructions)
        {
            const auto definitions = labels.find(instruction.offset);
            if (definitions != labels.end())
                for (const auto &name : definitions->second)
                    decoded << name << ":\n";
            std::int64_t displacement = 0;
            std::string text = instruction.text;
            const char *jumpType = nullptr;
            if (loader.cpu()->short_jump_displacement(bytes, instruction.offset, displacement))
                jumpType = "short";
            else if (loader.cpu()->near_jump_displacement(bytes, instruction.offset, displacement))
                jumpType = "near";
            if (jumpType != nullptr)
            {
                std::size_t target = instruction.end;
                bool valid = true;
                if (displacement < 0)
                {
                    const auto magnitude = static_cast<std::uint64_t>(-(displacement + 1)) + 1;
                    if (magnitude > target)
                        valid = false;
                    else
                        target -= static_cast<std::size_t>(magnitude);
                }
                else if (static_cast<std::uint64_t>(displacement) > bytes.size() - target)
                    valid = false;
                else
                    target += static_cast<std::size_t>(displacement);
                const auto found = valid ? labels.find(target) : labels.end();
                if (found != labels.end())
                    text = std::string("jmp ") + jumpType + " " + found->second.front();
            }

            decoded << text << '\n';
        }

        const auto endLabels = labels.find(bytes.size());
        if (endLabels != labels.end())
            for (const auto &name : endLabels->second)
                decoded << name << ":\n";
        std::ofstream output(outputPath, std::ios::binary);
        if (output)
            output << decoded.str();
        output.close();
        if (!output)
        {
            std::cerr << "kasm: could not write output file: " << outputPath.string() << "\n";
            return 1;
        }
        std::cout << "wrote " << outputPath.string() << "\n";
        return 0;
    }

    std::vector<Statement> statements;
    std::string error;
    if (!commandLine.output.empty() && commandLine.input.empty())
    {
        std::cerr << "kasm: an input file is required for output\n";
        return 1;
    }
    if (!commandLine.input.empty())
    {
        Source source;
        if (!Source::load(std::string(commandLine.input), source))
        {
            std::cerr << "kasm: could not open input file: " << commandLine.input << "\n";
            return 1;
        }
        std::vector<std::filesystem::path> active;
        if (!expandIncludes(source, statements, active, error))
        {
            std::cerr << error << "\n";
            return 1;
        }
        std::cout << "parsed " << statements.size() << " statements\n";
    }
    if (!commandLine.output.empty())
    {
        Loader loader;
        if (!loadCpu(commandLine, argv[0], loader, error))
            return 1;

        SymbolTable symbolTable;
        std::vector<std::uint8_t> bytes;
        std::vector<JumpFixup> jumpFixups;
        // Pass 1 - Encode statements and collect jump fixups
        std::cout << "Encoding statements and collecting jump fixups...\n";
        for (Statement &statement : statements)
        {
            statement.offset = bytes.size();
            if (
                statement.kind == StatementKind::Directive && 
                statement.directiveStage == DirectiveStage::Assembly &&
                statement.directiveKind == DirectiveKind::AssertPosition
            )
            {
                if(statement.offset != statement.assertPosition)
                {
                    std::cerr << statement.sourceName << ":" << statement.line << ": Assert position failed: expected " << statement.assertPosition << ", got " << statement.offset << "\n";
                    return 1;
                }
                continue;
            }
            if (statement.kind == StatementKind::Label)
            {
                if (!symbolTable.addLabel(statement.name, statement.offset))
                {
                    std::cerr << statement.sourceName << ":" << statement.line << ": Duplicate label: " << statement.name << "\n";
                    return 1;
                }
                continue;
            }
            if (statement.kind == StatementKind::JumpShort)
            {
                jumpFixups.push_back({statement.target, statement.offset, statement.line, JumpKind::Short, statement.sourceName});
                bytes.push_back(0); // Placeholder for the jump instruction byte
                bytes.push_back(0);
                continue;
            }
            if (statement.kind == StatementKind::JumpNear)
            {
                jumpFixups.push_back({statement.target, statement.offset, statement.line, JumpKind::Near, statement.sourceName});
                bytes.push_back(0); // Placeholder for the jump instruction byte
                bytes.push_back(0);
                bytes.push_back(0); // Extra byte for the near jump offset
                continue;
            }
            if (statement.kind == StatementKind::JumpFar)
            {
                std::vector<std::uint8_t> encoded;
                if (!loader.cpu()->encode_far_jump(statement.farSegment, statement.farOffset, encoded, error))
                {
                    std::cerr << statement.sourceName << ":" << statement.line << ": " << error << "\n";
                    return 1;
                }
                bytes.insert(bytes.end(), encoded.begin(), encoded.end());
                continue;
            }

            // Times directive handling
            std::uint8_t byte = 0;

            if (statement.kind == StatementKind::Times)
            {
                if (!check_byte(statement, byte, error))
                {
                    std::cerr << statement.sourceName << ":" << statement.line
                              << ": " << error << "\n";
                    return 1;
                }

                Parser evaluator(std::vector<Token>{});
                PositionContext positions{statement.offset, 0};

                if (!evaluator.evaluate(
                        statement.countExpression,
                        statement.countExpression.root,
                        statement.count,
                        error,
                        &positions))
                {
                    std::cerr << statement.sourceName << ":" << statement.line
                              << ": " << error << "\n";
                    return 1;
                }

                if (statement.count > 1000000)
                {
                    std::cerr << statement.sourceName << ":" << statement.line
                              << ": Repeat count exceeds the limit\n";
                    return 1;
                }

                bytes.insert(bytes.end(), statement.count, byte);
                continue;
            }

            if (!encode_statement(statement, *loader.cpu(), byte, error))
            {
                std::cerr << statement.sourceName << ":" << statement.line << ": " << error << "\n";
                return 1;
            }
            bytes.push_back(byte);
        }
        // Pass 2 -
        std::cout << "Running pass 2...\n";
        for (const JumpFixup &fixup : jumpFixups)
        {
            const Symbol *label = symbolTable.getSymbol(fixup.target);
            if (label == nullptr)
            {
                std::cerr << fixup.sourceName << ":" << fixup.line
                          << ": Unknown label: " << fixup.target << "\n";
                return 1;
            }
            const std::size_t jumpSize = fixup.kind == JumpKind::Near ? 3 : 2;
            const std::int64_t displacement = static_cast<std::int64_t>(label->offset) - static_cast<std::int64_t>(fixup.offset) - static_cast<std::int64_t>(jumpSize);
            std::vector<std::uint8_t> encoded;
            bool success = false;

            if (fixup.kind == JumpKind::Short)
            {
                success = loader.cpu()->encode_short_jump(displacement, encoded, error);
            }
            else if (fixup.kind == JumpKind::Near)
            {
                success = loader.cpu()->encode_near_jump(displacement, encoded, error);
            }
            else if (fixup.kind == JumpKind::Far)
            {
                success = false; // Far jump encoding not implemented yet
            }

            if (!success || encoded.size() != jumpSize)
            {
                std::cerr << fixup.sourceName << ":" << fixup.line
                          << ": " << (error.empty() ? "Invalid jump encoding" : error)
                          << "\n";
                return 1;
            }

            for (std::size_t i = 0; i < encoded.size(); ++i)
            {
                bytes[fixup.offset + i] = encoded[i];
            }
        }
        const std::filesystem::path binaryPath{std::string(commandLine.output)};
        auto symbolsPath = binaryPath;
        symbolsPath.replace_extension(".sym");
        const auto sourcePath = std::filesystem::absolute(std::filesystem::path(std::string(commandLine.input))).lexically_normal();
        if (std::filesystem::absolute(binaryPath).lexically_normal() == std::filesystem::absolute(symbolsPath).lexically_normal() ||
            std::filesystem::absolute(symbolsPath).lexically_normal() == sourcePath)
        {
            std::cerr << "kasm: symbol file must differ from the binary and input file\n";
            return 1;
        }
        std::ofstream outFile(std::string(commandLine.output), std::ios::binary);
        if (!outFile)
        {
            std::cerr << "kasm: could not open output file: " << commandLine.output << "\n";
            return 1;
        }
        if (!symbolTable.save_to_file(symbolsPath.string()))
        {
            std::cerr << "kasm: could not write symbol file: " << symbolsPath.string() << "\n";
            return 1;
        }
        outFile.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        outFile.close();
        if (!outFile)
        {
            std::cerr << "kasm: could not write output file: " << commandLine.output << "\n";
            return 1;
        }
    }
    return 0;
}
