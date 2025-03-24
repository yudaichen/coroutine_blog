//
// Created by ydc on 2025/2/18.
//

#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H
#include <iostream>
#include <string>
#include <ctime>
#include <memory>
#include <vector>
#include <cstdlib>


inline std::string getCurrentTimeString() {
    const std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&now));
    return {buf};
}

class CrashHandlerBase {
public:
    virtual void setup() = 0;
    virtual ~CrashHandlerBase() = default;
};

// Windows
#ifdef _WIN32
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

class WindowsCrashHandler : public CrashHandlerBase {
public:
    void setup() override {
        SetUnhandledExceptionFilter(UnhandledExceptionFilterEx);
    }

private:
    static LONG WINAPI UnhandledExceptionFilterEx(PEXCEPTION_POINTERS ExceptionInfo) {
        std::string dumpFileName = "crash_dump_" + getCurrentTimeString() + ".dmp";
        HANDLE hFile = CreateFileA(dumpFileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create dump file: " << dumpFileName << std::endl;
            return EXCEPTION_EXECUTE_HANDLER;
        }

        MINIDUMP_EXCEPTION_INFORMATION ExInfo;
        ExInfo.ThreadId = GetCurrentThreadId();
        ExInfo.ExceptionPointers = ExceptionInfo;
        ExInfo.ClientPointers = FALSE;

        if (!MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, nullptr, nullptr)) {
            std::cerr << "Failed to write dump file: " << dumpFileName << std::endl;
        }

        CloseHandle(hFile);
        std::cerr << "Crash dump file created: " << dumpFileName << std::endl;
        return EXCEPTION_EXECUTE_HANDLER;
    }
};

#else
#include <csignal>
#include <execinfo.h>
#include <fstream>

class LinuxCrashHandler : public CrashHandlerBase {
public:
    void setup() override {
        struct sigaction sa;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        // ´¦Àí¶à¸öÐÅºÅ
        std::vector<int> signals = {SIGSEGV, SIGABRT, SIGFPE};
        for (int sig : signals) {
            sigaction(sig, &sa, nullptr);
        }
    }

private:
    static void signalHandler(int signum) {
        void *array[10];

        const size_t size = backtrace(array, 10);
        char** messages = backtrace_symbols(array, size);

        std::string dumpFileName = "crash_dump_" + getCurrentTimeString() + ".txt";
        std::ofstream dumpFile(dumpFileName);

        if (dumpFile.is_open()) {
            dumpFile << "Program Version: 1.0" << std::endl;


            extern char** environ;
            dumpFile << "Command Line Args:" << std::endl;
            for (int i = 0; environ[i] != nullptr; ++i) {
                dumpFile << environ[i] << std::endl;
            }


            dumpFile << "Environment Variables:" << std::endl;
            for (char** env = environ; *env != nullptr; ++env) {
                dumpFile << *env << std::endl;
            }

            dumpFile << "Signal " << signum << " received. Stack trace:" << std::endl;
            for (size_t i = 0; i < size; ++i) {
                dumpFile << messages[i] << std::endl;
            }
            dumpFile.close();
            std::cerr << "Crash dump file created: " << dumpFileName << std::endl;
        } else {
            std::cerr << "Failed to open dump file: " << dumpFileName << std::endl;
        }

        free(messages);
        exit(signum);
    }
};
#endif

class CrashHandler {
public:
    CrashHandler() {
#ifdef _WIN32

        SetConsoleOutputCP(65001);

        handler = std::make_unique<WindowsCrashHandler>();
#else
        handler = std::make_unique<LinuxCrashHandler>();
#endif
        handler->setup();
    }

private:
    std::unique_ptr<CrashHandlerBase> handler;
};

#endif //CRASH_HANDLER_H
