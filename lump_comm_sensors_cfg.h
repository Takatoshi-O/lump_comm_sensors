#ifndef LUMP_COMM_SENSORS_CFG_H
/** @brief センサー共通設定ヘッダーのインクルードガードです。 */
#define LUMP_COMM_SENSORS_CFG_H
/**
 * @file lump_comm_sensors_cfg.h
 * @brief Kconfigで選択されたセンサーの有効状態とキャリブレーション機能の有効状態を、コンポーネント内で使用しやすいマクロへ変換します。
 */

#ifdef CONFIG_COLOR_SENSOR_AVAILABLE
/** @brief カラーセンサーが有効化されていることを表します。 */
#define COLOR_SENSOR_AVAILABLE true
#else
/** @brief カラーセンサーが無効化されていることを表します。 */
#define COLOR_SENSOR_AVAILABLE false
#endif

#ifdef CONFIG_CAMERA_AVAILABLE
/** @brief カメラが有効化されていることを表します。 */
#define CAMERA_AVAILABLE true
#else
/** @brief カメラが無効化されていることを表します。 */
#define CAMERA_AVAILABLE false
#endif

#ifdef CONFIG_COLOR_CALIB_ENABLE
/** @brief カラーセンサーのキャリブレーション機能が有効です。 */
#define COLOR_CALIB_ENABLE true
#else
/** @brief カラーセンサーのキャリブレーション機能が無効です。 */
#define COLOR_CALIB_ENABLE false
#endif

#ifdef CONFIG_CAMERA_CALIB_ENABLE
/** @brief カメラのキャリブレーション機能が有効です。 */
#define CAMERA_CALIB_ENABLE true
#else
/** @brief カメラのキャリブレーション機能が無効です。 */
#define CAMERA_CALIB_ENABLE false
#endif

#endif /* LUMP_COMM_SENSORS_CFG_H */