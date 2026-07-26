#include "addons.hpp"
#include "callbacks.hpp"
#include "cblks_def.hpp"
#include "../server.h"

// Maybe include asmjit to this. We need dynamic generation.

#include <asmjit/core.h>
#include <asmjit/x86.h>
#include <asmjit/host.h>
#include <cstdint>
#include <sys/mman.h>

using namespace asmjit;
static x86::Assembler *a = nullptr;

JitRuntime rt;

// Should be a array which their id are defined in a file.
uint64_t *AddonsCallbacks = nullptr;

void servAddons::preCompileCallbacks() {
    AddonsCallbacks = (uint64_t*)mmap(nullptr, 32*8, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

void _Arguments(bool way) {
    if (way) {
        // Expose them
        a->mov(x86::rdi, x86::r12);
        a->mov(x86::rsi, x86::r13);
        a->mov(x86::rdx, x86::r14);
        a->mov(x86::rcx, x86::r15);
        a->mov(x86::r8,  x86::rbx);
        a->mov(x86::r9, x86::qword_ptr(x86::rbp, -8));
        a->jmp(x86::r10);
    } else {
        a->mov(x86::qword_ptr(x86::rbp, -8), x86::r9);
        a->mov(x86::r12, x86::rdi);
        a->mov(x86::r13, x86::rsi);
        a->mov(x86::r14, x86::rdx);
        a->mov(x86::r15, x86::rcx);
        a->mov(x86::rbx, x86::r8);
    }
}

void servAddons::setReady() {
    preCompileCallbacks();
    uint64_t _COUNTER = 0;
    std::vector<uint64_t> *callbacks = nullptr;
    infostream << FUNCTION_NAME << ": total callbacks groups: " <<CBregistered.size() << std::endl;
    while (true) {
        bool _showGeneratedCode = false;
        try {
            callbacks = &CBregistered.at(_COUNTER);
        } catch (std::out_of_range &a) {
            break;
        }
        CodeHolder code;
        code.init(rt.environment());
        x86::Assembler asm_(&code);
        StringLogger qlog0;
        code.set_logger(&qlog0);
        a = &asm_;
        /// GENERATION ///
        a->push(x86::rbp);
        a->mov(x86::rbp, x86::rsp);
        a->push(x86::r12);
        a->push(x86::r13);
        a->push(x86::r14);
        a->push(x86::r15);
        a->push(x86::rbx);
        a->sub(x86::rsp, 24); // 16 = 8,8
        //Label _Q = a->new_label();
        Label _SQ = a->new_label();
        _Arguments(false);
        a->jmp(_SQ);
        Label _restore = a->new_label();
        a->bind(_restore);
        _Arguments(true);
        Label _EXIT = a->new_label();
        a->bind(_SQ);
        a->mov(x86::rcx, &AddonsCallbackStatus);
        a->mov(x86::rcx, x86::qword_ptr(x86::rcx));
        a->test(x86::rcx, x86::rcx);
        a->jz(_EXIT);        
        uint64_t _ = 0;
        while (true) {
            uint64_t PTR;
            try {
                PTR = callbacks->at(_);
            } catch (std::out_of_range &z) {
                break;
            }
            _showGeneratedCode = true;
            if (_showGeneratedCode) {
                qlog0._log2("\nGoing to exec func<Start>: ");
                qlog0._log2(std::to_string(PTR).c_str());
                qlog0._log2("\n");
            }
                
            a->lea(x86::r10, x86::qword_ptr(x86::rip));
            a->add(x86::r10, 0x6);
            a->jmp(_restore);
            if (!PTR) {
                errorstream << FUNCTION_NAME << ": No defined function found! ID=" << _COUNTER+1 << std::endl;
                a->mov(x86::rax, 0);
            }
            a->call(PTR);   
            qlog0._log2("Going to exec func < END >\n");
            _++;
        }
        a->bind(_EXIT);
        // After finished, compile that function and then save to their unique address
        a->pop(x86::r12);
        a->pop(x86::r13);
        a->pop(x86::r14);
        a->pop(x86::r15);
        a->pop(x86::rbx);
        a->leave();
        a->ret();
        a->finalize();
        void *toalloc = nullptr;
        Error ERR = rt.add(&toalloc, &code);
        if (ERR != Error::kOk) {
            errorstream << FUNCTION_NAME << ": Couldnt generate a function for main callbacks!" << std::endl;
            return;
        }
        // If show generated code..
        if (_showGeneratedCode)
            std::cout << "\033[1;33mAssembly Code:\033[0m \n" << qlog0.data() << "" << std::endl;
        // Save!
        AddonsCallbacks[_COUNTER] = (uint64_t)toalloc;
        _COUNTER++;
    }
}


