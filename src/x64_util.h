#pragma once

#include <stdint.h>
#include <stddef.h>

#define X64_REGISTER_ID_R8 (0x08)

static inline uintptr_t x64_calc_rel32(const uintptr_t address, const int32_t rel, const size_t offset)
{
    return (address + rel + sizeof(int32_t) + offset);
}

static inline void x64_make_rel32_jmp(void* addr, const uintptr_t jmp_from, const uintptr_t jmp_to)
{
    uint8_t* a = addr;

    a[0] = 0xE9;
    *((uint32_t*)(a + 1)) = (uint32_t)(jmp_to - (jmp_from + 5));
    return;
}

static inline void x64_make_rel32_cjmp(void* addr, const uintptr_t jmp_from, const uintptr_t jmp_to, const uint8_t type)
{
    uint8_t* a = addr;

    a[0] = 0x0F;
    a[1] = type;
    *((uint32_t*)(a + 2)) = (uint32_t)(jmp_to - (jmp_from + 6));
    return;
}

static inline void x64_make_register_mov(void* out, const uint8_t reg_idx_lhs, const uint8_t reg_idx_rhs)
{
    uint8_t* mov = out;

    mov[1] = 0x89;

    mov[0] = (reg_idx_rhs >= X64_REGISTER_ID_R8) ? 0x4C : 0x48;

    if (reg_idx_lhs >= X64_REGISTER_ID_R8)
    {
        mov[0] += 0x01;
    }

    const uint8_t multiplier = (reg_idx_rhs >= X64_REGISTER_ID_R8) ? (reg_idx_rhs - 0x08) : reg_idx_rhs;

    mov[2] = 0xC0 + ((0x8 * multiplier) + reg_idx_lhs);

    if (reg_idx_lhs >= X64_REGISTER_ID_R8)
    {
        mov[2] -= 0x08;
    }
}