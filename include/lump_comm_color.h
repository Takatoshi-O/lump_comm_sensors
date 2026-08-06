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

/* ===================== カラーセンサー ===================== */

#define LUMP_SENSOR_COLOR LUMP_TYPE_1

/*
 * このライブラリを使う前に1回呼ぶこと。
 * SPIKEからのインスタンス登録コマンド(mode=0)を受け取れるよう、
 * 内部で lump_command_dispatch_register() を行う。
 */
void lump_color_init(void);

/*
 * 指定したインスタンスIDが、SPIKE側から「使用する」と登録済みかどうか。
 * (SPIKEが起動時に mode=0 のコマンドで、各インスタンスIDの有効/無効を
 *  送ってくることを想定している)
 */
bool color_sensor_is_instance_active(uint8_t instance_id);

/*
 * センサードライバから、読み取った値をバッファへ書き込む。
 * この時点ではまだ送信されない(reportを呼ぶまで保持される)。
 */
void lump_color_set_color_id(uint8_t instance_id, int16_t color_id);
void lump_color_set_rgbc(uint8_t instance_id, int16_t r, int16_t g, int16_t b, int16_t c);

/*
 * モード1: RGBC値を送信する。
 */
void lump_color_report_rgbc(uint8_t instance_id);

/*
 * モード2: 検出した色のIDを送信する。
 * color_id: アプリで定義した色の識別番号(例: 0=なし, 1=赤, 2=青, ...)
 */
void lump_color_report_color_id(uint8_t instance_id);

/*
 * キャリブレーション要求の取得。
 *
 * このAPIを呼んだ側(センサードライバ等)は、要求があれば現在の
 * RGBC値を読み取り、その色のキャリブレーションデータとして保存する
 * 処理を行うこと。
 *
 * out_color_id: 要求があった場合、キャリブレーション対象の色IDが書き込まれる。
 * 戻り値: 未処理の要求があれば true(呼ぶと消費され、以後は false になる)。
 */
bool lump_color_get_calib_request(uint8_t instance_id, int16_t *out_color_id);

/*
 * キャリブレーション要求の確認
 */
bool lump_color_is_calib_request();

/*
 * キャリブレーション終了時に呼び出す
 */
void lump_color_calib_fin();

/*
 * Color IDをmaskに変換 
 */
bool color_id_to_mask(uint8_t color_id, uint16_t *mask);

/*
 * maskに含まれるColor IDを全て出力
 */
uint8_t mask_to_color_ids(uint16_t mask, uint8_t *colors);

/*
 * maskに指定したColor IDが含まれるかを出力
 */
bool watch_contains_color(uint16_t mask, uint8_t color_id);

#ifdef __cplusplus
}
#endif