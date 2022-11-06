cmake_minimum_required(VERSION 3.10)

SET(CMAKE_C_COMPILER /usr/bin/clang)

project(mem_pool LANGUAGES C)

set(SRC
    main.c
    src/malloc.c
)

set(CMAKE_C_FLAGS "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -static-libsan")

option(MEMORY_POOL_DEBUG "memory pool debug"  OFF)

if(MEMORY_POOL_DEBUG)
  set(SRC ${SRC} src/debug.c)
endif()

add_executable(mem_pool ${SRC})

if(MEMORY_POOL_DEBUG)
  target_compile_definitions(mem_pool PUBLIC -DCONFIG_MEMORY_POOL_DEBUG=1)
endif()

add_compile_options(-Wall -Wextra -Werror -Wno-format -g)

target_include_directories(mem_pool PRIVATE "${CMAKE_SOURCE_DIR}/src/")
