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

/*
 * カメラの汎用送信API。将来のモード追加時に、このAPIをラップする形で
 * camera_report_position() 等の専用関数を追加していく想定。
 * mode: camera_mode_t の値を使うこと(0と31は使用禁止)
 */
void camera_report(uint8_t instance_id, uint8_t mode, int16_t v1, int16_t v2, int16_t v3, int16_t v4);

void lump_camera_set_12pos_color(int8_t color_lists[3][4]);

void lump_camera_report_12pos_color(uint8_t instance_id);

bool lump_color_is_calib_request();

void lump_color_calib_fin();

#ifdef __cplusplus
}
#endif