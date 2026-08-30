#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"
#include "lump_comm_color.h"
#include "lump_comm_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "camera_yuv_query.h"
#include "camera_color.h"



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
    xTaskCreate(lump_sensors_task, "lump_sensors", 4096, NULL, 8, NULL);
}

const char *lump_color_id_to_char(lump_color_id_t color)
{
    switch (color) 
    {
    case LUMP_COLOR_BLACK:   return "BLACK";
    case LUMP_COLOR_WHITE:   return "WHITE";
    case LUMP_COLOR_RED:     return "RED";
    case LUMP_COLOR_GREEN:   return "GREEN";
    case LUMP_COLOR_BLUE:    return "BLUE";
    case LUMP_COLOR_YELLOW:  return "YELLOW";
    case LUMP_COLOR_ORANGE:  return "ORANGE";
    case LUMP_COLOR_PURPLE:  return "PURPLE";
    case LUMP_COLOR_CYAN:    return "CYAN";
    case LUMP_COLOR_MAGENTA: return "MAGENTA";
    case LUMP_COLOR_BROWN:   return "BROWN";
    case LUMP_COLOR_GRAY:    return "GRAY";
    case LUMP_COLOR_PINK:    return "PINK";
    case LUMP_COLOR_LIME:    return "LIME";
    case LUMP_COLOR_NAVY:    return "NAVY";
    case LUMP_COLOR_UNKNOWN: 
    default:                 return "?";
    }
}

