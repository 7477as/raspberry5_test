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

    mvc/stdf_mvc.c
    mvc/stdf_mvc_subject.c
    mvc/stdf_mvc_pool.c
    mvc/stdf_mvc_emit.c
    mvc/stdf_mvc_async.c
    mvc/stdf_mvc_port.c
    mvc/stdf_mvc_port_linux.c

    app/stdf_app.c
    app/hal_dummy/stdf_app_hal_dummy.c
    app/input_mock/stdf_app_input_mock.c
    app/video/stdf_app_video.c
    app/ai/stdf_app_ai.c
    app/gimbal/stdf_app_gimbal.c
    app/imu/stdf_app_imu.c
    app/battery/stdf_app_battery.c
    app/network/stdf_app_network.c
    app/storage/stdf_app_storage.c
    app/indicator/stdf_app_indicator.c
)

target_include_directories(stdf_mvc_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mvc
    ${CMAKE_SOURCE_DIR}/subject
    ${CMAKE_SOURCE_DIR}/payload
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/app/hal_dummy
    ${CMAKE_SOURCE_DIR}/app/input_mock
    ${CMAKE_SOURCE_DIR}/app/video
    ${CMAKE_SOURCE_DIR}/app/ai
    ${CMAKE_SOURCE_DIR}/app/gimbal
    ${CMAKE_SOURCE_DIR}/app/imu
    ${CMAKE_SOURCE_DIR}/app/battery
    ${CMAKE_SOURCE_DIR}/app/network
    ${CMAKE_SOURCE_DIR}/app/storage
    ${CMAKE_SOURCE_DIR}/app/indicator
)

target_compile_options(stdf_mvc_demo PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
    -include ${CMAKE_SOURCE_DIR}/subject/stdf_mvc_subjects.h
)

target_link_libraries(stdf_mvc_demo PRIVATE Threads::Threads)
