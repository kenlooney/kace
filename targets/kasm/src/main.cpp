#include <iostream>
#include "kace/source.hpp"
#include "kace/statement.hpp"
#include "kace/semantic.hpp"
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

struct CommandLine
{
    std::vector<std::string_view> arguments;
    std::vector<std::string_view> options;
    std::string_view input;
    std::string_view output;
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
                 "  -h, --help   Show this help message\n"
                 "  -i, --input  FILE  Parse an input file (also --input=FILE)\n"
                 "  -o, --output FILE  Store the result in a file (also --output=FILE)\n"
                 "  --          Treat remaining tokens as arguments\n"
                 "\nKASM's command-line workflow is under development.\n";
}
int main(int argc, char *argv[])
{
    const auto commandLine = parseCommandLine(argc, argv);

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
        std::vector<std::uint8_t> bytes;
        for (const Statement &statement : statements)
        {
            std::uint8_t byte = 0;
            if (!check_byte(statement, byte, error))
            {
                std::cerr << error << "\n";
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
