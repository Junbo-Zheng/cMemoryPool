cmake_minimum_required(VERSION 3.10)

project(mem_pool LANGUAGES C)

set(SRC
    main.c
    src/malloc.c
)

add_executable(mem_pool ${SRC})

add_compile_options(-Wall -Wextra -Werror -Wno-format -g)

target_include_directories(mem_pool PRIVATE "${CMAKE_SOURCE_DIR}/src/")
