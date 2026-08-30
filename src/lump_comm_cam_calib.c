#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lump_comm_camera.h"
#include "lump_comm_sensors.h"
#include "lump_comm_calib.h"

#include "lcd_lvgl.h"
#include "lcd_lvgl_ui.h"
#include "lcd_lvgl_ui_cfg.h"

#include "nvs_manager.h"

nvs_manager_pos_list_t pos_lists;
uint8_t instance_id = -1;
uint8_t pos_list_number = 0;
uint8_t pos_number = 0;

static void camera_color_calib()
{
    lump_camera_color_calib_request_t req;
    if (lump_camera_get_color_calib_request(&req))
    {
        if (!lump_camera_is_instance_active(req.instance_id))
        {
            lcd_ui_text_display(LCD_UI_CAMERA_CALIB_COLOR, 
                            LCD_UI_CAMERA_CALIB_COLOR_TEXT_CAMERA_EXIST, true);
            lcd_ui_set(LCD_UI_CAMERA_CALIB_COLOR);
            lcd_ui_display(LCD_UI_DISPLAY_POS, false);
            return;
        }
        lcd_ui_text_display(LCD_UI_CAMERA_CALIB_COLOR, 
                            LCD_UI_CAMERA_CALIB_COLOR_TEXT_CAMERA_EXIST, false);

        if (req.save_value)
        {
            int16_t x = pos_lists.pos[pos_number].x;
            int16_t y = pos_lists.pos[pos_number].y;
            lump_camera_sensor_data_t data;
            lump_camera_poll_read_pos_color(req.instance_id, x, y, 1, &data);

            nvs_manager_cam_yuv_t save_data;
            nvs_manager_read_cam_yuv(req.instance_id, &save_data);
            save_data.color[req.color_id].y = data.y;
            save_data.color[req.color_id].u = data.u;
            save_data.color[req.color_id].v = data.v;
            nvs_manager_write_cam_yuv(req.instance_id, &save_data);
            
            lump_camera_poll_update_color_ref(req.instance_id);
        }

        if (instance_id != req.instance_id || pos_list_number != req.pos_list)
        {
            nvs_manager_read_pos_list(req.instance_id, req.pos_list, &pos_lists);
            
            instance_id = req.instance_id;
            pos_list_number = req.pos_list;
            pos_number = req.pos;
            for (int i = 0; i < NVS_MANAGER_POS_LIST_COUNT; i++)
            {
                if (i >= MAX_RECT_COUNT) break;
                lcd_lvgl_set_rect(i, pos_lists.pos[i].x, pos_lists.pos[i].y, DISPLAY_COLOR_BLUE);
            }
        }
        else if (pos_number != req.pos)
        {
            lcd_lvgl_set_rect(pos_number, pos_lists.pos[pos_number].x, pos_lists.pos[pos_number].y, DISPLAY_COLOR_BLUE);
            pos_number = req.pos;
        }

        lcd_lvgl_set_rect(req.pos, pos_lists.pos[req.pos].x, pos_lists.pos[req.pos].y, DISPLAY_COLOR_RED);

        char buf[4]; 

        snprintf(buf, sizeof(buf), "%u", req.pos_list);
        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_COLOR, LCD_UI_CAMERA_CALIB_COLOR_TEXT_LIST_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);
        
        snprintf(buf, sizeof(buf), "%u", req.pos);
        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_COLOR, LCD_UI_CAMERA_CALIB_COLOR_TEXT_POS_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);

        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_COLOR, LCD_UI_CAMERA_CALIB_COLOR_TEXT_COLOR_VALUE, 
                        lump_color_id_to_char(req.color_id), DISPLAY_COLOR_WHITE);

        lcd_ui_set(LCD_UI_CAMERA_CALIB_COLOR);
        lcd_ui_display(LCD_UI_DISPLAY_POS, true);
    }
}

static void camera_pos_calib()
{
    lump_camera_pos_calib_request_t req;
    if (lump_camera_get_pos_calib_request(&req))
    {
        if (!lump_camera_is_instance_active(req.instance_id))
        {
            lcd_ui_text_display(LCD_UI_CAMERA_CALIB_POS, 
                            LCD_UI_CAMERA_CALIB_POS_TEXT_CAMERA_EXIST, true);
            lcd_ui_set(LCD_UI_CAMERA_CALIB_POS);
            lcd_ui_display(LCD_UI_DISPLAY_POS, false);
            return;
        }
        lcd_ui_text_display(LCD_UI_CAMERA_CALIB_POS, 
                            LCD_UI_CAMERA_CALIB_POS_TEXT_CAMERA_EXIST, false);

        if (instance_id != req.instance_id || pos_list_number != req.pos_list)
        {
            nvs_manager_write_pos_list(instance_id, pos_list_number, &pos_lists);

            nvs_manager_read_pos_list(req.instance_id, req.pos_list, &pos_lists);

            instance_id = req.instance_id;
            pos_list_number = req.pos_list;
            for (int i = 0; i < NVS_MANAGER_POS_LIST_COUNT; i++)
            {
                if (i >= MAX_RECT_COUNT) break;
                lcd_lvgl_set_rect(i, pos_lists.pos[i].x, pos_lists.pos[i].y, DISPLAY_COLOR_BLUE);
            }
        }
        else if (pos_number != req.pos)
        {
            lcd_lvgl_set_rect(pos_number, pos_lists.pos[pos_number].x, pos_lists.pos[pos_number].y, DISPLAY_COLOR_BLUE);
            pos_number = req.pos;
        }

        pos_lists.pos[req.pos].x += req.dx;
        pos_lists.pos[req.pos].y += req.dy;

        if (pos_lists.pos[req.pos].x < 0) pos_lists.pos[req.pos].x = 0;
        if (pos_lists.pos[req.pos].y < 0) pos_lists.pos[req.pos].y = 0;

        lcd_lvgl_set_rect(req.pos, pos_lists.pos[req.pos].x, pos_lists.pos[req.pos].y, DISPLAY_COLOR_RED);

        char buf[4];

        snprintf(buf, sizeof(buf), "%u", req.pos_list);
        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_POS, LCD_UI_CAMERA_CALIB_POS_TEXT_LIST_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);
        
        snprintf(buf, sizeof(buf), "%u", req.pos);
        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_POS, LCD_UI_CAMERA_CALIB_POS_TEXT_POS_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);

        snprintf(buf, sizeof(buf), "%u", pos_lists.pos[req.pos].x);
        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_POS, LCD_UI_CAMERA_CALIB_POS_TEXT_POS_X_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);

        snprintf(buf, sizeof(buf), "%u", pos_lists.pos[req.pos].y);
        lcd_ui_text_set(LCD_UI_CAMERA_CALIB_POS, LCD_UI_CAMERA_CALIB_POS_TEXT_POS_Y_VALUE, 
                        buf, DISPLAY_COLOR_WHITE);

        lcd_ui_set(LCD_UI_CAMERA_CALIB_POS);
        lcd_ui_display(LCD_UI_DISPLAY_POS, true);
    }
}

static void cam_calib_task(void *arg)
{
    lump_camera_sys_mode_t mode = LUMP_CAMERA_SYS_MODE_SYSTEM;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        
        if (now_calib_sensor != LUMP_CALIB_SENSOR_CAMERA) continue;

        if (mode != lump_camera_get_calib_mode() && mode == LUMP_CAMERA_SYS_MODE_POS_CALIB)
        {
            nvs_manager_write_pos_list(instance_id, pos_list_number, &pos_lists);
        }
        
        mode = lump_camera_get_calib_mode();
        switch (mode)
        {
        case LUMP_CAMERA_SYS_MODE_COLOR_CALIB:
            camera_color_calib();
            break;
        case LUMP_CAMERA_SYS_MODE_POS_CALIB:
            camera_pos_calib();
            break;
        default:
            break;
        }
    }
}

void lump_cam_calib_start() 
{
    xTaskCreate(cam_calib_task, "cam_calib", 4096, NULL, 8, NULL);
}