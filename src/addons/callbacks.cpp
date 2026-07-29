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

void _Arguments(uint32_t count, bool way) {
    if (way) {
        // Expose them
        if (count)
            a->mov(x86::rdi, x86::r12);
        if (count >= 2)
            a->mov(x86::rsi, x86::r13);
        if (count >= 3)
            a->mov(x86::rdx, x86::r14);
        if (count >= 4)
            a->mov(x86::rcx, x86::r15);
        if (count >= 5)
            a->mov(x86::r8,  x86::rbx);
        if (count == 6)
            a->mov(x86::r9, x86::qword_ptr(x86::rbp, -8));
        a->jmp(x86::r10);
    } else {
        if (count)
            a->mov(x86::r12, x86::rdi);
        if (count >= 2)
            a->mov(x86::r13, x86::rsi);
        if (count >= 3)
            a->mov(x86::r14, x86::rdx);
        if (count >= 4)
            a->mov(x86::r15, x86::rcx);
        if (count >= 5)
            a->mov(x86::rbx, x86::r8);
        if (count == 6)
            a->mov(x86::qword_ptr(x86::rbp, -8), x86::r9);
    }
}


void _testThing(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f) {
    asm volatile ( "ud2" );
}

void servAddons::setReady() {
    // Arguments count.
    std::vector<uint32_t> _argCount({
        0, // void, // ON_SHUTDOWN
        1, // PlayerSAO*, // JOINPLAYER
        1, // PlayerSAO* // NEWPLAYER
        2, // PlayerSAO*, int (ClientDeletionReason) // LEAVEPLAYER
        2, // PlayerSAO*, PlayerHPChangeReason* // DIEPLAYER
        2, // PlayerSAO*, const char* // PLAYEREVENT
        3, // const char*, const char*, std::string* // ON_PREJOINPLAYER
        0, // void // CAN_BYPASS_USERLIMIT
        2, // PlayerSAO*, const char* // ON_CHEAT
        //DEPRECATED
        4, // v3s16*, MapNode*, PlayerSAO*, PointedThing* // NODE_ONPUNCH [on_punch on ContentFeatures]
        3, // ItemStack*, PlayerSAO*, PointedThing* // ONSECONDARYUSE
        3, // ItemStack*, PlayerSAO*, PointedThing* // ON_PLACE [on_place on ContentFeatures]
        4, // v3s16*, const char*, StringMap*, PlayerSAO* // ON_NODE_RECEIVEFIELDS
        3, // PlayerSAO*, const char*, StringMap* // ON_PLAYER_RECEIVEFIELDS
        3, // const char*, const char*, bool // ON_AUTHPLAYER
        2, // const char*, const char* // SET_PASSWORD
        3, // v3s16*, MapNode*, PlayerSAO* // ON_DIG
        3, // ItemStack*, PlayerSAO*, PointedThing* // ON_USE [on_use on ItemDefinition]
        3, // v3s16*, MapNode*, float // NODE_ON_TIMER
        1, // PlayerSAO* // ON_RESPAWNPLAYER
        2, // const char*, const char* // FILLED_SERVER
        //END DEPRECATED
    });
    
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
        uint32_t argCount = _argCount.at(_COUNTER);
        CodeHolder code;
        code.init(rt.environment());
        x86::Assembler asm_(&code);
        StringLogger qlog0;
        code.set_logger(&qlog0);
        a = &asm_;
        /// GENERATION ///
        qlog0._log2("\nFUNCTION: ");
        qlog0._log2(std::to_string(_COUNTER).c_str());
        qlog0._log2("\n");
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
        _Arguments(argCount, false);
        a->jmp(_SQ);
        Label _restore = a->new_label();
        a->bind(_restore);
        _Arguments(argCount, true);
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
        a->add(x86::rsp, 24);
        a->pop(x86::rbx);
        a->pop(x86::r15);
        a->pop(x86::r14);
        a->pop(x86::r13);
        a->pop(x86::r12);
        a->mov(x86::rsp, x86::rbp);
        a->pop(x86::rbp);
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
        warningstream << "Function for " <<_COUNTER << " = <addr> " << std::hex << (uint64_t)toalloc << std::dec << std::endl; 
        _COUNTER++;
    }
}


