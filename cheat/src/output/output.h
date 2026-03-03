#pragma once
#include <print>
#include <array>
#include <ctime>
#include <memory>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#endif

#include <mutex>

enum class e_level
{
    INFO,
    WARN,
    ERORR,
    DEBUG
};

class c_logger final
{
private:
    std::mutex mtx;
public:
    c_logger() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }

        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        if (hInput != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            GetConsoleMode(hInput, &mode);
            SetConsoleMode(hInput, mode);
        }

        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hOut, &cursorInfo);
#endif
    }

    template <e_level level, class... types>
    inline void log(const std::format_string<types...> format, types&&... args)
    {
        std::lock_guard<std::mutex> lock(mtx);
        const char* color;
        const char* label;

        if constexpr (level == e_level::INFO) color = "\x1b[92m", label = "INFO";
        else if constexpr (level == e_level::WARN) color = "\x1b[93m", label = "WARN";
        else if constexpr (level == e_level::ERORR)  color = "\x1b[31m", label = "ERROR";
        else if constexpr (level == e_level::DEBUG) color = "\x1b[38;2;204;115;146m", label = "DEBUG";

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &t);

        std::array<char, 11> date_buf;
        std::strftime(date_buf.data(), date_buf.size(), "%m-%d-%Y", &tm);

        std::array<char, 9> time_buf;
        std::strftime(time_buf.data(), time_buf.size(), "%H:%M:%S", &tm);

        std::print("[\033[0m{}{} {}\033[0m][\033[0m{}{}\033[0m] ",
            color, date_buf.data(), time_buf.data(), color, label);

        std::println(format, std::forward<types>(args)...);
    }
};

inline std::unique_ptr<c_logger> logger = std::make_unique<c_logger>();

