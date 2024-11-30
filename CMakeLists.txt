cmake_minimum_required(VERSION 3.23)

project(mem_pool LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror -Wno-format -g)

add_subdirectory(src)

set(SRC main.c)

add_executable(mem_pool_test ${SRC})

target_link_libraries(mem_pool_test PRIVATE memory_pool)

target_include_directories(mem_pool_test PRIVATE src)
