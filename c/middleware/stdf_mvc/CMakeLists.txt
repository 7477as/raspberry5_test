cmake_minimum_required(VERSION 3.16)
project(stdf_mvc_demo VERSION 1.0.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
find_package(Threads REQUIRED)
option(STDF_MVC_ENABLE_SDL2 "Build the SDL2 application UI when available" ON)
if(STDF_MVC_ENABLE_SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
endif()
add_library(stdf_mvc STATIC
    mvc/stdf_mvc.c
    mvc/stdf_mvc_api.c
    mvc/core/stdf_mvc_core.c
    mvc/core/stdf_mvc_core_async.c
    mvc/core/stdf_mvc_core_subject.c
    mvc/core/stdf_mvc_core_value.c
    mvc/core/stdf_mvc_core_port.c
    mvc/core/stdf_mvc_core_port_linux.c
)
target_include_directories(stdf_mvc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mvc
    PRIVATE mvc/core mvc/subsystem mvc/config mvc/util)
target_compile_definitions(stdf_mvc PUBLIC _POSIX_C_SOURCE=200809L)
target_link_libraries(stdf_mvc PUBLIC Threads::Threads)
add_executable(stdf_mvc_demo main.c
    app/stdf_app.c app/dm/stdf_app_dm_temperature.c
    app/net/stdf_app_net.c app/camera/stdf_app_camera.c
    app/ui/stdf_app_ui.c app/ui/stdf_app_ui_print.c)
target_include_directories(stdf_mvc_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stdf_mvc_demo PRIVATE stdf_mvc)
if(STDF_MVC_ENABLE_SDL2 AND SDL2_FOUND)
    target_sources(stdf_mvc_demo PRIVATE app/ui/stdf_app_ui_sdl.c)
    target_compile_definitions(stdf_mvc_demo PRIVATE STDF_MVC_HAS_SDL2)
    target_link_libraries(stdf_mvc_demo PRIVATE PkgConfig::SDL2)
endif()
foreach(target stdf_mvc stdf_mvc_demo)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes)
endforeach()