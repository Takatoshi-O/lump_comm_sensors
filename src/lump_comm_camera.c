#include <stdio.h>
#include <stdlib.h>
#include "lump_comm_camera.h"
#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * カメラのモード定義。
 * 0: 予約(初期設定用)
 * 1〜31: 後で追加するモード用に空けておく
 */
typedef enum {
    CAMERA_MODE_SYSTEM = 0,
    CAMERA_MODE_POS_COLOR = 1,
    CAMERA_MODE_12POS_COLOR = 2,
} camera_mode_t;

typedef struct {
    int16_t color_id;
    int16_t y, u, v;
} camera_sensor_buffer_t;
static camera_sensor_buffer_t s_camera_buffer[LUMP_MAX_INSTANCES_PER_TYPE];


uint8_t position_colors[3][4] = {0};

static request_get_color_t request_get_color;

static request_get_12pos_color_t request_get_12pos_color;

static bool s_camera_instance_active[LUMP_MAX_INSTANCES_PER_TYPE];

static void on_camera_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4);

void lump_camera_init(void) {
    request_get_color.isrequest = false;
    request_get_12pos_color.isrequest = false;
    lump_command_dispatch_register(LUMP_SENSOR_CAMERA, on_camera_command);
}

void camera_sensor_instance_active(uint8_t instance_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    s_camera_instance_active[instance_id] = true;
}

bool camera_sensor_is_instance_active(uint8_t instance_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return false;
    return s_camera_instance_active[instance_id];
}

void lump_camera_report_color(uint8_t instance_id) 
{
    if (!camera_sensor_is_instance_active(instance_id)) return;
    lump_device_report(LUMP_SENSOR_CAMERA, CAMERA_MODE_POS_COLOR, instance_id, 
                        s_camera_buffer[instance_id].y, s_camera_buffer[instance_id].u, 
                        s_camera_buffer[instance_id].v, s_camera_buffer[instance_id].color_id);
}

void lump_camera_set_color(uint8_t instanceID, int16_t y, int16_t u, int16_t v, int16_t colorID)
{
    s_camera_buffer[instanceID].y = y;
    s_camera_buffer[instanceID].u = u;
    s_camera_buffer[instanceID].v = v;
    s_camera_buffer[instanceID].color_id = colorID;
}

void lump_camera_set_12pos_color(uint8_t color_lists[3][4])
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            position_colors[i][j] = color_lists[i][j];
        }
    }
}

static int16_t pack_nibbles(const uint8_t in[4])
{
    uint16_t out;

    out=((uint16_t)in[0] << 12) |
        ((uint16_t)in[1] << 8)  |
        ((uint16_t)in[2] << 4)  |
        ((uint16_t)in[3]);

    return out;
}

static void unpack_poslist_and_pos(const int16_t value, uint8_t out[2])
{
    out[0] = (uint8_t)((value >> 8) & 0xFF);
    out[1]  = (uint8_t)(value & 0xFF);
}

void lump_camera_report_12pos_color(uint8_t instance_id, uint8_t data_list)
{
    uint16_t buf[3];
    for (int i = 0; i < 3; i++)
    {
        buf[i] = pack_nibbles(position_colors[i]);
    }
    lump_device_report(LUMP_SENSOR_CAMERA, CAMERA_MODE_12POS_COLOR, instance_id, data_list, buf[0], buf[1], buf[2]);
}

static void on_camera_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    switch (command)
    {
    case CAMERA_MODE_POS_COLOR:
        request_get_color.instanceID = instance_id;
        request_get_color.x = v1;
        request_get_color.y = v2;
        request_get_color.px_size = (uint8_t)v3;
        request_get_color.isrequest = true;
        while (request_get_color.isrequest) vTaskDelay(pdMS_TO_TICKS(10));
        lump_camera_report_color(instance_id);
        break;
    case CAMERA_MODE_12POS_COLOR:
        request_get_12pos_color.instanceID = instance_id;
        request_get_12pos_color.data_list = v1;
        request_get_12pos_color.px_size = (uint8_t)v2;
        request_get_12pos_color.isrequest = true;
        while (request_get_12pos_color.isrequest) vTaskDelay(pdMS_TO_TICKS(10));
        lump_camera_report_12pos_color(instance_id, request_get_12pos_color.data_list);
        break;
    case CAMERA_MODE_SYSTEM:
        uint8_t pos[2];
        unpack_poslist_and_pos(v2, pos);
        ESP_LOGI("CAM CALIB","ID:%d,MODE:%d,LIST:%d,POS:%d,%d,%d",instance_id,v1,pos[0],pos[1],v3,v4);
        break;
    default:
        break;
    }
}

request_get_color_t lump_camera_is_request_color()
{
    return request_get_color;
}

void lump_camera_request_color_fin()
{
    request_get_color.isrequest = false;
}

request_get_12pos_color_t lump_camera_is_request_12pos_color()
{
    return request_get_12pos_color;
}

void lump_camera_request_12pos_color_fin()
{
    request_get_12pos_color.isrequest = false;
}