file(GLOB_RECURSE PINK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(Pink STATIC ${PINK_SOURCES})

target_include_directories(Pink PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Vendored glad loader (see root CMakeLists.txt): a real target with PUBLIC
# headers, so `#include <glad/gl.h>` resolves for anything linking Pink.
add_library(glad STATIC vendor/glad/src/gl.c)
target_include_directories(glad PUBLIC vendor/glad/include)

# Vendored stb_image (Lesson 10): single-header image loader, compiled once
# as C++. stb_image.cpp #defines STB_IMAGE_IMPLEMENTATION exactly once —
# the header alone in any other TU is just declarations.
add_library(stb STATIC vendor/stb/stb_image.cpp)
target_include_directories(stb PUBLIC vendor/stb)

target_link_libraries(Pink
    PUBLIC spdlog::spdlog
    PRIVATE glfw glad stb
)

# Debug asserts (PM_CORE_ASSERT / PM_ASSERT) compile out in Release.
target_compile_definitions(Pink PUBLIC
    $<$<CONFIG:Debug>:PM_ENABLE_ASSERTS>
)

if(MSVC)
    target_compile_options(Pink PRIVATE /W4)
else()
    target_compile_options(Pink PRIVATE -Wall -Wextra)
endif()
