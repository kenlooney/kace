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
#include <kace/source.hpp>
#include <string_view>
#include "kace/symbols.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

std::filesystem::path executableDirectory(const char* argv0)
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
    if (!error) return path.parent_path();
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
                 "  -d, --disassemble   Disassemble the input file\n"
                 "  --          Treat remaining tokens as arguments\n"
                 "\nKASM's command-line workflow is under development.\n";
}

enum class ClientMode
{
    Assemble,
    Disassemble
};

void disassemble(const std::string &file)
{
    std::cout << "Disassembling file: " << file << "\n";
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
        if (mode == ClientMode::Disassemble)
        {
            // Disassemble input file
            disassemble(std::string(commandLine.input));
            return 0; // Exit after disassembling
        }
        const std::vector<Token> tokens;
        KasmParser parser(tokens);
        if (!parser.parse(source, statements, error))
        {
            std::cerr << error << "\n";
            return 1;
        }
        std::cout << "parsed " << statements.size() << " statements\n";
    }
    if (!commandLine.output.empty())
    {
        const std::string cpuName(commandLine.cpu);
        if (cpuName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos)
        {
            std::cerr << "kasm: invalid CPU plugin name: " << cpuName << "\n";
            return 1;
        }
        const auto pluginPath = executableDirectory(argv[0]) /
            (std::string(KACE_PLUGIN_PREFIX) + cpuName + KACE_PLUGIN_SUFFIX);
        Loader loader;
        if (!loader.open(pluginPath.string(), error))
        {
            std::cerr << "kasm: could not load CPU plugin '" << cpuName << "' ("
                      << pluginPath.string() << "): " << error << "\n";
            return 1;
        }
        
        SymbolTable symbolTable;
        std::vector<std::uint8_t> bytes;
        for (Statement &statement : statements)
        {
            statement.offset = bytes.size();
            
            if(statement.kind == StatementKind::Label) {
                symbolTable.addLabel(statement.name, statement.offset);
                continue;
            }
          
            std::uint8_t byte = 0;
            if (!encode_statement(statement, *loader.cpu(), byte, error))
            {
                std::cerr << commandLine.input << ":" << statement.line << ": " << error << "\n";
                return 1;
            }
            bytes.push_back(byte);
        }
        std::ofstream outFile(std::string(commandLine.output), std::ios::binary);
        if (!outFile)
        {
            std::cerr << "kasm: could not open output file: " << commandLine.output << "\n";
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
