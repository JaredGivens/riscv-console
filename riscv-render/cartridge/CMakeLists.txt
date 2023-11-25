add_executable( cartridge 
    src/main.c 
    src/crt0.s 
    src/interrupt.s 
    src/spider.c 
    src/spider.h)
target_link_options(cartridge PRIVATE "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/riscv32-console.ld")
set(RISCV_CONSOLE ON)
add_subdirectory(PureC-Software-Renderer/Mango)
target_link_libraries(cartridge PRIVATE Mango)
