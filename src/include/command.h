/*****************************************************************//**
 * \file   command.h
 * \brief  Defines commands available in the CLI.
 * 
 * \author Mikalai Varapai
 * \date   November 2024
 *********************************************************************/

#include <string>
#include <iostream>
#include <array>
#include <vector>

class JSONInterface;
class CommandInterface;
class CommandLineInterpreter;

void ProcessInput(std::string, JSONInterface&, CommandInterface&);

template <int N>
class ConsoleTable
{
    const int TabSize = 8;

    const std::array<int, N> columnWidths;
    const int tabOffset;

public:
    ConsoleTable(std::array<int, N> columnWidths, int tabOffset) 
        : columnWidths(columnWidths), tabOffset(tabOffset) { }

    void PrintLine(std::array<std::string, N> elements)
    {

        size_t maxNumLines = 0;
        int i = 0;
        for (std::string s : elements)
        {
            int columnSize = columnWidths[i] * TabSize - 1;
            int numLines = (int)s.size() / columnSize;
            if (s.size() % columnSize > 0) numLines++;

            if (numLines > maxNumLines) maxNumLines = numLines;
            i++;
        }

        std::vector<std::string> lines(maxNumLines);

        // Iterate through columns
        for (int i = 0; i < N; i++)
        {
            std::string currentString = elements[i];
            int columnSize = columnWidths[i] * TabSize - 1;

            // Get the amout of lines for the string
            int numLines = (int)currentString.size() / columnSize;
            if ((int)currentString.size() % columnSize > 0) numLines++;

            size_t pos = 0;

            // Loop with whole lines
            int j;
            for (j = 0; j < numLines - 1; j++)
            {
                lines[j] += currentString.substr(pos, columnSize);
                lines[j] += '\t';
                pos += columnSize;
            }

            // Process the last line
            std::string lastElement = currentString.substr(pos, columnSize);
            lines[j] += lastElement;

            // Get and append the amount of tabs
            size_t tabAmount = (int)lastElement.size() / TabSize;
            tabAmount = columnWidths[i] - tabAmount;
            lines[j] += std::string(tabAmount, '\t');

            // If there are other lines, fill them with tabs
            j++;
            for (j; j < maxNumLines; j++)
            {
                lines[j] += std::string(columnWidths[i], '\t');
            }
        }

        // Add offset and print the lines

        for (std::string line : lines)
        {
            std::cout << std::string(tabOffset, '\t') << line << std::endl;
        }
    }

    void PrintSeparator(const char c)
    {
        size_t length = 0;
        for (int n : columnWidths) length += n;

        length *= TabSize;
        std::string separator(length, c);

        size_t offset = tabOffset;
        std::string offsetStr = std::string(offset, '\t');

        std::cout << offsetStr + separator << std::endl;
    }
};

// Class to represent possible commands.
// Supposed to be extensible.
class Command
{
protected:
    friend class CommandInterface;
    friend class CommandHelp;

    // Only specified at object level.
    const std::string name;
    const std::string alias;

    // For correct :help output
    const std::string cmdHelpSyntax;
    const std::string cmdHelpDesc;

public:
    // Virtual function for commands to implement.
    virtual void Execute(const CommandLineInterpreter& interpreter) const = 0;

    Command(const std::string name, const std::string alias,
        const std::string syntax, const std::string desc) 
        : name(name), alias(alias), cmdHelpSyntax(syntax), cmdHelpDesc(desc) { }
};

class CommandInterface
{
    friend class CommandHelp;
    std::vector<Command*> commands;

public:
    CommandInterface() = default;

    // Return a pointer to const instance of Command
    // if search was successful, nullptr otherwise.
    // Forbids modifying data at returned location.
    Command const* FindCommand(std::string cmdName) const
    {
        for (const Command* c : commands) 
            if (cmdName == c->name || cmdName == c->alias) return c;
        return nullptr;
    }

    // Push command in the argument to the list of commands.
    void RegisterCommand(Command* cmd)
    {
        if (FindCommand(cmd->name))
        {
            std::cout << "Tried to register already defined command \"" 
                << cmd->name << "\"." << std::endl;
            return;
        }
        commands.push_back(cmd);
    }

    ~CommandInterface()
    {
        for (Command* c : commands)
        {
            delete c;
        }
    }
};

class CommandQuit : public Command
{
public:
    CommandQuit() : Command("quit", "q", ":quit", "Exit the CLI.") { }

    void Execute(const CommandLineInterpreter& args) const override
    {
        std::cout << "Exiting.." << std::endl;
        exit(0);
    }
};

class CommandHelp : public Command
{
    CommandInterface& cmdInterface;

public:
    CommandHelp(CommandInterface& interface) 
        : Command("help", "h", ":help", "Display the list of commands"),
            cmdInterface(interface) { }

    void Execute(const CommandLineInterpreter& args) const override
    {
        ConsoleTable<2> table({ 7, 9 }, 0);

        table.PrintLine({ "List of commands:", "" });
        std::cout << std::endl;

        for (const Command* c : cmdInterface.commands)
        {
            table.PrintLine({ c->cmdHelpSyntax, c->cmdHelpDesc });
        }

        std::cout << std::endl;
    }
};

class CommandCurrent : public Command
{
    JSONInterface& json;
public:
    CommandCurrent(JSONInterface& json) : json(json),
        Command("current", "c", 
        ":current (--recursive=MAX_DEPTH) (--show-values)", 
        "Displays info about current object.") { }

    void Execute(const CommandLineInterpreter& interpreter) const override;
};

class CommandSelect : public Command
{
    JSONInterface& json;
public:
    CommandSelect(JSONInterface& json) : json(json),
        Command("select", "s", ":select <EXPR>",
        "Select object member. Must also be an object.") { }

    void Execute(const CommandLineInterpreter& interpreter) const override;
};

class CommandBack : public Command
{
    JSONInterface& json;

public:
    CommandBack(JSONInterface& json) : json(json),
        Command("back", "b", ":back (<NUM_STEPS>) (--root)", "Move up the hierarchy.") { }

    void Execute(const CommandLineInterpreter& interpreter) const override;
};
