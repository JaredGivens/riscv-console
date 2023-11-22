project(ecs251)
cmake_minimum_required(VERSION 3.13)
# Set the C and C++ compilers for cross-compilation
set(CMAKE_C_COMPILER "riscv32-unknown-elf-gcc")
set(CMAKE_ASM_COMPILER "riscv32-unknown-elf-gcc")
enable_language(ASM)
set(CMAKE_C_COMPILER_WORKS 1)

# Specify additional flags or settings required for cross-compilation
set(CMAKE_C_FLAGS "-ffreestanding -nostartfiles")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
add_subdirectory(firmware)
add_subdirectory(cartridge)


