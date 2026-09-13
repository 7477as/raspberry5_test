/* stdf_app_subject_imu - IMU 姿态域 subject */

#ifndef __STDF_APP_SUBJECT_IMU_H__
#define __STDF_APP_SUBJECT_IMU_H__

#define STDF_APP_SUBJECTS_IMU \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_IMU_ACCEL,                "imu.accel") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_IMU_GYRO,                 "imu.gyro") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_IMU_QUATERNION,           "imu.quaternion") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_IMU_EULER_ANGLE,          "imu.euler") \
    STDF_MVC_SUBJECT_X(STDF_MVC_SUBJECT_IMU_TEMPERATURE,          "imu.temperature")

#endif
