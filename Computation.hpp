#ifndef COMPUTATION_HPP
#define COMPUTATION_HPP

using HARD_FAIL = os::lab1::compfuncs::hard_fail;
using SOFT_FAIL = os::lab1::compfuncs::soft_fail;

class Computation {
private:
    char func;
    int x;
    HANDLE pipe;
    std::variant<HARD_FAIL, SOFT_FAIL, int> result;

    void computeResult() {
        result = os::lab1::compfuncs::trial_f<os::lab1::compfuncs::INT_SUM>(x);
    }

    void connectToNamedPipe() {
        std::string pipeName = std::string(R"(\\.\pipe\pipe_)") + func;
        char* c_pipeName = const_cast<char*>(pipeName.c_str());

        pipe = CreateFile(
                c_pipeName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
        );
        if (pipe == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to connect to pipe.");
        }
    }

    void sendResultToNamedPipe() {
        void* resultVoid = static_cast<void*>(&result);
        bool flag = WriteFile(
                pipe, // handle to our outbound pipe
                resultVoid, // data to send
                sizeof(result), // size of data to send (bytes)
                nullptr, // actual amount of data sent
                nullptr // not using overlapped IO
        );
        if (!flag) {
            throw std::runtime_error("Failed to send data.");
        }
    }

    void closeNamedPipe() {
        CloseHandle(pipe);
    }
public:
    explicit Computation(char func, int x) : func(func), x(x) {
        if (func != 'f' && func != 'g')
            throw std::runtime_error("Unexpected computation function.");
        pipe = nullptr;
    }

    void run() {
        computeResult();
        connectToNamedPipe();
        sendResultToNamedPipe();
        closeNamedPipe();
    }
};


#endif //COMPUTATION_HPP
