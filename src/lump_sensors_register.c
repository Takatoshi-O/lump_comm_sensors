#include "lump_comm_sensors.h"
#include "lump_command_dispatch.h"
#include "lump_comm_color.h"
#include "lump_comm_camera.h"
#include "lump_comm_sensors_cfg.h"

#ifdef CONFIG_COLOR_SENSOR_AVAILABLE

#include "color_calib.h"
#include "color_classify.h"
#include "color_sensor_hw.h"

static bool color_read_color(uint8_t instance_id,
                             uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c,
                             lump_color_id_t *color_id)
{
    color_sensor_hw_read_channel(instance_id, r, g, b, c);
    *color_id = (lump_color_id_t)color_classify_detect(instance_id, *r, *g, *b, *c);
    return true;
}

static void color_update_color_ref(uint8_t instance_id)
{
    color_calib_reload(instance_id);
}

#endif

#ifdef CONFIG_CAMERA_AVAILABLE

#include "camera_yuv_query.h"
#include "camera_color.h"

static bool camera_read_pos_color(uint8_t instance_id,
                                  int16_t x, int16_t y, uint8_t radius,
                                  int16_t *out_y, int16_t *out_u, int16_t *out_v,
                                  lump_color_id_t *out_color_id)
{
    camera_yuv_result_t yuv;
    esp_err_t err = camera_yuv_get_average(x, y, radius, &yuv, pdMS_TO_TICKS(1000));
    if (err != ESP_OK) return false;
    *out_color_id = (lump_color_id_t)camera_color_classify(instance_id, yuv.y, yuv.u, yuv.v);
    *out_y = yuv.y; *out_u = yuv.u; *out_v = yuv.v;
    return true;
}

static bool camera_read_12pos_color(uint8_t instance_id, uint8_t radius,
                                    const lump_camera_pos_t positions[12],
                                    lump_color_id_t out_colors[12])
{
    camera_yuv_point_t points[12];
    for (int i = 0; i < 12; i++)
    {
        points[i].x = positions[i].x;
        points[i].y = positions[i].y;
        points[i].radius = radius;
    }
    
    camera_yuv_result_t results[12];
    esp_err_t err = camera_yuv_get_batch(points, results, 12, pdMS_TO_TICKS(1000));
    if (err != ESP_OK) return false;
    for (int i = 0; i < 12; i++)
    {
        if (results[i].status != ESP_OK) 
        {
            out_colors[i] = LUMP_COLOR_UNKNOWN;
            continue;
        }
        out_colors[i] = camera_color_classify(instance_id, results[i].y, results[i].u, results[i].v);
    }
    
    return true;
}

static void camera_update_color_ref(uint8_t instance_id)
{
    camera_load_reference(instance_id);
}

#endif

void lump_sersors_register()
{
    #ifdef CONFIG_COLOR_SENSOR_AVAILABLE
        lump_color_register_read_color_fn(color_read_color);
        lump_color_register_update_color_ref_fn(color_update_color_ref);
    #endif

    #ifdef CONFIG_CAMERA_AVAILABLE
        lump_camera_register_read_pos_color_fn(camera_read_pos_color);
        lump_camera_register_read_12pos_color_fn(camera_read_12pos_color);
        lump_camera_register_update_color_ref_fn(camera_update_color_ref);
    #endif
}