#pragma once

#include <functional>
#include <string>
#include <vector>

namespace xe {

struct ConsoleCommand {
    std::string name;
    std::string help;
    std::function<void(const std::vector<std::string>&)> handler;
};

class Console {
public:
    Console() = default;

    void Register(const std::string& name, const std::string& help,
                  std::function<void(const std::vector<std::string>&)> handler);

    // Submit a line of input (e.g. "fps 1.5"). Splits by whitespace, finds
    // command, invokes handler. Output goes to the output buffer.
    void Execute(const std::string& line);

    // Append raw output text (no newline added).
    void Print(const std::string& text);

    void PrintLn(const std::string& text);

    // Open / close toggle
    void SetOpen(bool open) { open_ = open; }
    bool IsOpen() const { return open_; }
    void Toggle() { open_ = !open_; }

    void ClearOutput() { output_.clear(); }

    // Active text buffer (input field)
    std::string& InputLine() { return input_; }
    const std::string& InputLine() const { return input_; }

    void AppendChar(char c);
    void Backspace();
    void HistoryPrev();
    void HistoryNext();
    void SubmitInput();

    const std::vector<std::string>& OutputLines() const { return output_; }
    const std::vector<std::string>& History() const { return history_; }
    const std::vector<ConsoleCommand>& Commands() const { return commands_; }

private:
    std::vector<ConsoleCommand> commands_;
    std::vector<std::string> output_;
    std::vector<std::string> history_;
    std::string input_;
    int history_cursor_ = -1;  // -1 = not navigating
    bool open_ = false;
};

}  // namespace xe