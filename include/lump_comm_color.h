#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"
#include "lump_comm_sensors.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool is_request;
    uint8_t instance_id;
    lump_color_id_t color_id;
    bool save_value;
} lump_color_calib_request_t;

typedef enum {
    LUMP_COLOR_SYS_MODE_SYSTEM      = 0,
    LUMP_COLOR_SYS_MODE_NOT_CALIB   = 1,
    LUMP_COLOR_SYS_MODE_COLOR_CALIB = 2,
} lump_color_sys_mode_t;

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
 * センサードライバ側で実装する「値取得」関数の型。
 * この関数の中で実際のI2C通信等を行い、取得した値を引数経由で返す。
 * 戻り値: 取得に成功したら true。false ならバッファは更新されない。
 */
typedef bool (*lump_color_read_color_fn_t)(uint8_t instance_id,
                                           uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c,
                                           lump_color_id_t *color_id);

/*
 * センサー値取得関数を登録する。
 * 既に登録済みの場合は上書きされる。NULLを渡すと未登録状態に戻る
 * (=このコンポーネント単体ではセンサー値が更新されなくなる)。
 *
 * 別仕様のセンサーに差し替えたい場合は、コンポーネント外から
 * このAPIを呼び直すだけでよい。
 */
void lump_color_register_read_color_fn(lump_color_read_color_fn_t fn);

void lump_color_poll_read_color(void);


typedef void (*lump_color_update_color_ref_t)(uint8_t);

void lump_color_register_update_color_ref_fn(lump_color_update_color_ref_t fn);

void lump_color_poll_update_color_ref(uint8_t instance_id);


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
bool lump_color_is_instance_active(uint8_t instance_id);

/*
 * センサードライバから、読み取った値をバッファへ書き込む。
 * この時点ではまだ送信されない(reportを呼ぶまで保持される)。
 */
void lump_color_set_color_id(uint8_t instance_id, lump_color_id_t color_id);
void lump_color_set_rgbc(uint8_t instance_id, int16_t r, int16_t g, int16_t b, int16_t c);

void lump_color_get_rgbc_buffer(uint8_t instance_id, int16_t *r, int16_t *g, int16_t *b, int16_t *c);

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
bool lump_color_get_calib_request(lump_color_calib_request_t *req_data);

lump_color_sys_mode_t lump_color_get_calib_mode();

#ifdef __cplusplus
}
#endif