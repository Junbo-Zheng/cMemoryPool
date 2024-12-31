cmake_minimum_required(VERSION 3.23)

# ~~~
# Build memory pool static library, add source entry
# ~~~
set(MEM_POOL_SRC malloc.c)

# Option to enable memory pool debug
option(MEMORY_POOL_DEBUG "Enable memory pool debug" OFF)

# Create static library
add_library(memory_pool STATIC ${MEM_POOL_SRC})

# If debugging is enabled, add debug source and definition
if(MEMORY_POOL_DEBUG)
  # Add debug.c to the target's sources
  target_sources(memory_pool PRIVATE debug.c)
  target_compile_definitions(memory_pool PUBLIC -DCONFIG_MEMORY_POOL_DEBUG=1)
endif()

# Include current directory for memory pool
target_include_directories(memory_pool PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Add compile options for the memory pool target
target_compile_options(memory_pool PRIVATE -Wall -Wextra -Werror -Wno-format -g)

add_custom_target(update_timestamp COMMAND ${CMAKE_COMMAND} -E touch
                                           ${CMAKE_CURRENT_LIST_DIR}/malloc.c)
