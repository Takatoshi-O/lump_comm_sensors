#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "lump_comm_color.h"
#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"
#include "lump_comm_calib.h"

/* 監視色として指定できる色IDの範囲(ビットマスクで管理するための上限) */
#define COLOR_SENSOR_MAX_WATCH_COLOR_ID 16

/*
 * カラーセンサーのモード定義。
 * 0: 初期設定用
 * 1: RGB値
 * 2: 色ID
 */
typedef enum {
    COLOR_MODE_SYSTEM = 0,
    COLOR_MODE_RGBC = 1,
    COLOR_MODE_COLOR_ID = 2,
    COLOR_MODE_NOTIFY_COLOR = 3,
} color_sensor_mode_t;

typedef struct {
    lump_color_id_t color_id;
    int16_t r, g, b, c;
} color_sensor_buffer_t;
static color_sensor_buffer_t s_color_buffer[LUMP_MAX_INSTANCES_PER_TYPE];

/* インスタンスごとの、未処理のキャリブレーション要求 */
static lump_color_calib_request_t s_calib_request;
static lump_color_sys_mode_t s_calib_mode = LUMP_COLOR_SYS_MODE_SYSTEM;

static bool s_color_instance_active[LUMP_MAX_INSTANCES_PER_TYPE];
/* インスタンスごとの「監視対象の色」ビットマスク(bit i = 色ID iを監視中) */
static uint32_t s_watch_mask[LUMP_MAX_INSTANCES_PER_TYPE];

//カラーセンサーデータ用関数ポインタ
static lump_color_read_color_fn_t s_read_fn = NULL;
static lump_color_update_color_ref_t s_update_color_ref_fn = NULL;

void lump_color_register_read_color_fn(lump_color_read_color_fn_t fn)
{
    s_read_fn = fn;
}

void lump_color_poll_read_color(void)
{
    if (s_read_fn == NULL) return;

    for (uint8_t instance_id = 0; instance_id < LUMP_MAX_INSTANCES_PER_TYPE; instance_id++) 
    {
        if (!lump_color_is_instance_active(instance_id)) continue;

        uint16_t r, g, b, c;
        lump_color_id_t color_id;
        if (s_read_fn(instance_id, &r, &g, &b, &c, &color_id)) 
        {
            lump_color_set_rgbc(instance_id, r, g, b, c);
            lump_color_set_color_id(instance_id, color_id);
        }
    }
}

void lump_color_register_update_color_ref_fn(lump_color_update_color_ref_t fn)
{
    s_update_color_ref_fn = fn;
}

void lump_color_poll_update_color_ref(uint8_t instance_id)
{
    if (s_update_color_ref_fn == NULL) return;
    s_update_color_ref_fn(instance_id);
}

static void on_color_sensor_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4);

void lump_color_notify_color_task(void *arg);

void lump_color_init(void) 
{
    memset(s_color_instance_active, 0, sizeof(s_color_instance_active));
    memset(s_color_buffer, 0, sizeof(s_color_buffer));
    s_calib_request.is_request = false;
    lump_command_dispatch_register(LUMP_SENSOR_COLOR, on_color_sensor_command);
    xTaskCreate(lump_color_notify_color_task, "lump_color_notify", 4096, NULL, 5, NULL);
}

bool lump_color_is_instance_active(uint8_t instance_id) 
{
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return false;
    return s_color_instance_active[instance_id];
}

void lump_color_set_color_id(uint8_t instance_id, lump_color_id_t color_id) 
{
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    s_color_buffer[instance_id].color_id = color_id;
}

void lump_color_set_rgbc(uint8_t instance_id, int16_t r, int16_t g, int16_t b, int16_t c) 
{
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    s_color_buffer[instance_id].r = r;
    s_color_buffer[instance_id].g = g;
    s_color_buffer[instance_id].b = b;
    s_color_buffer[instance_id].c = c;
}

void lump_color_get_rgbc_buffer(uint8_t instance_id, int16_t *r, int16_t *g, int16_t *b, int16_t *c) 
{
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    
    *r = s_color_buffer[instance_id].r;
    *g = s_color_buffer[instance_id].g;
    *b = s_color_buffer[instance_id].b;
    *c = s_color_buffer[instance_id].c;
}

void lump_color_report_color_id(uint8_t instance_id) 
{
    if (!lump_color_is_instance_active(instance_id)) return;
    lump_device_report(LUMP_SENSOR_COLOR, COLOR_MODE_COLOR_ID, instance_id, s_color_buffer[instance_id].color_id, 0, 0, 0);
}

static bool watch_contains_color(uint16_t mask, uint8_t color_id)
{
    if (color_id >= 16) {
        return false;
    }

    return (mask & (1U << color_id)) != 0;
}

void lump_color_notify_color_task(void *arg)
{
    lump_color_id_t last_send_color[LUMP_MAX_INSTANCES_PER_TYPE];
    uint8_t last_send_instance_id = 0;
    memset(last_send_color, LUMP_COLOR_UNKNOWN, sizeof(last_send_color));

    TickType_t last_wake_time = xTaskGetTickCount();
    while (true)
    {
        for (int i = 0; i < LUMP_MAX_INSTANCES_PER_TYPE; i++)
        {
            uint8_t instance_id = (last_send_instance_id + i + 1) % LUMP_MAX_INSTANCES_PER_TYPE;
            lump_color_id_t send_color;
            if (!lump_color_is_instance_active(instance_id)) continue;

            if (watch_contains_color(s_watch_mask[instance_id], s_color_buffer[instance_id].color_id)) 
                send_color = s_color_buffer[instance_id].color_id;
            else send_color = LUMP_COLOR_UNKNOWN;

            if (send_color != last_send_color[instance_id])
            {
                ESP_LOGI("COLOR","%d",send_color);
                lump_device_report(LUMP_SENSOR_COLOR, COLOR_MODE_NOTIFY_COLOR, instance_id, send_color, 0, 0, 0);
                last_send_color[instance_id] = send_color;
                last_send_instance_id = instance_id;
                break;
            }
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10));
    }
}

void lump_color_report_rgbc(uint8_t instance_id) 
{
    if (!lump_color_is_instance_active(instance_id)) return;
    lump_device_report(LUMP_SENSOR_COLOR, COLOR_MODE_RGBC, instance_id, 
                        s_color_buffer[instance_id].r, 
                        s_color_buffer[instance_id].g, 
                        s_color_buffer[instance_id].b, 
                        s_color_buffer[instance_id].c
                        );
}

static void color_systems(uint8_t instance_id, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    switch (v1)
    {
    case LUMP_SENSOR_INIT:
        s_color_instance_active[instance_id] = (v2 != 0);
        break;
    case LUMP_COLOR_SYS_MODE_NOT_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_NOT;
        break;
    case LUMP_COLOR_SYS_MODE_COLOR_CALIB:
        now_calib_sensor = LUMP_CALIB_SENSOR_COLOR;
        s_calib_mode = LUMP_COLOR_SYS_MODE_COLOR_CALIB;
        s_calib_request.color_id    = v2;
        s_calib_request.instance_id = instance_id;
        s_calib_request.save_value  = (v3 != 0);
        s_calib_request.is_request = true;
        break;
    default:
        break;
    }
}

static void handle_watch_config(uint8_t instance_id, int16_t v1, int16_t v2) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    if (v1 < 0 || v1 >= COLOR_SENSOR_MAX_WATCH_COLOR_ID) return; /* 範囲外の色IDは無視 */
    
    uint16_t bit = (1UL << v1);
    if (v2 != 0) {
        s_watch_mask[instance_id] |= bit;  /* 追加 */
    } else {
        s_watch_mask[instance_id] &= ~bit; /* 削除 */
    }
}

lump_color_sys_mode_t lump_color_get_calib_mode()
{
    return s_calib_mode;
}


bool lump_color_get_calib_request(lump_color_calib_request_t *req_data) 
{
    if (!s_calib_request.is_request) return false;

    req_data->color_id    = s_calib_request.color_id;
    req_data->instance_id = s_calib_request.instance_id;
    req_data->save_value  = s_calib_request.save_value;
    req_data->is_request  = s_calib_request.is_request;
    s_calib_request.is_request = false; /* 取得したら消費する */
    return true;
}

static void on_color_sensor_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    switch (command)
    {
    case COLOR_MODE_RGBC:
        lump_color_report_rgbc(instance_id);
        break;
    case COLOR_MODE_COLOR_ID:
        lump_color_report_color_id(instance_id);
        break;
    case COLOR_MODE_NOTIFY_COLOR:
        handle_watch_config(instance_id, v1, v2);
        break;
    case COLOR_MODE_SYSTEM:
        color_systems(instance_id, v1, v2, v3, v4);
        break;
    default:
        break;
    }
}
