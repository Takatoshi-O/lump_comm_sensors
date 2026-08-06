#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== 初期設定 ===================== */

#define LUMP_MAX_INSTANCES_PER_TYPE 16
#define LUMP_SENSOR_INIT 0

//Color ID
typedef enum {
    COLOR_BLACK = 0,
    COLOR_WHITE,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_ORANGE,
    COLOR_PURPLE,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_BROWN,
    COLOR_GRAY,
    COLOR_PINK,
    COLOR_LIME,
    COLOR_NAVY,
    COLOR_UNKNOWN,
} color_id_t;

/*
 * lump_device ライブラリの上に乗るセンサー別APIレイヤー。
 * センサードライバ(I2C等)は別途用意し、取得した値をこのAPIに渡す形で使う。
 *
 * モード番号の共通ルール:
 *   0   = 予約(初期設定用。センサー初期化完了通知・エラー状態通知等)
 *   1〜30 = センサーごとの有効モード
 *   31  = キャリブレーション予約(lump_deviceが内部使用。このAPIからは使用禁止)
 */

/*
 * モード番号の使用禁止チェック。
 * 0=初期設定予約のためアサートで弾く。
 * このチェックを通過したモードのみ lump_device_report() へ渡す。
 */
bool is_valid_mode(uint8_t mode); 

/* ===================== 共通: 初期設定モード(モード0) ===================== */

/*
 * センサーの状態をモード0(予約: 初期設定用)として送信する。
 * 起動直後の初期化完了通知や、エラー状態の通知に使う。
 *   status: 0=初期化中, 1=初期化完了, 負値=エラーコード
 */
void sensor_report_status(lump_sensor_type_t type, uint8_t instance_id, int16_t status);

#ifdef __cplusplus
}
#endif