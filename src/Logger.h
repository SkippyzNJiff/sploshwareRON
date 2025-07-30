#pragma once
#include <iostream>
#include <streambuf>

// Simple runtime toggle for std::cout spam.
// Call EnableDebugLog(false) once the project has finished initialising to silence most output.
// Press F9 (added in main.cpp) to toggle while the cheat is running.

namespace Logger {

    class NullBuffer : public std::streambuf {
    protected:
        int overflow(int c) override { return c; }
    };

    inline NullBuffer g_nullBuffer{};
    inline std::ostream g_nullStream{ &g_nullBuffer };

    inline std::streambuf* g_savedBuf = nullptr;
    inline bool g_enabled = true;

    inline void EnableDebugLog(bool enable) {
        if (enable == g_enabled) return;

        if (enable) {
            // Restore original buffer
            if (g_savedBuf) {
                std::cout.rdbuf(g_savedBuf);
            }
            g_enabled = true;
        }
        else {
            // Redirect cout to /dev/null-like sink
            if (!g_savedBuf) {
                g_savedBuf = std::cout.rdbuf();
            }
            std::cout.rdbuf(g_nullStream.rdbuf());
            g_enabled = false;
        }
    }
} 