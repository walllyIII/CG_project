cmake_minimum_required(VERSION 3.16)
project(CourtyardReflections LANGUAGES C CXX)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 QUIET)
if(NOT TARGET glfw)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_EXAMPLES OFF)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw)
endif()

add_executable(courtyard src/main.cpp)
target_compile_features(courtyard PRIVATE cxx_std_17)
target_link_libraries(courtyard PRIVATE glfw OpenGL::GL)
