#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * lump_device ライブラリの上に乗るセンサー別APIレイヤー。
 * センサードライバ(I2C等)は別途用意し、取得した値をこのAPIに渡す形で使う。
 *
 * モード番号の共通ルール:
 *   0   = 予約(初期設定用。センサー初期化完了通知・エラー状態通知等)
 *   1〜31 = センサーごとの有効モード
 */

/* ===================== カメラ ===================== */

#define LUMP_SENSOR_CAMERA LUMP_TYPE_2

typedef struct 
{
    bool isrequest;
    uint8_t instanceID;
    int16_t x, y;
    uint8_t px_size;
} request_get_color_t;

typedef struct 
{
    bool isrequest;
    uint8_t instanceID;
    int16_t data_list;
    uint8_t px_size;
} request_get_12pos_color_t;

/*
 * カメラの汎用送信API。将来のモード追加時に、このAPIをラップする形で
 * camera_report_position() 等の専用関数を追加していく想定。
 * mode: camera_mode_t の値を使うこと(0と31は使用禁止)
 */
void lump_camera_init(void);

void camera_sensor_instance_active(uint8_t instance_id);

bool camera_sensor_is_instance_active(uint8_t instance_id);

void lump_camera_report_color(uint8_t instance_id);

void lump_camera_set_color(uint8_t instanceID, int16_t y, int16_t u, int16_t v, int16_t colorID);

void lump_camera_set_12pos_color(uint8_t color_lists[3][4]);

void lump_camera_report_12pos_color(uint8_t instance_id, uint8_t data_list);

request_get_color_t lump_camera_is_request_color();

void lump_camera_request_color_fin();

request_get_12pos_color_t lump_camera_is_request_12pos_color();

void lump_camera_request_12pos_color_fin();

#ifdef __cplusplus
}
#endif