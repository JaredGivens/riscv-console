add_executable(cartridge src/main.c src/crt0.s src/interrupt.s src/startup.c)
target_link_options(cartridge PRIVATE "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/riscv32-console.ld")
