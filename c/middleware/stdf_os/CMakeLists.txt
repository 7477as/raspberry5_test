cmake_minimum_required(VERSION 3.16)
project(stdf_demo VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD IMPORTED_TARGET libgpiod)

add_executable(stdf_demo
    main.c
    stdf_main/stdf.c
    stdf_os/stdf_os.c
    stdf_os/stdf_os_mem.c
    stdf_os/stdf_os_msg.c
    stdf_os/stdf_os_delay_msg.c
    stdf_os/stdf_os_port.c
    stdf_app/stdf_app.c
    stdf_app/stdf_app_key.c
    stdf_app/stdf_app_key_sim.c
    stdf_app/stdf_app_heartbeat.c
)

target_include_directories(stdf_demo PRIVATE
    stdf_main
    stdf_os
    stdf_app
)

target_compile_options(stdf_demo PRIVATE
    -Wall -Wextra -Wshadow -Wstrict-prototypes
)

target_compile_definitions(stdf_demo PRIVATE _POSIX_C_SOURCE=200809L)

target_link_libraries(stdf_demo PRIVATE Threads::Threads)

if(GPIOD_FOUND)
    target_compile_definitions(stdf_demo PRIVATE STDF_DEMO_HAS_GPIOD)
    target_link_libraries(stdf_demo PRIVATE PkgConfig::GPIOD)
    message(STATUS "libgpiod found: real GPIO backend enabled")
else()
    message(STATUS "libgpiod NOT found: stdout fallback backend")
endif()
