#pragma once

#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <chrono>
#include <ctime>

// optional ANSI color codes (works in most terminals)
inline constexpr const char* CLR_RESET = "\033[0m";
inline constexpr const char* CLR_INFO  = "\033[1;34m"; // bold blue
inline constexpr const char* CLR_WARN  = "\033[1;33m"; // bold yellow
inline constexpr const char* CLR_ERR   = "\033[1;31m"; // bold red

struct Console {
    std::mutex mtx;

    // basic single-arg (avoid copying if caller has string_view)
    void log(std::string_view string) {
        std::lock_guard lock(mtx);
        std::cout << string << '\n';
    }

    // variadic, prints space-separated values like JS console.log
    template<typename... Args>
    void log(const Args&... args) {
        std::lock_guard lock(mtx);
        (print_one(args), ...);        // fold expression (C++17)
        std::cout << '\n' << std::flush;
    }

    template<typename... Args>
    void info(const Args&... args) {
        std::lock_guard lock(mtx);
        std::cout << CLR_INFO;
        (print_one(args), ...);
        std::cout << CLR_RESET << '\n' << std::flush;
    }

    template<typename... Args>
    void warn(const Args&... args) {
        std::lock_guard lock(mtx);
        std::cout << CLR_WARN;
        (print_one(args), ...);
        std::cout << CLR_RESET << '\n' << std::flush;
    }

    template<typename... Args>
    void error(const Args&... args) {
        std::lock_guard lock(mtx);
        std::cout << CLR_ERR;
        (print_one(args), ...);
        std::cout << CLR_RESET << '\n' << std::flush;
    }

private:
    // helper prints one item followed by a space
    template<typename T>
    void print_one(const T& v) {
        std::cout << v << ' ';
    }
};