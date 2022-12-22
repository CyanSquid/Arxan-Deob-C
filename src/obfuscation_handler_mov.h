#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "deob_context.h"

extern const uint8_t obfuscation_pattern_mov_rr[7];
extern const uint8_t obfuscation_pattern_mov_ee[11];
extern const uint8_t obfuscation_pattern_mov_re[9];
extern const uint8_t obfuscation_pattern_mov_er[9];

bool obfuscation_handler_mov_rr(const struct deob_context* const context, uint8_t** segment_addresses);
bool obfuscation_handler_mov_ee(const struct deob_context* const context, uint8_t** segment_addresses);
bool obfuscation_handler_mov_re(const struct deob_context* const context, uint8_t** segment_addresses);
bool obfuscation_handler_mov_er(const struct deob_context* const context, uint8_t** segment_addresses);