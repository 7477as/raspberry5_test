/* stdf_app_imu - IMU 数据源 */

#include "stdf_app_imu.h"
#include "stdf_mvc.h"
#include "stdf_mvc_payloads.h"
#include "stdf_app_hal_dummy.h"

#define STDF_APP_LOG(...)             do { } while (0)
#define STDF_APP_IMU_ASSERT(cond)

static uint64_t s_tick_ms = 0;

int stdf_app_imu_init(void)
{
    return 0;
}

void stdf_app_imu_tick(void)
{
    const stdf_app_hal_dummy_state_t *hal = stdf_app_hal_dummy_get_state();
    s_tick_ms += 10;

    stdf_mvc_payload_imu_accel_t accel = {
        .ax = hal->imu_accel[0],
        .ay = hal->imu_accel[1],
        .az = hal->imu_accel[2],
        .timestamp_ms = s_tick_ms,
    };
    stdf_mvc_signal_data_t p = { .ptr = &accel };
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_IMU_ACCEL, &p);

    stdf_mvc_payload_imu_gyro_t gyro = {
        .gx = hal->imu_gyro[0],
        .gy = hal->imu_gyro[1],
        .gz = hal->imu_gyro[2],
        .timestamp_ms = s_tick_ms,
    };
    p.ptr = &gyro;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_IMU_GYRO, &p);

    stdf_mvc_payload_imu_quaternion_t quat = {
        .x = hal->imu_quat[0],
        .y = hal->imu_quat[1],
        .z = hal->imu_quat[2],
        .w = hal->imu_quat[3],
        .timestamp_ms = s_tick_ms,
    };
    p.ptr = &quat;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_IMU_QUATERNION, &p);

    stdf_mvc_payload_imu_euler_t euler = {
        .roll_deg  = hal->imu_quat[0] * 57.3f,
        .pitch_deg = hal->imu_quat[1] * 57.3f,
        .yaw_deg   = hal->imu_quat[2] * 57.3f,
        .timestamp_ms = s_tick_ms,
    };
    p.ptr = &euler;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_IMU_EULER_ANGLE, &p);

    stdf_mvc_payload_imu_temperature_t temp = {
        .temperature_c = hal->imu_temperature_c,
    };
    p.ptr = &temp;
    stdf_mvc_subject_emit(STDF_MVC_SUBJECT_IMU_TEMPERATURE, &p);
}
