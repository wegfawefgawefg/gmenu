cmake_minimum_required(VERSION 3.20)

project(gmenu VERSION 0.1.0 LANGUAGES CXX)

option(GMENU_STRICT "Enable strict compiler warnings" ON)
option(GMENU_WARN_AS_ERROR "Treat warnings as errors" OFF)
option(GMENU_BUILD_EXAMPLES "Build example programs" ON)
option(GMENU_BUILD_TESTS "Build test programs" ON)
option(GMENU_BUILD_SDL_DEMO "Build SDL3 demo program when SDL3 is available" ON)

set(GMENU_GLAYOUT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../glayout"
    CACHE PATH "Path to a glayout source tree when glayout::core is not already available")
set(GMENU_GINPUT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ginput"
    CACHE PATH "Path to a ginput source tree when ginput::ginput is not already available")

if(NOT TARGET glayout::core)
    if(EXISTS "${GMENU_GLAYOUT_SOURCE_DIR}/CMakeLists.txt")
        set(GLAYOUT_BUILD_EXAMPLES OFF CACHE BOOL "Build glayout examples" FORCE)
        set(GLAYOUT_BUILD_TESTS OFF CACHE BOOL "Build glayout tests" FORCE)
        set(GLAYOUT_BUILD_SDL_DEMO OFF CACHE BOOL "Build glayout SDL demo" FORCE)
        set(GLAYOUT_WITH_IMGUI OFF CACHE BOOL "Build glayout ImGui helpers" FORCE)
        add_subdirectory("${GMENU_GLAYOUT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/glayout")
    else()
        message(FATAL_ERROR
            "glayout::core target was not found and GMENU_GLAYOUT_SOURCE_DIR does not point "
            "to a glayout source tree: ${GMENU_GLAYOUT_SOURCE_DIR}")
    endif()
endif()

if(NOT TARGET ginput::ginput)
    if(EXISTS "${GMENU_GINPUT_SOURCE_DIR}/CMakeLists.txt")
        set(GINPUT_BUILD_EXAMPLES OFF CACHE BOOL "Build ginput examples" FORCE)
        set(GINPUT_BUILD_TESTS OFF CACHE BOOL "Build ginput tests" FORCE)
        add_subdirectory("${GMENU_GINPUT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/ginput")
    else()
        message(FATAL_ERROR
            "ginput::ginput target was not found and GMENU_GINPUT_SOURCE_DIR does not point "
            "to a ginput source tree: ${GMENU_GINPUT_SOURCE_DIR}")
    endif()
endif()

add_library(gmenu
    src/input.cpp
    src/menu.cpp
    src/screens.cpp
    src/types.cpp
    src/view.cpp
    src/widgets.cpp
)

add_library(gmenu::gmenu ALIAS gmenu)

target_include_directories(gmenu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(gmenu PUBLIC cxx_std_20)
target_link_libraries(gmenu PUBLIC glayout::core ginput::ginput)

function(gmenu_apply_warnings target_name)
    if(NOT GMENU_STRICT)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        if(GMENU_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
        if(GMENU_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

gmenu_apply_warnings(gmenu)

if(GMENU_BUILD_EXAMPLES)
    add_executable(gmenu_demo examples/gmenu_demo.cpp)
    target_link_libraries(gmenu_demo PRIVATE gmenu::gmenu)
    target_compile_features(gmenu_demo PRIVATE cxx_std_20)
    gmenu_apply_warnings(gmenu_demo)
endif()

if(GMENU_BUILD_SDL_DEMO)
    find_package(SDL3 CONFIG QUIET)
    if(TARGET SDL3::SDL3)
        set(GMENU_SDL3_TARGET SDL3::SDL3)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL3_PC QUIET IMPORTED_TARGET sdl3)
            if(TARGET PkgConfig::SDL3_PC)
                set(GMENU_SDL3_TARGET PkgConfig::SDL3_PC)
            endif()
        endif()
    endif()

    if(GMENU_SDL3_TARGET)
        add_executable(gmenu_sdl_demo examples/sdl_demo/main.cpp)
        target_link_libraries(gmenu_sdl_demo PRIVATE gmenu::gmenu ${GMENU_SDL3_TARGET})
        target_compile_features(gmenu_sdl_demo PRIVATE cxx_std_20)
        gmenu_apply_warnings(gmenu_sdl_demo)
    else()
        message(STATUS "SDL3 not found; skipping gmenu_sdl_demo")
    endif()
endif()

if(GMENU_BUILD_TESTS)
    enable_testing()

    add_executable(gmenu_tests tests/gmenu_tests.cpp)
    target_link_libraries(gmenu_tests PRIVATE gmenu::gmenu)
    target_compile_features(gmenu_tests PRIVATE cxx_std_20)
    gmenu_apply_warnings(gmenu_tests)

    add_test(NAME gmenu_tests COMMAND gmenu_tests)
endif()
