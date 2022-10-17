#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <stdexcept>
#include <future>
#include "Windows.h"
#include "src/compfuncs.hpp"
#include "src/trialfuncs.hpp"

using HARD_FAIL = os::lab1::compfuncs::hard_fail;
using SOFT_FAIL = os::lab1::compfuncs::soft_fail;

class Manager {
private:
    int x;
    std::string arg;
    HANDLE pipeF, pipeG;
    std::variant<HARD_FAIL, SOFT_FAIL, int> resF, resG;


    void xInputRequest() {
        std::cout << " >> Enter x: ";
        std::cin >> x;
    }

    void createNamedPipes() {
        pipeF = CreateNamedPipe(
                R"(\\.\pipe\pipe_f)", // name of the pipe
                PIPE_ACCESS_DUPLEX, // 2-way pipe
                PIPE_TYPE_BYTE, // send data as a byte stream
                1, // only allow 1 instance of this pipe
                0, // no outbound buffer
                0, // no inbound buffer
                0, // use default wait time
                nullptr // use default security attributes
        );
        if (pipeF == nullptr || pipeF == INVALID_HANDLE_VALUE) {
            std::cerr << "[M] Failed to create pipeF instance.\n";
            throw std::runtime_error("");
        }

        pipeG = CreateNamedPipe(
                R"(\\.\pipe\pipe_g)", // name of the pipe
                PIPE_ACCESS_DUPLEX, // 2-way pipe
                PIPE_TYPE_BYTE, // send data as a byte stream
                1, // only allow 1 instance of this pipe
                0, // no outbound buffer
                0, // no inbound buffer
                0, // use default wait time
                nullptr // use default security attributes
        );
        if (pipeG == nullptr || pipeG == INVALID_HANDLE_VALUE) {
            std::cerr << "[M] Failed to create pipeG instance.\n";
            throw std::runtime_error("");
        }
    }

    void createProcesses() {
        STARTUPINFO siF, siG;
        PROCESS_INFORMATION piF, piG;

        ZeroMemory(&siF, sizeof(siF));
        siF.cb = sizeof(siF);
        ZeroMemory(&piF, sizeof(piF));
        ZeroMemory(&siG, sizeof(siG));
        siG.cb = sizeof(siG);
        ZeroMemory(&piG, sizeof(piG));

        std::string cmdF = arg + " f " + std::to_string(x);
        char* c_cmdF = const_cast<char*>(cmdF.c_str());
        std::string cmdG = arg + " g " + std::to_string(x);
        char* c_cmdG = const_cast<char*>(cmdG.c_str());

        if (!CreateProcess( nullptr, // No module name (use command line)
                            c_cmdF, // Command line
                            nullptr, // Process handle not inheritable
                            nullptr, // Thread handle not inheritable
                            false, // Set handle inheritance to FALSE
                            0, // No creation flags
                            nullptr, // Use parent's environment block
                            nullptr, // Use parent's starting directory
                            &siF,  // Pointer to STARTUPINFO structure
                            &piF ) // Pointer to PROCESS_INFORMATION structure
                )
        {
            std::cerr << "[M] Failed to create process for f-function computation.\n";
            throw std::runtime_error("");
        }
        if (!CreateProcess( nullptr, // No module name (use command line)
                            c_cmdG, // Command line
                            nullptr, // Process handle not inheritable
                            nullptr, // Thread handle not inheritable
                            false, // Set handle inheritance to FALSE
                            0, // No creation flags
                            nullptr, // Use parent's environment block
                            nullptr, // Use parent's starting directory
                            &siG, // Pointer to STARTUPINFO structure
                            &piG ) // Pointer to PROCESS_INFORMATION structure
                )
        {
            std::cerr << "[M] Failed to create process for g-function computation.\n";
            throw std::runtime_error("");
        }
    }

    static std::variant<HARD_FAIL, SOFT_FAIL, int> readResultFromNamedPipe(char func, HANDLE* pipe, std::mutex* m) {
        BOOL result = ConnectNamedPipe(*pipe, nullptr);
        if (!result) {
            std::cerr << "[M] Failed to make connection on named pipe.\n";
            throw std::runtime_error("");
        }
        std::variant<HARD_FAIL, SOFT_FAIL, int> data;
        void* buffer = static_cast<void*>(&data);
        bool flag = ReadFile(
                *pipe,
                buffer, // the data from the pipe will be put here
                sizeof(data), // number of bytes allocated
                nullptr, // number of bytes actually read
                nullptr // not using overlapped IO
        );
        if (!flag) {
            std::cerr << "[M] Failed to read data from named pipe.\n";
            throw std::runtime_error("");
        }
        auto* res = static_cast<std::variant<os::lab1::compfuncs::hard_fail, os::lab1::compfuncs::soft_fail, int>*>(buffer);
        m->lock();
        std::cout << " -- f(" << func << "): " << *res << "\n";
        m->unlock();
        return *res;
    }

    void receiveResults() {
        std::mutex m;

        auto futureF = std::async(std::launch::async, readResultFromNamedPipe, 'f', &pipeF, &m);
        auto futureG = std::async(std::launch::async, readResultFromNamedPipe, 'g', &pipeG, &m);

        resF = futureF.get();
        resG = futureG.get();
    }

    void closeNamedPipes() {
        CloseHandle(pipeF);
        CloseHandle(pipeG);
    }

public:
    explicit Manager(std::string arg) : arg(std::move(arg)) {
        x = 0;
        pipeF = nullptr;
        pipeG = nullptr;
    }

    void run() {
        xInputRequest();
        createNamedPipes();
        createProcesses();
        receiveResults(); // resF, resG
        closeNamedPipes();
    }
};


#endif //MANAGER_HPP
