/*
 * SSH Mounter - Simple GUI for SSHFS mounts
 * Copyright (C) 2025 SonicandTailsCD
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */

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