#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lump_comm.h"
#include "lump_comm_sensors.h"

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

typedef struct {
    lump_color_id_t color_id;
    int16_t y, u, v;
} lump_camera_sensor_data_t;

typedef struct {
    bool is_request;
    uint8_t instance_id;
    uint8_t pos_list, pos;
    lump_color_id_t color_id;
    bool save_value;
} lump_camera_color_calib_request_t;


typedef struct {
    bool is_request;
    uint8_t instance_id;
    uint8_t pos_list, pos;
    int16_t dx, dy;
} lump_camera_pos_calib_request_t;

typedef enum {
    LUMP_CAMERA_SYS_MODE_SYSTEM      = 0,
    LUMP_CAMERA_SYS_MODE_NOT_CALIB   = 1,
    LUMP_CAMERA_SYS_MODE_COLOR_CALIB = 2,
    LUMP_CAMERA_SYS_MODE_POS_CALIB   = 3,
} lump_camera_sys_mode_t;

/*
 * カメラの汎用送信API。将来のモード追加時に、このAPIをラップする形で
 * camera_report_position() 等の専用関数を追加していく想定。
 * mode: camera_mode_t の値を使うこと(0は使用禁止)
 */

/*
 * 指定座標(x, y)周辺 radius の色を取得する関数の型。
 * センサードライバ側で実装し、取得結果を引数経由で返す。
 * 戻り値: 取得に成功したら true。
 */
typedef bool (*lump_camera_read_pos_color_fn_t)(uint8_t instance_id,
                                                int16_t x, int16_t y, uint8_t radius,
                                                int16_t *out_y, int16_t *out_u, int16_t *out_v,
                                                lump_color_id_t *out_color_id);

void lump_camera_register_read_pos_color_fn(lump_camera_read_pos_color_fn_t fn);

void lump_camera_poll_read_pos_color(uint8_t instance_id, int16_t x, int16_t y, uint8_t radius,
                                     lump_camera_sensor_data_t *out_data);

typedef struct {
    int16_t x, y;
} lump_camera_pos_t;

/*
 * 12点分の色を一括取得する関数の型。
 * positions: 取得対象となる12点の座標(呼び出し元のコンポーネントが用意する)。
 * out_colors: 取得結果を書き込む配列(12点分)。
 */
typedef bool (*lump_camera_read_12pos_color_fn_t)(uint8_t instance_id, uint8_t radius,
                                                  const lump_camera_pos_t positions[12],
                                                  lump_color_id_t out_colors[12]);

void lump_camera_register_read_12pos_color_fn(lump_camera_read_12pos_color_fn_t fn);

void lump_camera_poll_read_12pos_color(uint8_t instance_id, const lump_camera_pos_t *pos, uint8_t radius,
                                       lump_color_id_t *color_ids);

typedef void (*lump_camera_update_color_ref_t)(uint8_t);

void lump_camera_register_update_color_ref_fn(lump_camera_update_color_ref_t fn);

void lump_camera_poll_update_color_ref(uint8_t instance_id);

void lump_camera_init(void);

void lump_camera_instance_active(uint8_t instance_id);

bool lump_camera_is_instance_active(uint8_t instance_id);

void lump_camera_set_color(uint8_t instance_id, int16_t y, int16_t u, int16_t v, lump_color_id_t colorID);

void lump_camera_report_color(uint8_t instance_id);

void lump_camera_set_12pos_color(uint8_t instance_id, lump_color_id_t color_lists[12]);

void lump_camera_report_12pos_color(uint8_t instance_id, uint8_t data_list);

lump_camera_sys_mode_t lump_camera_get_calib_mode();

bool lump_camera_get_color_calib_request(lump_camera_color_calib_request_t *req_data);

bool lump_camera_get_pos_calib_request(lump_camera_pos_calib_request_t *req_data);

#ifdef __cplusplus
}
#endif