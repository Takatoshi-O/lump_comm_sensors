#pragma once

#include <stdint.h>

typedef enum {
    LUMP_CALIB_SENSOR_NOT,
    LUMP_CALIB_SENSOR_COLOR,
    LUMP_CALIB_SENSOR_CAMERA,
    LUMP_CALIB_SENSOR_MAX
} lump_calib_sensor_t;

extern lump_calib_sensor_t now_calib_sensor;

void lump_cam_calib_start();

void lump_color_calib_start();