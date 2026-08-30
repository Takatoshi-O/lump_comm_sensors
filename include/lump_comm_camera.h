#pragma once
/**
 * @file lump_comm_camera.h
 * @brief LUMP通信上でカメラセンサーの値・色・キャリブレーション要求を扱うセンサー別APIを定義します。
 */

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

/** @brief LUMP上でカメラセンサーに割り当てるセンサー種別です。 */
#define LUMP_SENSOR_CAMERA LUMP_TYPE_2

/**
 * @brief カメラから取得した1点分のYUVと色IDを保持します。
 */
typedef struct {
    lump_color_id_t color_id;
    int16_t y, u, v;
} lump_camera_sensor_data_t;

/**
 * @brief カメラ色キャリブレーション要求の状態を保持します。
 */
typedef struct {
    bool is_request;
    uint8_t instance_id;
    uint8_t pos_list, pos;
    lump_color_id_t color_id;
    bool save_value;
} lump_camera_color_calib_request_t;


/**
 * @brief カメラ位置キャリブレーション要求の状態を保持します。
 */
typedef struct {
    bool is_request;
    uint8_t instance_id;
    uint8_t pos_list, pos;
    int16_t dx, dy;
} lump_camera_pos_calib_request_t;

/**
 * @brief カメラ側キャリブレーションに関するシステムモードです。
 */
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
/**
 * @brief 指定座標周辺のカメラYUVと色IDを取得するコールバック型です。
 *
 * センサードライバ側が実処理を提供し、LUMP層はこの登録関数を介して利用します。
 */
typedef bool (*lump_camera_read_pos_color_fn_t)(uint8_t instance_id,
                                                int16_t x, int16_t y, uint8_t radius,
                                                int16_t *out_y, int16_t *out_u, int16_t *out_v,
                                                lump_color_id_t *out_color_id);

/**
 * @brief 単一点の色取得コールバックを登録します。
 *
 * @param fn 登録するコールバックです。
 */
void lump_camera_register_read_pos_color_fn(lump_camera_read_pos_color_fn_t fn);

/**
 * @brief 登録済みコールバックを使って指定位置の色情報を取得します。
 *
 * @param instance_id カメラインスタンスIDです。
 * @param x X座標です。
 * @param y Y座標です。
 * @param radius 平均取得半径です。
 * @param out_data 結果の格納先です。
 */
void lump_camera_poll_read_pos_color(uint8_t instance_id, int16_t x, int16_t y, uint8_t radius,
                                     lump_camera_sensor_data_t *out_data);

/** @brief カメラ位置キャリブレーション等で使用する2次元座標です。 */
typedef struct {
    int16_t x, y;
} lump_camera_pos_t;

/*
 * 12点分の色を一括取得する関数の型。
 * positions: 取得対象となる12点の座標(呼び出し元のコンポーネントが用意する)。
 * out_colors: 取得結果を書き込む配列(12点分)。
 */
/** @brief 12点分のカメラ座標から色IDをまとめて取得するコールバック型です。 */
typedef bool (*lump_camera_read_12pos_color_fn_t)(uint8_t instance_id, uint8_t radius,
                                                  const lump_camera_pos_t positions[12],
                                                  lump_color_id_t out_colors[12]);

/**
 * @brief 12点一括色取得コールバックを登録します。
 *
 * @param fn 登録するコールバックです。
 */
void lump_camera_register_read_12pos_color_fn(lump_camera_read_12pos_color_fn_t fn);

/**
 * @brief 登録済みコールバックを使って12点の色IDを取得します。
 *
 * @param instance_id カメラインスタンスIDです。
 * @param pos 12点の座標配列です。
 * @param radius 色取得時の半径です。
 * @param color_ids 色IDの出力配列です。
 */
void lump_camera_poll_read_12pos_color(uint8_t instance_id, const lump_camera_pos_t *pos, uint8_t radius,
                                       lump_color_id_t *color_ids);

/** @brief カメラ色基準値更新処理を呼び出すコールバック型です。 */
typedef void (*lump_camera_update_color_ref_t)(uint8_t);

/** @brief カメラ色基準値更新コールバックを登録します。 */
void lump_camera_register_update_color_ref_fn(lump_camera_update_color_ref_t fn);

/** @brief 登録済みの色基準値更新コールバックを実行します。 */
void lump_camera_poll_update_color_ref(uint8_t instance_id);

/** @brief カメラセンサーAPIとLUMPコマンド処理を初期化します。 */
void lump_camera_init(void);

/** @brief 指定カメラインスタンスをアクティブとして登録します。 */
void lump_camera_instance_active(uint8_t instance_id);

/** @brief 指定カメラインスタンスがアクティブか確認します。 */
bool lump_camera_is_instance_active(uint8_t instance_id);

/**
 * @brief 指定カメラの最新YUV値と色IDを送信用バッファへ設定します。
 */
void lump_camera_set_color(uint8_t instance_id, int16_t y, int16_t u, int16_t v, lump_color_id_t colorID);

/** @brief 指定カメラの色結果をLUMPへ送信します。 */
void lump_camera_report_color(uint8_t instance_id);

/** @brief 指定カメラの12点分の色IDを送信用バッファへ設定します。 */
void lump_camera_set_12pos_color(uint8_t instance_id, lump_color_id_t color_lists[12]);

/** @brief 指定カメラの12点色データから指定ブロックをLUMPへ送信します。 */
void lump_camera_report_12pos_color(uint8_t instance_id, uint8_t data_list);

/** @brief カメラの現在のキャリブレーションシステムモードを取得します。 */
lump_camera_sys_mode_t lump_camera_get_calib_mode();

/**
 * @brief 未処理のカメラ色キャリブレーション要求を取得します。
 *
 * @param req_data 要求内容の格納先です。
 * @return 要求が存在した場合trueです。取得すると要求は消費されます。
 */
bool lump_camera_get_color_calib_request(lump_camera_color_calib_request_t *req_data);

/**
 * @brief 未処理のカメラ位置キャリブレーション要求を取得します。
 *
 * @param req_data 要求内容の格納先です。
 * @return 要求が存在した場合trueです。取得すると要求は消費されます。
 */
bool lump_camera_get_pos_calib_request(lump_camera_pos_calib_request_t *req_data);

#ifdef __cplusplus
}
#endif