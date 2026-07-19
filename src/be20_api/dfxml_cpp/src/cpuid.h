/*
 * Revision History:
 *  2015 - Initial implementation, based on https://stackoverflow.com/questions/1666093/cpuid-implementations-in-c
 *  2021 - Cleaned up. Added LGPL copyright notice.
 *
 * Copyright (C) 2021 Simson L. Garfinkel.
 *
 * LICENSE: LGPL Version 3. See COPYING.md for further information.
 */

#ifndef CPUID_H
#define CPUID_H

#ifdef _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;
#endif

#include <cstdint>
#include <string>

class CPUID {
#if defined(_WIN32) || defined(HAVE_ASM_CPUID)
    uint32_t regs[4];
#endif

public:
    explicit CPUID(unsigned i) {
#ifdef _WIN32
        __cpuid((int *)regs, (int)i);
#elif HAVE_ASM_CPUID
        asm volatile
            ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
             : "a" (i), "c" (0));
        // ECX is set to zero for CPUID function 4
#else
#endif
    }

#if defined(_WIN32) || defined(HAVE_ASM_CPUID)
    const uint32_t &EAX() const {return regs[0];}
    const uint32_t &EBX() const {return regs[1];}
    const uint32_t &ECX() const {return regs[2];}
    const uint32_t &EDX() const {return regs[3];}
#endif

    static std::string vendor() {
        std::string vendor;
#if defined(_WIN32) || defined(HAVE_ASM_CPUID)
        CPUID  cpuID(0);                     // get CPU vendor
        vendor += std::string((const char *)&cpuID.EBX(), 4);
        vendor += std::string((const char *)&cpuID.EDX(), 4);
        vendor += std::string((const char *)&cpuID.ECX(), 4);
#endif
        return vendor;
    }
};

#endif // CPUID_H
