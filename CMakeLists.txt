cmake_minimum_required(VERSION 3.11)

project(arxandeobc LANGUAGES C)

add_executable(arxandeobc
    "src/main.c"
    "src/read_file.h"
    "src/read_file.c"
    "src/array_util.h"
    "src/x64_util.h"
    "src/win32_util.h" 
    "src/win32_util.c"
    "src/pattern.h" 
    "src/deob_context.h"
    "src/obfuscation_handler_mov.h"
    "src/obfuscation_handler_mov.c"
    "src/obfuscation_handler_push_pop.h" 
    "src/obfuscation_handler_push_pop.c" 
    "src/obfuscation_handler_jmp.h" 
    "src/obfuscation_handler_jmp.c" 
    "src/obfuscation_handler_cjmp.h" 
    "src/obfuscation_handler_cjmp.c"      
    "src/obfuscation_handler_ret.h" 
    "src/obfuscation_handler_ret.c" 
    )

set_property(TARGET arxandeobc PROPERTY C_STANDARD 17)

if (MSVC)
    target_compile_options(arxandeobc PRIVATE 
        "/Wall"     # Enable all warnings
        "/WX"       # Treat warnings as errors   
        "/wd 5045"  # Disable warning "Compiler will insert Spectre mitigation"
        "/wd 4996"  # Disable warning "This function or variable may be unsafe. Consider using function_s instead."
        "/wd 5105"  # Disable warning "Macro expansion producing 'defined' has undefined behaviour"
    )
ENDIF()