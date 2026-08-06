#include <stdio.h>
#include "lump_comm_camera.h"
#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"

/*
 * カメラのモード定義。
 * 0: 予約(初期設定用)
 * 1〜31: 後で追加するモード用に空けておく
 */
typedef enum {
    CAMERA_MODE_SYSTEM = 0,
    CAMERA_MODE_12POS_COLOR = 2,
    /* 将来のモードはここに追加する:
     * CAMERA_MODE_POSITION  = 1,
     * CAMERA_MODE_BBOX      = 2,
     * CAMERA_MODE_DETECTION = 3,
     */
} camera_mode_t;

uint8_t position_colors[3][4] = {0};
bool request_get_color = false;

static void on_camera_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4);

void lump_camera_init(void) {
    lump_command_dispatch_register(LUMP_SENSOR_CAMERA, on_camera_command);
}


void camera_report(uint8_t instance_id, uint8_t mode, int16_t v1, int16_t v2, int16_t v3, int16_t v4) 
{
    if (!is_valid_mode(mode)) return;
    lump_device_report(LUMP_SENSOR_CAMERA, instance_id, mode, v1, v2, v3, v4);
}

void lump_camera_set_12pos_color(int8_t color_lists[3][4])
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
    /*
    if (in == NULL || out == NULL) {
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (in[i] > 0x0F) {
            return;      // 4bitを超える値
        }
    }
    */

    out=((uint16_t)in[0] << 12) |
        ((uint16_t)in[1] << 8)  |
        ((uint16_t)in[2] << 4)  |
        ((uint16_t)in[3]);

    return out;
}

void lump_camera_report_12pos_color(uint8_t instance_id)
{
    uint16_t buf[3];

    for (int i = 0; i < 3; i++)
    {
        buf[i] = pack_nibbles(position_colors[i]);
    }
    lump_device_report(LUMP_SENSOR_CAMERA, CAMERA_MODE_12POS_COLOR, instance_id, 0, buf[0], buf[1], buf[2]);
}

static void on_camera_command(uint8_t instance_id, uint8_t command, uint8_t seq, int16_t v1, int16_t v2, int16_t v3, int16_t v4)
{
    switch (command)
    {
    case CAMERA_MODE_12POS_COLOR:
        request_get_color = true;
        break;
    case CAMERA_MODE_SYSTEM:
        break;
    default:
        break;
    }
}

bool lump_color_is_calib_request()
{
    return request_get_color;
}

void lump_color_calib_fin()
{
    request_get_color = false;
}