# lump_comm_sensors

`lump_comm_sensors` は `lump_comm` の上位に位置するセンサー向けAPI層です。アプリケーション側のセンサードライバをLUMP通信へ接続し、センサー固有のコマンド、モード、インスタンス管理、キャリブレーション処理を担当します。

## 対応センサー層

現在は以下を提供します。

- カラーセンサーAPI（`LUMP_TYPE_1`）
- カメラAPI（`LUMP_TYPE_2`）
- センサー共通の状態・ステータス処理
- コマンドディスパッチ
- Kconfigで有効化できるキャリブレーション処理

実際のセンサー読み取りはコールバック登録API経由でアプリケーション側から提供します。これにより、通信層を特定のI2Cドライバやカメラ実装から分離しています。

## ディレクトリ構成

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

## Kconfig設定

**Component config -> LUMP comm sensors Configuration** から設定します。

### 使用可能センサー

| 項目 | デフォルト | 意味 |
|---|---|---|
| `COLOR_SENSOR_AVAILABLE` | off | カラーセンサー連携を有効化 |
| `CAMERA_AVAILABLE` | off | カメラ連携を有効化 |

### キャリブレーション

| 項目 | デフォルト | 意味 |
|---|---|---|
| `COLOR_CALIB_ENABLE` | off | カラーセンサーのキャリブレーション処理を有効化 |
| `CAMERA_CALIB_ENABLE` | off | カメラのキャリブレーション処理を有効化 |

`lump_comm_sensors_cfg.h` は、これらのKconfigシンボルをコンポーネント内部で使うboolean相当のマクロへ変換します。

## 初期化例

通常は通信層を先に起動し、その後センサー層とアプリケーション側コールバックを登録します。

```c
lump_device_start();
lump_sensors_start();
lump_sersors_register();
```

カメラを使用する場合は `lump_comm_camera.h` の各種コールバックを登録し、カラーセンサーでは `lump_comm_color.h` の対応するコールバックを登録します。

アプリケーションの起動順は周辺タスク構成に合わせて調整できますが、ポーリングAPIから実センサー値を取得する前に必要なハードウェアコールバックを登録しておく必要があります。

## カラーセンサーAPI

`lump_comm_color.h` は次を提供します。

- センサーインスタンスの有効状態確認
- RGBC・色IDバッファ
- センサー値取得コールバック登録
- キャリブレーション基準値更新コールバック登録
- RGBC / 色IDの送信モード
- キャリブレーション要求取得

現在のカラーセンサー層の主なモードは次のとおりです。

- mode 0: システム・初期化
- mode 1: RGBC
- mode 2: 色ID
- mode 3: 監視対象色の変化通知

## カメラAPI

`lump_comm_camera.h` は次を提供します。

- 単一点色取得コールバック登録
- 12点一括色取得コールバック登録
- 色基準値更新コールバック登録
- カメラインスタンス有効化
- 1点色結果送信
- 12点色結果送信
- 色・位置キャリブレーション要求取得

現在のカメラシステムモードには、通常状態、色キャリブレーション、位置キャリブレーションがあります。

## コマンドディスパッチ

`lump_command_dispatch` は `lump_comm` のコマンドキューから未処理コマンドを取り出し、`lump_sensor_type_t` ごとに登録されたハンドラへ振り分けます。

センサー種別ごとに1回登録します。

```c
lump_command_dispatch_register(LUMP_SENSOR_COLOR, color_handler);
lump_command_dispatch_register(LUMP_SENSOR_CAMERA, camera_handler);
```

コマンド処理を担当するタスクやループから `lump_command_dispatch_poll()` を定期的に呼び出します。

## キャリブレーション

キャリブレーション処理は通常のセンサー送信APIとは分離されています。内部のキャリブレーション層では `lump_calib_sensor_t` で現在の対象センサーを管理し、Kconfigで有効にされた場合にカラー/カメラのキャリブレーション処理を開始します。

## 依存コンポーネント

- `lump_comm`
- `freertos`
- `lcd_lvgl`
- `nvs_manager`
- `camera_manager`
- `color_sensor`

## 公開ヘッダー

- `lump_comm_sensors.h`
- `lump_comm_color.h`
- `lump_comm_camera.h`
