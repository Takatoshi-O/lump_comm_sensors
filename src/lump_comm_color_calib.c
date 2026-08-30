#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lump_comm_color.h"
#include "lump_comm_sensors.h"
#include "lump_comm_calib.h"

#include "lcd_lvgl.h"
#include "lcd_lvgl_ui.h"
#include "lcd_lvgl_ui_cfg.h"

#include "nvs_manager.h"


static void color_color_calib()
{
    lump_color_calib_request_t req;
    if (lump_color_get_calib_request(&req))
    {
        char buf[4];

        snprintf(buf, sizeof(buf), "%u", req.instance_id);
        lcd_ui_text_set(LCD_UI_COLOR_CALIB_COLOR, LCD_UI_COLOR_CALIB_COLOR_TEXT_ID_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);

        if (!lump_color_is_instance_active(req.instance_id))
        {
            lcd_ui_display(LCD_UI_COLOR_RGBC, false);
            lcd_ui_text_display(LCD_UI_COLOR_CALIB_COLOR, 
                                LCD_UI_COLOR_CALIB_COLOR_TEXT_SENSOR_EXIST, true);
            return;
        }
        lcd_ui_text_display(LCD_UI_COLOR_CALIB_COLOR, 
                            LCD_UI_COLOR_CALIB_COLOR_TEXT_SENSOR_EXIST, false);
        lcd_ui_display(LCD_UI_COLOR_RGBC, true);

        int16_t r,g,b,c;
        //lump_color_poll_read_color();
        //lump_color_get_rgbc_buffer(req.instance_id, &r, &g, &b, &c);
        r = 100; g = 100; b = 100; c = 100;

        if (req.save_value)
        {
            nvs_manager_color_rgbc_t save_data;
            nvs_manager_read_color_rgbc(req.instance_id, &save_data);
            save_data.color[req.color_id].r = r;
            save_data.color[req.color_id].g = g;
            save_data.color[req.color_id].b = b;
            save_data.color[req.color_id].c = c;
            nvs_manager_write_color_rgbc(req.instance_id, &save_data);

            //lump_color_poll_update_color_ref(req.instance_id);
        }

        lcd_ui_text_set(LCD_UI_COLOR_CALIB_COLOR, LCD_UI_COLOR_CALIB_COLOR_TEXT_COLOR_VALUE, 
                        lump_color_id_to_char(req.color_id), DISPLAY_COLOR_WHITE);

        snprintf(buf, sizeof(buf), "%u", r);
        lcd_ui_text_set(LCD_UI_COLOR_RGBC, LCD_UI_COLOR_RGBC_TEXT_R_VALUE, 
                        buf, DISPLAY_COLOR_RED);

        snprintf(buf, sizeof(buf), "%u", g);
        lcd_ui_text_set(LCD_UI_COLOR_RGBC, LCD_UI_COLOR_RGBC_TEXT_G_VALUE, 
                        buf, DISPLAY_COLOR_GREEN);

        snprintf(buf, sizeof(buf), "%u", b);
        lcd_ui_text_set(LCD_UI_COLOR_RGBC, LCD_UI_COLOR_RGBC_TEXT_B_VALUE, 
                        buf, DISPLAY_COLOR_BLUE);

        snprintf(buf, sizeof(buf), "%u", c);
        lcd_ui_text_set(LCD_UI_COLOR_RGBC, LCD_UI_COLOR_RGBC_TEXT_C_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);

        lcd_ui_set(LCD_UI_COLOR_CALIB_COLOR);
    }
    
}

static void color_calib_task(void *arg)
{
    lump_color_sys_mode_t mode;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        
        if (now_calib_sensor != LUMP_CALIB_SENSOR_COLOR) continue;
        
        mode = lump_color_get_calib_mode();
        switch (mode)
        {
        case LUMP_COLOR_SYS_MODE_COLOR_CALIB:
            color_color_calib();
            break;
        default:
            break;
        }
    }
}

void lump_color_calib_start() 
{
    xTaskCreate(color_calib_task, "color_calib", 4096, NULL, 8, NULL);
}