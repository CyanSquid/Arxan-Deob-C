#include <string.h>

#include "obfuscation_handler_mov.h"

#include "pattern.h"
#include "x64_util.h"


/*
* This pattern could match any 2 bytes which match ((byte[i] & 0xF0) == 0x50)
* For example, 2 push instructions, 2 pop instructions, a pop then a push, etc
* (only rax through rdi, though)
* Therefore we need to do a little sanity check in the handler to make
* sure we actually got a push followed by a pop
*
* pattern suffixes:
*   r = regular
*   e = extended (rex prefix)
*/

const uint8_t obfuscation_pattern_mov_rr[7] =
{
    0x02,
    0x01, 0x01,

    0x50, // push
    0xF0,

    0x50, // pop
    0xF0,
};

const uint8_t obfuscation_pattern_mov_ee[11] =
{
    0x02,
    0x02, 0x02,

    0x41, 0x50, // REXB push
    0xFF, 0xF0,

    0x41, 0x50, // REXB pop
    0xFF, 0xF0,
};

const uint8_t obfuscation_pattern_mov_re[9] =
{
    0x02,
    0x01, 0x02,

    0x50, // push
    0xF0,

    0x41, 0x50, // REXB pop
    0xFF, 0xF0,
};

const uint8_t obfuscation_pattern_mov_er[9] =
{
    0x02,
    0x02, 0x01,

    0x41, 0x50, // REXB push
    0x41, 0xF0,

    0x50, // pop
    0xF0,
};

bool obfuscation_handler_mov_rr(const struct deob_context* const context, uint8_t** segment_addresses)
{
    (void)context;

    if ((segment_addresses[0][0] & 0x0F) >= 0x08)
    {
        // Found a pop, rather than a push
        return false;
    }

    if ((segment_addresses[1][0] & 0x0F) < 0x08)
    {
        // Found a push, rather than a pop
        return false;
    }

    // If we made it here, we know that we have a byte matching a push (rax through rdi) 
    // followed by a byte matching a pop (rax through rdi)

    // Let's check if we have enough bytes to place a mov REGA, REGB instruction. We need
    // 3 bytes. Because this stage of the deobfuscation comes after we deobfuscate obfuscated
    // push / pops, we can check if it's an address we patched. We'll have at least 3 nops 
    // after the first byte. We only need 2 for this. 

    uint8_t* segment = segment_addresses[0];

    if ((segment[1] != 0x90) || (segment[2] != 0x90))
    {
        segment = segment_addresses[1];

        if ((segment[1] != 0x90) || (segment[2] != 0x90))
        {
            // neither of the segments have enough space to put our mov instruction :(
            return false;
        }
    }

    const uint8_t push_reg_idx = segment_addresses[0][0] & 0x0F;
    const uint8_t pop_reg_idx = (segment_addresses[1][0] & 0x0F) - 0x08;

    segment_addresses[0][0] = 0x90;
    segment_addresses[1][0] = 0x90;

    x64_make_register_mov(segment, pop_reg_idx, push_reg_idx);

    return true;
}

bool obfuscation_handler_mov_ee(const struct deob_context* const context, uint8_t** segment_addresses)
{
    (void)context;

    if ((segment_addresses[0][1] & 0x0F) >= 0x08)
    {
        return false;
    }

    if ((segment_addresses[1][1] & 0x0F) < 0x08)
    {
        return false;
    }

    uint8_t* segment = segment_addresses[0];

    if (segment[2] != 0x90)
    {
        segment = segment_addresses[1];

        if (segment[2] != 0x90)
        {
            return false;
        }
    }

    const uint8_t push_reg_idx = (segment_addresses[0][1] & 0x0F) + 0x08;
    const uint8_t pop_reg_idx = (segment_addresses[1][1] & 0x0F);

    memset(segment_addresses[0], 0x90, 2);
    memset(segment_addresses[1], 0x90, 2);

    x64_make_register_mov(segment, pop_reg_idx, push_reg_idx);

    return true;
}

bool obfuscation_handler_mov_re(const struct deob_context* const context, uint8_t** segment_addresses)
{
    (void)context;

    if ((segment_addresses[0][0] & 0x0F) >= 0x08)
    {
        return false;
    }

    if ((segment_addresses[1][1] & 0x0F) < 0x08)
    {
        return false;
    }

    uint8_t* segment = segment_addresses[0];

    if ((segment[1] != 0x90) || (segment[2] != 0x90))
    {
        segment = segment_addresses[1];

        if (segment[2] != 0x90)
        {
            return false;
        }
    }

    const uint8_t push_reg_idx = segment_addresses[0][0] & 0x0F;
    const uint8_t pop_reg_idx = (segment_addresses[1][1] & 0x0F);

    memset(segment_addresses[0], 0x90, 1);
    memset(segment_addresses[1], 0x90, 2);

    x64_make_register_mov(segment, pop_reg_idx, push_reg_idx);

    return true;
}

bool obfuscation_handler_mov_er(const struct deob_context* const context, uint8_t** segment_addresses)
{
    (void)context;

    if ((segment_addresses[0][1] & 0x0F) >= 0x08)
    {
        return false;
    }

    if ((segment_addresses[1][0] & 0x0F) < 0x08)
    {
        return false;
    }

    uint8_t* segment = segment_addresses[0];

    if (segment[2] != 0x90)
    {
        segment = segment_addresses[1];

        if ((segment[1] != 0x90) || (segment[2] != 0x90))
        {
            // neither of the segments have enough space to put our mov instruction :(
            return false;
        }
    }

    const uint8_t push_reg_idx = (segment_addresses[0][1] & 0x0F) + 0x08;
    const uint8_t pop_reg_idx = (segment_addresses[1][0] & 0x0F) - 0x08;

    memset(segment_addresses[0], 0x90, 2);
    memset(segment_addresses[1], 0x90, 1);

    x64_make_register_mov(segment, pop_reg_idx, push_reg_idx);

    return true;
}