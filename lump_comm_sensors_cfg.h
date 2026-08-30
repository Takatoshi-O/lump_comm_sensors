#ifndef LUMP_COMM_SENSORS_CFG_H
#define LUMP_COMM_SENSORS_CFG_H

#ifdef CONFIG_COLOR_SENSOR_AVAILABLE
#define COLOR_SENSOR_AVAILABLE true
#else
#define COLOR_SENSOR_AVAILABLE false
#endif

#ifdef CONFIG_CAMERA_AVAILABLE
#define CAMERA_AVAILABLE true
#else
#define CAMERA_AVAILABLE false
#endif

#ifdef CONFIG_COLOR_CALIB_ENABLE
#define COLOR_CALIB_ENABLE true
#else
#define COLOR_CALIB_ENABLE false
#endif

#ifdef CONFIG_CAMERA_CALIB_ENABLE
#define CAMERA_CALIB_ENABLE true
#else
#define CAMERA_CALIB_ENABLE false
#endif

#endif /* LUMP_COMM_SENSORS_CFG_H */