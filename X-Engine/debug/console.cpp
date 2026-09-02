#include "debug/console.h"

#include <algorithm>
#include <sstream>

namespace xe {

void Console::Register(const std::string& name, const std::string& help,
                       std::function<void(const std::vector<std::string>&)> handler) {
    ConsoleCommand cmd{ name, help, std::move(handler) };
    for (auto& c : commands_) {
        if (c.name == name) {
            c = cmd;
            return;
        }
    }
    commands_.push_back(std::move(cmd));
}

void Console::Print(const std::string& text) {
    output_.push_back(text);
    constexpr size_t kMaxLines = 200;
    if (output_.size() > kMaxLines) {
        output_.erase(output_.begin(), output_.begin() + (output_.size() - kMaxLines));
    }
}

void Console::PrintLn(const std::string& text) {
    Print(text);
}

void Console::Execute(const std::string& line) {
    if (line.empty()) return;

    // Parse args by whitespace
    std::vector<std::string> args;
    std::istringstream iss(line);
    std::string a;
    while (iss >> a) args.push_back(a);
    if (args.empty()) return;

    const std::string& cmd_name = args[0];
    auto it = std::find_if(commands_.begin(), commands_.end(),
        [&](const ConsoleCommand& c) { return c.name == cmd_name; });

    if (it == commands_.end()) {
        PrintLn("[err] unknown command: " + cmd_name + "  (try 'help')");
        return;
    }

    PrintLn("> " + line);
    history_.push_back(line);
    if (history_.size() > 64) history_.erase(history_.begin());
    history_cursor_ = -1;

    try {
        it->handler(args);
    } catch (const std::exception& e) {
        PrintLn(std::string("[exception] ") + e.what());
    }
}

void Console::AppendChar(char c) {
    input_.push_back(c);
    history_cursor_ = -1;
}

void Console::Backspace() {
    if (!input_.empty()) input_.pop_back();
    history_cursor_ = -1;
}

void Console::HistoryPrev() {
    if (history_.empty()) return;
    if (history_cursor_ < 0) history_cursor_ = static_cast<int>(history_.size()) - 1;
    else if (history_cursor_ > 0) --history_cursor_;
    input_ = history_[history_cursor_];
}

void Console::HistoryNext() {
    if (history_.empty() || history_cursor_ < 0) return;
    if (history_cursor_ + 1 >= static_cast<int>(history_.size())) {
        history_cursor_ = -1;
        input_.clear();
    } else {
        ++history_cursor_;
        input_ = history_[history_cursor_];
    }
}

void Console::SubmitInput() {
    std::string line = input_;
    input_.clear();
    history_cursor_ = -1;
    Execute(line);
}

}  // namespace xe