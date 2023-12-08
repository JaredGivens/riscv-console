cmake_minimum_required(VERSION 3.13)
project(ecs251 LANGUAGES C ASM)
# Set the C and C++ compilers for cross-compilation
set(CMAKE_C_COMPILER "riscv32-unknown-elf-gcc")
set(CMAKE_ASM_COMPILER "riscv32-unknown-elf-gcc")

set(CMAKE_C_STANDARD 11)
set(C_STANDARD_REQUIRED ON)

# Specify required flags
set(CMAKE_C_FLAGS "-ffreestanding -nostartfiles -Wall -Werror")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(RVC ON)
add_subdirectory(firmware)
add_subdirectory(PureC-Software-Renderer)


