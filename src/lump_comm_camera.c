#include <stdio.h>
#include <stdlib.h>
#include "lump_comm_camera.h"
#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"
#include "lump_comm_calib.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_manager.h"

/*
 * カメラのモード定義。
 * 0: 予約(初期設定用)
 * 1〜31: 後で追加するモード用に空けておく
 */
typedef enum {
    CAMERA_MODE_SYSTEM      = 0,
    CAMERA_MODE_POS_COLOR   = 1,
    CAMERA_MODE_12POS_COLOR = 2,
} camera_mode_t;

typedef enum {
    HORIZONTAL = 1,
    VERTICAL = -1,
} direction_t;

typedef struct {
    lump_color_id_t color_id;
    int16_t y, u, v;
} camera_sensor_buffer_t;
static camera_sensor_buffer_t s_camera_buffer[LUMP_MAX_INSTANCES_PER_TYPE];

/* インスタンスごとの、未処理のキャリブレーション要求 */

static lump_camera_color_calib_request_t s_color_calib_request;
static lump_camera_pos_calib_request_t s_pos_calib_request;
static lump_camera_sys_mode_t s_calib_mode = LUMP_CAMERA_SYS_MODE_SYSTEM;

lump_color_id_t position_colors[LUMP_MAX_INSTANCES_PER_TYPE][12] = {0};
static bool s_camera_instance_active[LUMP_MAX_INSTANCES_PER_TYPE];

static lump_camera_read_pos_color_fn_t   s_read_pos_color_fn   = NULL;
static lump_camera_read_12pos_color_fn_t s_read_12pos_color_fn = NULL;
static lump_camera_update_color_ref_t    s_update_color_ref_fn = NULL;

void lump_camera_register_read_pos_color_fn(lump_camera_read_pos_color_fn_t fn)
{
    s_read_pos_color_fn = fn;
}

void lump_camera_poll_read_pos_color(uint8_t instance_id, int16_t x, int16_t y, uint8_t radius,
                                     lump_camera_sensor_data_t *out_data)
{
    if (s_read_pos_color_fn == NULL) return;

    if (s_read_pos_color_fn(instance_id, x, y, radius, &out_data->y, &out_data->u, &out_data->v, &out_data->color_id))
    {
        lump_camera_set_color(instance_id, out_data->y, out_data->u, out_data->v, out_data->color_id);
    }
}

void lump_camera_register_read_12pos_color_fn(lump_camera_read_12pos_color_fn_t fn)
{
    s_read_12pos_color_fn = fn;
}

void lump_camera_poll_read_12pos_color(uint8_t instance_id, const lump_camera_pos_t *pos, uint8_t radius,
                                       lump_color_id_t *color_ids)
{
    if (s_read_12pos_color_fn == NULL) return;
    if (pos == NULL) return;

    if (s_read_12pos_color_fn(instance_id, radius, pos, color_ids)) 
        lump_camera_set_12pos_color(instance_id, color_ids);
}

void lump_camera_register_update_color_ref_fn(lump_camera_update_color_ref_t fn)
{
    s_update_color_ref_fn = fn;
}

void lump_camera_poll_update_color_ref(uint8_t instance_id)
{
    if (s_update_color_ref_fn == NULL) return;
    s_update_color_ref_fn(instance_id);
}

static void on_camera_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4);

void lump_camera_init(void) {
    s_color_calib_request.is_request = false;
    s_pos_calib_request.is_request = false;
    lump_command_dispatch_register(LUMP_SENSOR_CAMERA, on_camera_command);
}

void lump_camera_instance_active(uint8_t instance_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    s_camera_instance_active[instance_id] = true;
}

bool lump_camera_is_instance_active(uint8_t instance_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return false;
    return s_camera_instance_active[instance_id];
}

void lump_camera_set_color(uint8_t instance_id, int16_t y, int16_t u, int16_t v, lump_color_id_t colorID)
{
    s_camera_buffer[instance_id].y = y;
    s_camera_buffer[instance_id].u = u;
    s_camera_buffer[instance_id].v = v;
    s_camera_buffer[instance_id].color_id = colorID;
}

void lump_camera_report_color(uint8_t instance_id) 
{
    if (!lump_camera_is_instance_active(instance_id)) return;
    lump_device_report(LUMP_SENSOR_CAMERA, CAMERA_MODE_POS_COLOR, instance_id, 
                        s_camera_buffer[instance_id].y, s_camera_buffer[instance_id].u, 
                        s_camera_buffer[instance_id].v, s_camera_buffer[instance_id].color_id);
}

void lump_camera_set_12pos_color(uint8_t instance_id, lump_color_id_t color_lists[12])
{
    for (int i = 0; i < 12; i++)
    {
        position_colors[instance_id][i] = color_lists[i];
    }
}

static uint16_t pack_nibbles(const lump_color_id_t in[4])
{
    return ((uint16_t)(in[0] & 0x0F) << 12) |
           ((uint16_t)(in[1] & 0x0F) <<  8) |
           ((uint16_t)(in[2] & 0x0F) <<  4) |
           ((uint16_t)(in[3] & 0x0F));
}

void lump_camera_report_12pos_color(uint8_t instance_id, uint8_t data_list)
{
    if (!lump_camera_is_instance_active(instance_id)) return;
    uint16_t buf[3];
    for (int i = 0; i < 3; i++)
    {
        buf[i] = pack_nibbles(&position_colors[instance_id][i * 4]);
    }
    lump_device_report(LUMP_SENSOR_CAMERA, CAMERA_MODE_12POS_COLOR, instance_id, data_list, 
                        (int16_t)buf[0], (int16_t)buf[1], (int16_t)buf[2]);
}

static void unpack_poslist_and_pos(const int16_t value, uint8_t *list, uint8_t *position)
{
    *list = (uint8_t)((value >> 8) & 0xFF);
    *position  = (uint8_t)(value & 0xFF);
}

static void camera_systems(uint8_t instance_id, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    switch (v1)
    {
    case LUMP_SENSOR_INIT:
        s_camera_instance_active[instance_id] = (v2 != 0);
        break;
    case LUMP_CAMERA_SYS_MODE_NOT_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_NOT;
        break;
    case LUMP_CAMERA_SYS_MODE_COLOR_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_CAMERA;
        s_calib_mode = LUMP_CAMERA_SYS_MODE_COLOR_CALIB;
        unpack_poslist_and_pos(v2, &s_color_calib_request.pos_list, &s_color_calib_request.pos);
        s_color_calib_request.instance_id = instance_id;
        s_color_calib_request.color_id    = (lump_color_id_t)v3;
        s_color_calib_request.save_value  = (v4 != 0);
        s_color_calib_request.is_request  = true;
        break;
    case LUMP_CAMERA_SYS_MODE_POS_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_CAMERA;
        s_calib_mode = LUMP_CAMERA_SYS_MODE_POS_CALIB;
        unpack_poslist_and_pos(v2, &s_pos_calib_request.pos_list, &s_pos_calib_request.pos);
        s_pos_calib_request.instance_id = instance_id;
        if (v4 == HORIZONTAL)
        {
            s_pos_calib_request.dx = v3;
            s_pos_calib_request.dy = 0;
        }
        else 
        {
            s_pos_calib_request.dx = 0;
            s_pos_calib_request.dy = v3;
        }
        s_pos_calib_request.is_request = true;
        break;
    default:
        break;
    }
}

static void on_camera_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    switch (command)
    {
    case CAMERA_MODE_POS_COLOR:
        lump_camera_sensor_data_t out;
        lump_camera_poll_read_pos_color(instance_id, v1, v2, v3, &out);
        lump_camera_report_color(instance_id);
        break;
    case CAMERA_MODE_12POS_COLOR:
        lump_camera_pos_t pos[12];
        nvs_manager_pos_list_t nvs_pos;
        esp_err_t err = nvs_manager_read_pos_list(instance_id, v1, &nvs_pos);
        if (err == ESP_OK)
        {
            for (int i = 0; i < 12; i++)
            {
                pos[i].x = nvs_pos.pos[i].x;
                pos[i].y = nvs_pos.pos[i].y;
            }
            lump_color_id_t color_list[12];
            lump_camera_poll_read_12pos_color(instance_id, pos, v2, color_list);
        }
        else
        {
            lump_color_id_t color_list[12] = {LUMP_COLOR_UNKNOWN};
            lump_camera_set_12pos_color(instance_id, color_list);
        }

        lump_camera_report_12pos_color(instance_id, v1);
        break;
    case CAMERA_MODE_SYSTEM:
        camera_systems(instance_id, v1, v2, v3, v4);
        break;
    default:
        break;
    }
}

lump_camera_sys_mode_t lump_camera_get_calib_mode()
{
    return s_calib_mode;
}

bool lump_camera_get_color_calib_request(lump_camera_color_calib_request_t *req_data)
{
    if (!s_color_calib_request.is_request) return false;

    req_data->is_request  = s_color_calib_request.is_request;
    req_data->color_id    = s_color_calib_request.color_id;
    req_data->instance_id = s_color_calib_request.instance_id;
    req_data->pos         = s_color_calib_request.pos;
    req_data->pos_list    = s_color_calib_request.pos_list;
    req_data->save_value  = s_color_calib_request.save_value;

    s_color_calib_request.is_request = false;
    return true;
}

bool lump_camera_get_pos_calib_request(lump_camera_pos_calib_request_t *req_data)
{
    if (!s_pos_calib_request.is_request) return false;
    
    req_data->is_request  = s_pos_calib_request.is_request;
    req_data->instance_id = s_pos_calib_request.instance_id;
    req_data->pos         = s_pos_calib_request.pos;
    req_data->pos_list    = s_pos_calib_request.pos_list;
    req_data->dx          = s_pos_calib_request.dx;
    req_data->dy          = s_pos_calib_request.dy;

    s_pos_calib_request.is_request = false;
    return true;
}