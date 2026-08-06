#include "lump_comm_sensors.h"

/* ===================== 内部ヘルパー ===================== */

/*
 * モード番号の使用禁止チェック。
 * 0=初期設定予約のためアサートで弾く。
 * このチェックを通過したモードのみ lump_device_report() へ渡す。
 */
bool is_valid_mode(uint8_t mode) 
{
    return mode != 0;
}

/* ===================== 共通: 初期設定モード(モード0) ===================== */

void sensor_report_status(lump_sensor_type_t type, uint8_t instance_id, int16_t status) 
{
    /* モード0は予約済み。ステータス通知専用としてここからのみ送信する */
    lump_device_report(type, 0, instance_id, status, 0, 0, 0);
}