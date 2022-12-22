#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "array_util.h"
#include "read_file.h"
#include "pattern.h"
#include "x64_util.h"

#include "deob_context.h"
#include "obfuscation_handler_mov.h"
#include "obfuscation_handler_push_pop.h"
#include "obfuscation_handler_jmp.h"
#include "obfuscation_handler_cjmp.h"
#include "obfuscation_handler_ret.h"
#include "win32_util.h"

struct pattern_info
{
    const char* name;
    const uint8_t* pattern;
    bool (*handler)(const struct deob_context* const context, uint8_t** segment_addresses);
};

static const struct pattern_info obfuscation_patterns_stage_1[] =
{
    {
        "'push'",
        obfuscation_pattern_push_1,
        obfuscation_handler_push_1
    },

    {
        "'push'",
        obfuscation_pattern_push_2,
        obfuscation_handler_push_2
    },

    {
        "'pop'",
        obfuscation_pattern_pop_1,
        obfuscation_handler_pop_1
    },

    {
        "'pop'",
        obfuscation_pattern_pop_2,
        obfuscation_handler_pop_2
    },

    {
        "'ret'",
        obfuscation_pattern_ret,
        obfuscation_handler_ret
    },
};

static const struct pattern_info obfuscation_patterns_stage_2[] =
{
    {
        "'jmp'",
        obfuscation_pattern_jmp,
        obfuscation_handler_jmp
    },

    {
        "'cjmp'",
        obfuscation_pattern_cjmp,
        obfuscation_handler_cjmp
    },

    {
        "'mov'",
        obfuscation_pattern_mov_rr,
        obfuscation_handler_mov_rr
    },

    {
        "'mov'",
        obfuscation_pattern_mov_ee,
        obfuscation_handler_mov_ee
    },

    {
        "'mov'",
        obfuscation_pattern_mov_re,
        obfuscation_handler_mov_re
    },

    {
        "'mov'",
        obfuscation_pattern_mov_er,
        obfuscation_handler_mov_er
    },
};

static inline bool does_byte_match(const uint8_t data, const uint8_t cmp_data, const uint8_t cmp_mask)
{
    return !((data ^ cmp_data) & cmp_mask);
}

static bool compare_data(const uint8_t* data, const uint8_t* pattern_data, const uint8_t* pattern_mask, const size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (!does_byte_match(data[i], pattern_data[i], pattern_mask[i]))
        {
            return false;
        }
    }
    return true;
}

static bool match_pattern(const uint8_t* address, const uint8_t* pattern, const uint8_t** segment_addresses, const uint8_t* const max_jmp, const uint8_t* const min_jmp)
{
    const size_t segment_count = pattern_get_section_count(pattern);
    size_t segment = 0;
    while (segment < segment_count)
    {
        uint8_t* data;
        uint8_t* mask;
        size_t size = pattern_get_section(pattern, segment, &data, &mask);

        if (!compare_data(address, data, mask, size))
        {
            if (segment)
            {
                if (address[0] == 0x90)
                {
                    address += 1;
                    continue;
                }

                if (address[0] == 0xE9)
                {
                    const uintptr_t virtual_address = file_offset_to_virtual_address(min_jmp, (uint32_t)(address - min_jmp));
                    const uint32_t file_offset = virtual_address_to_file_offset(min_jmp, x64_calc_rel32(virtual_address + 1, *(int32_t*)(address + 1), 0));
                    address = min_jmp + file_offset;
                    if (address > max_jmp || address < min_jmp)
                    {
                        return false;
                    }
                    continue;
                }
            }
            return false;
        }
        segment_addresses[segment] = address;

        segment += 1;
        address += size;
    }

    return true;
}

static void find_and_deobfuscate(const struct deob_context* const context, const struct pattern_info* patterns, const size_t pattern_count)
{
    uint8_t* addr = context->deob_begin;
    uint8_t* stop = context->deob_end;

    while (addr < stop)
    {
        size_t skip = 0;

        for (size_t i = 0; i < pattern_count; i++)
        {
            uint8_t* segment_addresses[15];
            if (match_pattern(addr, patterns[i].pattern, segment_addresses, context->deob_end, context->deob_begin))
            {
                skip = pattern_get_section_size(patterns[i].pattern, 0);
                if (patterns[i].handler(context, segment_addresses))
                {
                    fprintf(context->log_file, "%" PRIXPTR " : Patched obfuscated %s\n", file_offset_to_virtual_address(context->deob_begin, (uint32_t)(addr - context->deob_begin)), patterns[i].name);
                    fflush(context->log_file);
                }
                break;
            }
        }

        if (skip)
        {
            addr += skip;
            continue;
        }

        addr++;
    }
}

static uintptr_t get_jmp_chain_target(const struct deob_context* context, const uint8_t* address)
{
    uintptr_t virtual_address = file_offset_to_virtual_address(context->deob_begin, (uint32_t)(address - context->deob_begin));
    uintptr_t target = 0;

    while (address[0] == 0xE9)
    {
        target = x64_calc_rel32(virtual_address + 1, *(int32_t*)(address + 1), 0);

        const uint32_t file_offset = virtual_address_to_file_offset(context->deob_begin, target);

        if (file_offset == UINT32_MAX)
        {
            return 0;
        }

        virtual_address = target;
        address = (context->deob_begin + file_offset);
    }
    return target;
}

static void skip_jmps(const struct deob_context* context)
{
    uint8_t* addr = context->deob_begin;
    uint8_t* stop = context->deob_end;

    while (addr < stop)
    {
        if (addr[0] != 0xE9)
        {
            addr++;
            continue;
        }

        const uintptr_t virtual_address = file_offset_to_virtual_address(context->deob_begin, (uint32_t)(addr - context->deob_begin));
        const uintptr_t initial_target = x64_calc_rel32(virtual_address + 1, *(int32_t*)(addr + 1), 0);
        const uintptr_t target = get_jmp_chain_target(context, addr);
        const uint32_t target_file_offset = virtual_address_to_file_offset(context->deob_begin, target);

        if ((target == 0) || (target_file_offset == UINT32_MAX))
        {
            addr++;
            continue;
        }

        if (context->deob_begin[target_file_offset] == 0xC3)
        {
            memset(addr, 0x90, 5);
            addr[0] = 0xC3;
            fprintf(context->log_file, "Skipped 'jmp' into 'ret': %" PRIXPTR " -> %" PRIXPTR "\n", file_offset_to_virtual_address(context->deob_begin, (uint32_t)(addr - context->deob_begin)), target);
        }
        else if (target != initial_target)
        {
            x64_make_rel32_jmp(addr, virtual_address, target);
            fprintf(context->log_file, "Skipped one or more 'jmp's: %" PRIXPTR " -> %" PRIXPTR "\n", file_offset_to_virtual_address(context->deob_begin, (uint32_t)(addr - context->deob_begin)), target);
        } 
        else
        {
            addr++;
            continue;
        }

        addr += 5;
    }
}

static void deobfuscate(const struct deob_context* const context)
{
    find_and_deobfuscate(context, obfuscation_patterns_stage_1, ARRAY_SIZE(obfuscation_patterns_stage_1));
    find_and_deobfuscate(context, obfuscation_patterns_stage_2, ARRAY_SIZE(obfuscation_patterns_stage_2));
    skip_jmps(context);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("usage: arxandeobc dumpfile.exe\n");
        return 1;
    }

    const char* dumpfile = argv[1];

    struct memory_range file_data;

    if (!read_binary_file(&file_data, dumpfile))
    {
        printf("Couldn't open dump file!");
        return 2;
    }

    FILE* log_file = fopen("deob.log", "w+");

    struct deob_context context = { log_file, file_data.begin, file_data.end };
    deobfuscate(&context);

    fclose(log_file);

    FILE* out = fopen("deob.exe", "wb+");
    fwrite(file_data.begin, 1, (file_data.end - file_data.begin), out);
    fflush(out);
    fclose(out);

    free(file_data.begin);

    return 0;
}