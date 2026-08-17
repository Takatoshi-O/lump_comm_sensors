#include <stdio.h>
#include <string.h>
#include "lump_comm_color.h"
#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"

#define LUMP_COLOR_CALIB 1
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
    int16_t color_id;
    int16_t r, g, b, c;
} color_sensor_buffer_t;

/* インスタンスごとの、未処理のキャリブレーション要求 */
typedef struct {
    bool pending;
    int16_t color_id; /* キャリブレーション対象の色ID(要求時のv1) */
} color_calib_request_t;

static bool s_color_instance_active[LUMP_MAX_INSTANCES_PER_TYPE];
static color_sensor_buffer_t s_color_buffer[LUMP_MAX_INSTANCES_PER_TYPE];
static color_calib_request_t s_calib_request[LUMP_MAX_INSTANCES_PER_TYPE];
static bool calib_request = false;
/* インスタンスごとの「監視対象の色」ビットマスク(bit i = 色ID iを監視中) */
static uint32_t s_watch_mask[LUMP_MAX_INSTANCES_PER_TYPE];

static void on_color_sensor_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4);

void lump_color_init(void) {
    memset(s_color_instance_active, 0, sizeof(s_color_instance_active));
    memset(s_color_buffer, 0, sizeof(s_color_buffer));
    lump_command_dispatch_register(LUMP_SENSOR_COLOR, on_color_sensor_command);
}

bool color_sensor_is_instance_active(uint8_t instance_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return false;
    return s_color_instance_active[instance_id];
}

void lump_color_set_color_id(uint8_t instance_id, int16_t color_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    s_color_buffer[instance_id].color_id = color_id;
}

void lump_color_set_rgbc(uint8_t instance_id, int16_t r, int16_t g, int16_t b, int16_t c) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
    s_color_buffer[instance_id].r = r;
    s_color_buffer[instance_id].g = g;
    s_color_buffer[instance_id].b = b;
    s_color_buffer[instance_id].c = c;
}

void lump_color_report_color_id(uint8_t instance_id) 
{
    if (!color_sensor_is_instance_active(instance_id)) return;
    if (!is_valid_mode(COLOR_MODE_COLOR_ID)) return; /* 念のため(定数なので実際には外れない) */
    lump_device_report(LUMP_SENSOR_COLOR, COLOR_MODE_COLOR_ID, instance_id, s_color_buffer[instance_id].color_id, 0, 0, 0);
}

void lump_color_notify_color_id(uint8_t instance_id) 
{
    if (!color_sensor_is_instance_active(instance_id)) return;
    if (!is_valid_mode(COLOR_MODE_COLOR_ID)) return; /* 念のため(定数なので実際には外れない) */
    lump_device_report(LUMP_SENSOR_COLOR, COLOR_MODE_NOTIFY_COLOR, instance_id, s_color_buffer[instance_id].color_id, 0, 0, 0);
}

void lump_color_report_rgbc(uint8_t instance_id) 
{
    if (!color_sensor_is_instance_active(instance_id)) return;
    if (!is_valid_mode(COLOR_MODE_RGBC)) return;
    lump_device_report(LUMP_SENSOR_COLOR, COLOR_MODE_RGBC, instance_id, 
                        s_color_buffer[instance_id].r, 
                        s_color_buffer[instance_id].g, 
                        s_color_buffer[instance_id].b, 
                        s_color_buffer[instance_id].c
                        );
}

static void color_sensor_systems(uint8_t instance_id, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    switch (v1)
    {
    case LUMP_SENSOR_INIT:
        if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return;
        s_color_instance_active[instance_id] = (v2 != 0);
        break;
    case LUMP_COLOR_CALIB:
        if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE)
        {
            for (int i = 0; i < LUMP_MAX_INSTANCES_PER_TYPE; i++)
            {
                s_calib_request[i].pending = true;
                s_calib_request[i].color_id = v2;
            }
        }
        else
        {
            s_calib_request[instance_id].pending = true;
            s_calib_request[instance_id].color_id = v2;
        }
        calib_request = true;
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

bool color_id_to_mask(uint8_t color_id, uint16_t *mask)
{
    if (mask == NULL || color_id >= COLOR_SENSOR_MAX_WATCH_COLOR_ID) {
        return false;
    }

    *mask = (uint16_t)(1U << color_id);
    return true;
}

uint8_t mask_to_color_ids(uint16_t mask, uint8_t *colors)
{
    uint8_t count = 0;

    for (uint8_t i = 0; i < COLOR_SENSOR_MAX_WATCH_COLOR_ID; i++) {
        if (mask & (1U << i)) {
            colors[count++] = i;
        }
    }

    return count;
}

bool watch_contains_color(uint16_t mask, uint8_t color_id)
{
    if (color_id >= 16) {
        return false;
    }

    return (mask & (1U << color_id)) != 0;
}

bool lump_color_get_calib_request(uint8_t instance_id, int16_t *out_color_id) {
    if (instance_id >= LUMP_MAX_INSTANCES_PER_TYPE) return false;
    if (!s_calib_request[instance_id].pending) return false;

    *out_color_id = s_calib_request[instance_id].color_id;
    s_calib_request[instance_id].pending = false; /* 取得したら消費する */
    return true;
}

bool lump_color_is_calib_request()
{
    return calib_request;
}

void lump_color_calib_fin()
{
    calib_request = false;
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
        color_sensor_systems(instance_id, v1, v2, v3, v4);
        break;
    default:
        break;
    }
}