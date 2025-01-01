cmake_minimum_required(VERSION 3.23)

project(mem_pool LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror -Wno-format -g)

add_subdirectory(src)

set(SRC main.c)

add_executable(memorypool ${SRC})

target_link_libraries(memorypool PRIVATE memory_pool)

target_include_directories(memorypool PRIVATE src)

add_dependencies(memorypool update_timestamp)
