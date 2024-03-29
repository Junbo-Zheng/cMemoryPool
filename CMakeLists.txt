cmake_minimum_required(VERSION 3.10)

project(mem_pool LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MEMORY_POOL_DEBUG "memory pool debug"  OFF)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_compile_options(-Wall -Wextra -Werror -Wno-format -g)

set(SRC
    main.c
    src/malloc.c
)

if(MEMORY_POOL_DEBUG)
  set(SRC ${SRC} src/debug.c)
endif()

add_executable(mem_pool ${SRC})

if(MEMORY_POOL_DEBUG)
  target_compile_definitions(mem_pool PUBLIC -DCONFIG_MEMORY_POOL_DEBUG=1)
endif()

target_include_directories(mem_pool PRIVATE "${CMAKE_SOURCE_DIR}/src/")
