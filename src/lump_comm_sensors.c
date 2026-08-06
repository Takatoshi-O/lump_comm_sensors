#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"
#include "lump_comm_color.h"
#include "lump_comm_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

static void lump_sensors_task(void *arg) {
    while (1) {
        if (lump_device_is_connected()) {
            lump_command_dispatch_poll();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void lump_sensors_start(void) {
    lump_camera_init();
    lump_color_init();
    xTaskCreate(lump_sensors_task, "lump_sensors", 4096, NULL, 8, NULL);
}