/* stdf_mvc_payload_imu - IMU 姿态域 payload */

#ifndef __STDF_MVC_PAYLOAD_IMU_H__
#define __STDF_MVC_PAYLOAD_IMU_H__

#include <stdint.h>

typedef struct {
    float       ax;
    float       ay;
    float       az;
    uint64_t    timestamp_ms;
} stdf_mvc_payload_imu_accel_t;

typedef struct {
    float       gx;
    float       gy;
    float       gz;
    uint64_t    timestamp_ms;
} stdf_mvc_payload_imu_gyro_t;

typedef struct {
    float       x;
    float       y;
    float       z;
    float       w;
    uint64_t    timestamp_ms;
} stdf_mvc_payload_imu_quaternion_t;

typedef struct {
    float       roll_deg;
    float       pitch_deg;
    float       yaw_deg;
    uint64_t    timestamp_ms;
} stdf_mvc_payload_imu_euler_t;

typedef struct {
    float       temperature_c;
} stdf_mvc_payload_imu_temperature_t;

#endif
