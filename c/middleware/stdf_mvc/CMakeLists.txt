cmake_minimum_required(VERSION 3.16)
project(stdf_mvc_demo VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)
include(FindPkgConfig)
pkg_check_modules(SDL2 sdl2)

set(SUBSYSTEMS_H "${CMAKE_SOURCE_DIR}/subsystem/std_mvc_subsystems.h")

set(APP_SOURCES
    app/std_mvc_apps.c
    app/dm/std_mvc_dm_temperature.c
    app/net/std_mvc_net.c
    app/camera/std_mvc_camera.c
    app/ui/std_mvc_ui_print.c
)

if(SDL2_FOUND)
    list(APPEND APP_SOURCES app/ui/std_mvc_ui_sdl.c)
endif()

add_executable(stdf_mvc_demo
    main.c
    stdf_mvc_api.c

    core/stdf_mvc_core.c
    core/stdf_mvc_core_meta.c
    core/stdf_mvc_core_subject.c
    core/stdf_mvc_core_pool.c
    core/stdf_mvc_core_data.c
    core/stdf_mvc_core_dispatch.c
    core/stdf_mvc_core_async_linux.c
    core/stdf_mvc_core_port.c
    core/stdf_mvc_core_port_linux.c

    ${APP_SOURCES}
)

target_include_directories(stdf_mvc_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/util
    ${CMAKE_SOURCE_DIR}/subsystem
    ${CMAKE_SOURCE_DIR}/subsystem/dm
    ${CMAKE_SOURCE_DIR}/subsystem/net
    ${CMAKE_SOURCE_DIR}/subsystem/camera
    ${CMAKE_SOURCE_DIR}/subsystem/ui
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/app/dm
    ${CMAKE_SOURCE_DIR}/app/net
    ${CMAKE_SOURCE_DIR}/app/camera
    ${CMAKE_SOURCE_DIR}/app/ui
)

target_compile_options(stdf_mvc_demo PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
    -include "${SUBSYSTEMS_H}"
)

target_compile_definitions(stdf_mvc_demo PRIVATE _POSIX_C_SOURCE=200809L)

target_link_libraries(stdf_mvc_demo PRIVATE Threads::Threads)

if(SDL2_FOUND)
    target_compile_definitions(stdf_mvc_demo PRIVATE STDF_MVC_HAS_SDL2)
    target_include_directories(stdf_mvc_demo PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(stdf_mvc_demo PRIVATE ${SDL2_LIBRARIES})
endif()
