#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lump_comm_sensors_cfg.h"

#include "lump_comm_calib.h"

#include "lump_command.h"
#include "lump_command_dispatch.h"
#include "lump_comm_sensors.h"

#include "lcd_lvgl.h"
#include "lcd_lvgl_ui.h"
#include "lcd_lvgl_ui_cfg.h"

#include "nvs_manager.h"

#ifdef CONFIG_CAMERA_AVAILABLE
#include "camera_manager.h"
#endif

lump_calib_sensor_t now_calib_sensor = LUMP_CALIB_SENSOR_NOT;

static void lump_calib_task(void *arg)
{
    lump_calib_sensor_t last_calib_sensor = LUMP_CALIB_SENSOR_NOT;
    lcd_ui_set(LCD_UI_INFO);
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        
        if (now_calib_sensor == last_calib_sensor) continue;
        switch (now_calib_sensor)
        {
        case LUMP_CALIB_SENSOR_CAMERA:
            lcd_ui_display(LCD_UI_COLOR_RGBC, false);
            break;
        case LUMP_CALIB_SENSOR_COLOR:
            lcd_ui_display(LCD_UI_DISPLAY_POS, false);
            break;
        case LUMP_CALIB_SENSOR_NOT:
            lcd_ui_display(LCD_UI_COLOR_RGBC, false);
            lcd_ui_display(LCD_UI_DISPLAY_POS, false);
            lcd_ui_set(LCD_UI_INFO);
            break;
        default:
            break;
        }
        last_calib_sensor = now_calib_sensor;

    }
}

typedef enum
{
    SYS_NOT_CALIB         = 1,
    SYS_CAM_COLOR_CALIB   = 2,
    SYS_CAM_POS_CALIB     = 3,
    SYS_COLOR_COLOR_CALIB = 4,
    SYS_CALIB_MODE_MAX,
} sys_calib_mode_t;

static void on_calib_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    switch (v1)
    {
    case SYS_NOT_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_NOT;
        lcd_ui_set(LCD_UI_INFO);
        break;
    case SYS_CAM_COLOR_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_CAMERA;
        lcd_ui_set(LCD_UI_CAMERA_CALIB_COLOR);
        break;
    case SYS_CAM_POS_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_CAMERA;
        lcd_ui_set(LCD_UI_CAMERA_CALIB_POS);
        break;
    case SYS_COLOR_COLOR_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_COLOR;
        lcd_ui_set(LCD_UI_COLOR_CALIB_COLOR);
        break;
    default:
        now_calib_sensor = LUMP_CALIB_SENSOR_NOT;
        lcd_ui_set(LCD_UI_INFO);
        break;
    }
}


void lump_calib_start() 
{
    nvs_manager_init();
    ESP_ERROR_CHECK(lcd_lvgl_init());
    ESP_ERROR_CHECK(lcd_lvgl_start());
    #ifdef CONFIG_CAMERA_AVAILABLE
        nvs_manager_pos_list_t pos_list;
        int width, height;
        camera_get_frame_size(&width, &height);
        for (int i = 0; i < MAX_RECT_COUNT; i++)
        {
            pos_list.pos[i].x = width / 2;
            pos_list.pos[i].y = height / 2;
        }
        for (int i = 0; i < LUMP_MAX_INSTANCES_PER_TYPE; i++)
        {
            nvs_manager_write_pos_list(i, 0, &pos_list);
        }
    #endif

    lump_command_dispatch_register(LUMP_SYSTEM, on_calib_command);
    xTaskCreate(lump_calib_task, "lump_calib", 4096, NULL, 8, NULL);
    if (COLOR_CALIB_ENABLE) lump_color_calib_start();
    if (CAMERA_CALIB_ENABLE) lump_cam_calib_start();
}
