#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 初期設定 ===================== */

#define LUMP_MAX_INSTANCES_PER_TYPE 8
#define LUMP_SENSOR_INIT 0

//Color ID
typedef enum {
    LUMP_COLOR_UNKNOWN = 0,
    LUMP_COLOR_BLACK,
    LUMP_COLOR_WHITE,
    LUMP_COLOR_RED,
    LUMP_COLOR_GREEN,
    LUMP_COLOR_BLUE,
    LUMP_COLOR_YELLOW,
    LUMP_COLOR_ORANGE,
    LUMP_COLOR_PURPLE,
    LUMP_COLOR_CYAN,
    LUMP_COLOR_MAGENTA,
    LUMP_COLOR_BROWN,
    LUMP_COLOR_GRAY,
    LUMP_COLOR_PINK,
    LUMP_COLOR_LIME,
    LUMP_COLOR_NAVY,
} lump_color_id_t;

/*
 * lump_device ライブラリの上に乗るセンサー別APIレイヤー。
 * センサードライバ(I2C等)は別途用意し、取得した値をこのAPIに渡す形で使う。
 *
 * モード番号の共通ルール:
 *   0   = 予約(初期設定用。センサー初期化完了通知・エラー状態通知等)
 *   1〜30 = センサーごとの有効モード
 *   31  = キャリブレーション予約(lump_deviceが内部使用。このAPIからは使用禁止)
 */

/* ===================== 共通: 初期設定モード(モード0) ===================== */

/*
 * センサーの状態をモード0(予約: 初期設定用)として送信する。
 * 起動直後の初期化完了通知や、エラー状態の通知に使う。
 *   status: 0=初期化中, 1=初期化完了, 負値=エラーコード
 */
void sensor_report_status(lump_sensor_type_t type, uint8_t instance_id, int16_t status);

void lump_sensors_start(void);

const char *lump_color_id_to_char(lump_color_id_t color);

void lump_calib_start();

void lump_sersors_register();

#ifdef __cplusplus
}
#endif