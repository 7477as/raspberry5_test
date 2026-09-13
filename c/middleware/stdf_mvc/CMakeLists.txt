cmake_minimum_required(VERSION 3.16)
project(stdf_mvc_demo VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Threads REQUIRED)

add_executable(stdf_mvc_demo
    main.c

    stdf_mvc/stdf_mvc.c
    stdf_mvc/stdf_mvc_subject.c
    stdf_mvc/stdf_mvc_pool.c
    stdf_mvc/stdf_mvc_emit.c
    stdf_mvc/stdf_mvc_async.c
    stdf_mvc/stdf_mvc_port.c
    stdf_mvc/stdf_mvc_port_linux.c

    stdf_app/stdf_app.c
    stdf_app/hal_dummy/stdf_app_hal_dummy.c
    stdf_app/input_mock/stdf_app_input_mock.c
    stdf_app/video/stdf_app_video.c
    stdf_app/ai/stdf_app_ai.c
    stdf_app/gimbal/stdf_app_gimbal.c
    stdf_app/imu/stdf_app_imu.c
    stdf_app/battery/stdf_app_battery.c
    stdf_app/network/stdf_app_network.c
    stdf_app/storage/stdf_app_storage.c
    stdf_app/indicator/stdf_app_indicator.c
)

target_include_directories(stdf_mvc_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/stdf_mvc
    ${CMAKE_SOURCE_DIR}/stdf_app
    ${CMAKE_SOURCE_DIR}/stdf_app/subject
    ${CMAKE_SOURCE_DIR}/stdf_app/payload
    ${CMAKE_SOURCE_DIR}/stdf_app/hal_dummy
    ${CMAKE_SOURCE_DIR}/stdf_app/input_mock
    ${CMAKE_SOURCE_DIR}/stdf_app/video
    ${CMAKE_SOURCE_DIR}/stdf_app/ai
    ${CMAKE_SOURCE_DIR}/stdf_app/gimbal
    ${CMAKE_SOURCE_DIR}/stdf_app/imu
    ${CMAKE_SOURCE_DIR}/stdf_app/battery
    ${CMAKE_SOURCE_DIR}/stdf_app/network
    ${CMAKE_SOURCE_DIR}/stdf_app/storage
    ${CMAKE_SOURCE_DIR}/stdf_app/indicator
)

target_compile_options(stdf_mvc_demo PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
    -include ${CMAKE_SOURCE_DIR}/stdf_app/subject/stdf_app_subject.h
)

target_link_libraries(stdf_mvc_demo PRIVATE Threads::Threads)
