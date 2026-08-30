# lump_comm_sensors

`lump_comm_sensors` is the sensor-facing layer built on top of `lump_comm`. It connects application sensor drivers to the LUMP transport and handles sensor-specific commands, modes, instance activation, and calibration workflows.

## Supported sensor layers

The component currently provides:

- Color sensor API (`LUMP_TYPE_1`)
- Camera API (`LUMP_TYPE_2`)
- Shared sensor state/status handling
- Command dispatching
- Optional calibration tasks controlled by Kconfig

The actual sensor hardware access is intentionally supplied through callback registration APIs. This keeps the communication layer independent from a particular I2C driver or camera implementation.

## Directory structure

```text
lump_comm_sensors/
├── include/
│   ├── lump_comm_sensors.h
│   ├── lump_comm_color.h
│   └── lump_comm_camera.h
├── src/
│   ├── lump_comm_sensors.c
│   ├── lump_command_dispatch.c/.h
│   ├── lump_comm_color.c
│   ├── lump_comm_camera.c
│   ├── lump_comm_calib.c/.h
│   ├── lump_comm_cam_calib.c
│   ├── lump_comm_color_calib.c
│   └── lump_sensors_register.c
├── lump_comm_sensors_cfg.h
├── Kconfig
└── CMakeLists.txt
```

## Kconfig

Select **Component config -> LUMP comm sensors Configuration**.

### Available sensor options

| Option | Default | Meaning |
|---|---|---|
| `COLOR_SENSOR_AVAILABLE` | off | Enable the color-sensor integration |
| `CAMERA_AVAILABLE` | off | Enable the camera integration |

### Calibration options

| Option | Default | Meaning |
|---|---|---|
| `COLOR_CALIB_ENABLE` | off | Enable color-sensor calibration handling |
| `CAMERA_CALIB_ENABLE` | off | Enable camera calibration handling |

The internal `lump_comm_sensors_cfg.h` converts these Kconfig symbols into boolean-like macros used by the component.

## Initialization pattern

The transport should be started first, followed by the sensor layer and application-provided callbacks.

```c
lump_device_start();
lump_sensors_start();
lump_sersors_register();
```

For a camera integration, register the required camera read/update callbacks declared in `lump_comm_camera.h`. For a color sensor, register the corresponding functions declared in `lump_comm_color.h`.

The exact application startup order can be adapted to the surrounding tasks, but the hardware callback dependencies must be registered before polling APIs are expected to return sensor values.

## Color sensor API

`lump_comm_color.h` provides:

- Color sensor instance activation checks
- RGBC and color-ID buffers
- Callback registration for reading sensor values
- Callback registration for updating calibration references
- Reporting modes for RGBC and color ID
- Calibration request retrieval

The current LUMP modes implemented by the color layer include:

- mode 0: system/initialization
- mode 1: RGBC
- mode 2: color ID
- mode 3: notify watched-color changes

## Camera API

`lump_comm_camera.h` provides:

- Single-position color read callback registration
- 12-position color read callback registration
- Color reference update callback registration
- Camera instance activation
- Single color result reporting
- 12-position color result reporting
- Color and position calibration request retrieval

The current camera system modes include normal/system state, color calibration, and position calibration.

## Command dispatch

`lump_command_dispatch` consumes commands from the `lump_comm` command queue and dispatches them by `lump_sensor_type_t`.

Register a handler once per sensor type:

```c
lump_command_dispatch_register(LUMP_SENSOR_COLOR, color_handler);
lump_command_dispatch_register(LUMP_SENSOR_CAMERA, camera_handler);
```

Call `lump_command_dispatch_poll()` periodically from the task or loop responsible for command processing.

## Calibration

Calibration tasks are separated from the normal sensor transport APIs. The internal calibration layer tracks the current calibration target through `lump_calib_sensor_t` and starts color/camera calibration handlers when enabled by Kconfig.

## Dependencies

- `lump_comm`
- `freertos`
- `lcd_lvgl`
- `nvs_manager`
- `camera_manager`
- `color_sensor`

## Public headers

- `lump_comm_sensors.h`
- `lump_comm_color.h`
- `lump_comm_camera.h`
