# GAME A WEEK

cmake_minimum_required(VERSION 3.25)
project(gaw)
        
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_C_FLAGS_DEBUG "-g3")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Werror)
endif()

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/$<TARGET_PROPERTY:NAME>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/$<TARGET_PROPERTY:NAME>")

#find_package(SDL3 REQUIRED)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Create your game executable target as usual
add_executable(test src/EGL/EGL_testing.c src/EGL/EGL_xoshiro128plus.c src/EGL/EGL_xoshiro128plus_test.c)
add_executable(wheel src/gaw_wheel.c src/EGL/EGL_strings.c src/EGL/EGL_xoshiro128plus.c)
add_executable(florbles src/week1/game.c)

target_include_directories(test PUBLIC include)
target_include_directories(wheel PUBLIC include)
target_include_directories(florbles PUBLIC include)

# Link to the actual SDL3 library.
#target_link_libraries(wheel PRIVATE SDL3::SDL3)
target_link_libraries(wheel PRIVATE libSDL3.so libSDL3_ttf.so libcglm.a)
target_link_libraries(florbles PRIVATE libSDL3.so libSDL3_ttf.so libcglm.a)

# Copy necessary data into the target directories
add_custom_command(
    TARGET wheel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/data/games.dat
        ${CMAKE_CURRENT_SOURCE_DIR}/data/themes.dat
        ${CMAKE_CURRENT_SOURCE_DIR}/data/hey_comic.ttf
        ${CMAKE_CURRENT_SOURCE_DIR}/data/wheel.bmp
        $<TARGET_FILE_DIR:wheel>
    )

add_custom_command(
    TARGET florbles POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/data/sphere.bin
        $<TARGET_FILE_DIR:florbles>
    )
