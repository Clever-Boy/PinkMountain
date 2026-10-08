cmake_minimum_required(VERSION 3.25)

project(PinkMountain VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# --- spdlog (logging foundation, Lesson 1) ---
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

# --- GLFW 3.4 (window creation, input, GL context — Lesson 6) ---
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# --- glad (OpenGL function loader — Lesson 7) ---
# glad's sources are GENERATED (gl:core=3.3) and vendored under Pink/vendor/glad,
# the same approach Hazel uses. Vendoring keeps the exact loader version
# reproducible and needs no Python or extra downloads at configure time.
# To regenerate (e.g. for a newer GL version): pip install glad2, then
#   python -m glad --api "gl:core=3.3" --out-path <tmp> c
# and copy <tmp>/{include,src} over Pink/vendor/glad/.

FetchContent_MakeAvailable(spdlog glfw)

add_subdirectory(Pink)
add_subdirectory(Sandbox)
