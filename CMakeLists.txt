cmake_minimum_required(VERSION 3.20)

project(gmenu VERSION 0.1.0 LANGUAGES CXX)

option(GMENU_STRICT "Enable strict compiler warnings" ON)
option(GMENU_WARN_AS_ERROR "Treat warnings as errors" OFF)
option(GMENU_BUILD_EXAMPLES "Build example programs" ON)
option(GMENU_BUILD_TESTS "Build test programs" ON)
option(GMENU_BUILD_SDL_DEMO "Build SDL3 demo program when SDL3 is available" ON)
option(GMENU_WITH_IMGUI "Build optional Dear ImGui helpers" OFF)
set(GMENU_IMGUI_TARGET "" CACHE STRING "Dear ImGui CMake target for optional ImGui helpers")
set(GMENU_IMGUI_SOURCE_DIR "" CACHE PATH "Optional Dear ImGui source tree used to create an imgui target")

set(GMENU_GLAYOUT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../glayout"
    CACHE PATH "Path to a glayout source tree when glayout::core is not already available")
set(GMENU_GINPUT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ginput"
    CACHE PATH "Path to a ginput source tree when ginput::ginput is not already available")

if(NOT TARGET glayout::core)
    if(EXISTS "${GMENU_GLAYOUT_SOURCE_DIR}/CMakeLists.txt")
        set(GLAYOUT_BUILD_EXAMPLES OFF CACHE BOOL "Build glayout examples" FORCE)
        set(GLAYOUT_BUILD_TESTS OFF CACHE BOOL "Build glayout tests" FORCE)
        set(GLAYOUT_BUILD_SDL_DEMO OFF CACHE BOOL "Build glayout SDL demo" FORCE)
        if(GMENU_WITH_IMGUI)
            set(GLAYOUT_WITH_IMGUI ON CACHE BOOL "Build glayout ImGui helpers" FORCE)
            if(GMENU_IMGUI_TARGET)
                set(GLAYOUT_IMGUI_TARGET "${GMENU_IMGUI_TARGET}" CACHE STRING
                    "Dear ImGui CMake target for optional ImGui helpers" FORCE)
            endif()
            if(GMENU_IMGUI_SOURCE_DIR)
                set(GLAYOUT_IMGUI_SOURCE_DIR "${GMENU_IMGUI_SOURCE_DIR}" CACHE PATH
                    "Optional Dear ImGui source tree used to create an imgui target" FORCE)
            endif()
        else()
            set(GLAYOUT_WITH_IMGUI OFF CACHE BOOL "Build glayout ImGui helpers" FORCE)
        endif()
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
    src/input_screens.cpp
    src/menu.cpp
    src/nav.cpp
    src/nav_io.cpp
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

if(GMENU_WITH_IMGUI)
    set(GMENU_RESOLVED_IMGUI_TARGET "")
    if(GMENU_IMGUI_TARGET AND TARGET ${GMENU_IMGUI_TARGET})
        set(GMENU_RESOLVED_IMGUI_TARGET ${GMENU_IMGUI_TARGET})
    elseif(TARGET imgui)
        set(GMENU_RESOLVED_IMGUI_TARGET imgui)
    elseif(TARGET ImGui::ImGui)
        set(GMENU_RESOLVED_IMGUI_TARGET ImGui::ImGui)
    elseif(GMENU_IMGUI_SOURCE_DIR AND EXISTS "${GMENU_IMGUI_SOURCE_DIR}/imgui.cpp")
        add_library(imgui STATIC
            "${GMENU_IMGUI_SOURCE_DIR}/imgui.cpp"
            "${GMENU_IMGUI_SOURCE_DIR}/imgui_draw.cpp"
            "${GMENU_IMGUI_SOURCE_DIR}/imgui_tables.cpp"
            "${GMENU_IMGUI_SOURCE_DIR}/imgui_widgets.cpp"
        )
        target_include_directories(imgui PUBLIC "${GMENU_IMGUI_SOURCE_DIR}")
        target_compile_features(imgui PUBLIC cxx_std_20)
        set(GMENU_RESOLVED_IMGUI_TARGET imgui)
    endif()

    if(NOT GMENU_RESOLVED_IMGUI_TARGET)
        message(FATAL_ERROR
            "GMENU_WITH_IMGUI=ON requires GMENU_IMGUI_TARGET, imgui, ImGui::ImGui, "
            "or GMENU_IMGUI_SOURCE_DIR")
    endif()

    add_library(gmenu_imgui
        src/imgui.cpp
    )
    add_library(gmenu::imgui ALIAS gmenu_imgui)
    target_link_libraries(gmenu_imgui PUBLIC gmenu::gmenu ${GMENU_RESOLVED_IMGUI_TARGET})
    target_include_directories(gmenu_imgui
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_compile_features(gmenu_imgui PUBLIC cxx_std_20)
    gmenu_apply_warnings(gmenu_imgui)
endif()

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

        set(GMENU_SDL_DEMO_IMGUI_BACKENDS_AVAILABLE OFF)
        if(GMENU_IMGUI_SOURCE_DIR)
            set(GMENU_IMGUI_SDL3_BACKEND "${GMENU_IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp")
            set(GMENU_IMGUI_SDLRENDERER3_BACKEND
                "${GMENU_IMGUI_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp")
            if(EXISTS "${GMENU_IMGUI_SDL3_BACKEND}" AND EXISTS "${GMENU_IMGUI_SDLRENDERER3_BACKEND}")
                set(GMENU_SDL_DEMO_IMGUI_BACKENDS_AVAILABLE ON)
            endif()
        endif()

        if(GMENU_WITH_IMGUI AND TARGET gmenu::imgui AND TARGET glayout::imgui AND
                GMENU_SDL_DEMO_IMGUI_BACKENDS_AVAILABLE)
            if(MSVC)
                set_source_files_properties(
                    "${GMENU_IMGUI_SDL3_BACKEND}"
                    "${GMENU_IMGUI_SDLRENDERER3_BACKEND}"
                    PROPERTIES COMPILE_OPTIONS /w)
            else()
                set_source_files_properties(
                    "${GMENU_IMGUI_SDL3_BACKEND}"
                    "${GMENU_IMGUI_SDLRENDERER3_BACKEND}"
                    PROPERTIES COMPILE_OPTIONS -w)
            endif()
            target_sources(gmenu_sdl_demo PRIVATE
                examples/sdl_demo/imgui_debug.cpp
                "${GMENU_IMGUI_SDL3_BACKEND}"
                "${GMENU_IMGUI_SDLRENDERER3_BACKEND}"
            )
            target_include_directories(gmenu_sdl_demo PRIVATE
                "${GMENU_IMGUI_SOURCE_DIR}"
                "${GMENU_IMGUI_SOURCE_DIR}/backends"
            )
            target_link_libraries(gmenu_sdl_demo PRIVATE gmenu::imgui glayout::imgui)
            target_compile_definitions(gmenu_sdl_demo PRIVATE GMENU_SDL_DEMO_WITH_IMGUI=1)
        endif()

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

    add_executable(gmenu_screen_tests tests/screen_tests.cpp)
    target_link_libraries(gmenu_screen_tests PRIVATE gmenu::gmenu)
    target_compile_features(gmenu_screen_tests PRIVATE cxx_std_20)
    gmenu_apply_warnings(gmenu_screen_tests)

    add_test(NAME gmenu_screen_tests COMMAND gmenu_screen_tests)
endif()
