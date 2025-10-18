cmake_minimum_required(VERSION 3.16)
project(Chip-8)

# Set the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# This assumes the SDL source is available in SDL
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# Create your game executable target as usual
add_executable(Chip-8-emulator main.c)

# Link to the actual SDL3 library.
target_link_libraries(Chip-8-emulator PRIVATE SDL3::SDL3)