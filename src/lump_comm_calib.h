#pragma once
/**
 * @file lump_comm_calib.h
 * @brief センサーキャリブレーション処理で現在対象となっているセンサー種別と、キャリブレーション開始APIを定義します。
 */

#include <stdint.h>

/** @brief 現在のキャリブレーション対象センサー種別です。 */
typedef enum {
    LUMP_CALIB_SENSOR_NOT,
    LUMP_CALIB_SENSOR_COLOR,
    LUMP_CALIB_SENSOR_CAMERA,
    LUMP_CALIB_SENSOR_MAX
} lump_calib_sensor_t;

/** @brief 現在選択されているキャリブレーション対象センサー種別を保持します。 */
extern lump_calib_sensor_t now_calib_sensor;

/** @brief カメラキャリブレーション処理を開始します。 */
void lump_cam_calib_start();

/** @brief カラーセンサーキャリブレーション処理を開始します。 */
void lump_color_calib_start();