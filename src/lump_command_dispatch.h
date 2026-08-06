#pragma once

#include <stdint.h>
#include "lump_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * lump_command のキュー(lump_command_pop/lump_command_count)を消費し、
 * センサー種別ごとに登録されたハンドラへ自動的に振り分けるディスパッチ機構。
 *
 * lump_sensors.c (センサー別API本体)とは別ファイルにしてあるのは、
 * 「値を送る側のAPI」と「コマンドを受け取って振り分ける側の仕組み」の
 * 責務を分けるため。
 */

/*
 * コマンドを受け取った時に呼ばれるハンドラの型。
 * instance_id : そのセンサー種別内での個体識別子
 * command     : byte0下位5bitのコマンド/モード番号
 *               (0はSPIKE統合済みの初期設定/キャリブレーション用と解釈すること)
 * seq         : 送信側のシーケンス番号(重複判定などに使いたい場合に利用)
 */
typedef void (*lump_command_handler_t)(uint8_t instance_id, uint8_t command, uint8_t seq,
                                        int16_t v1, int16_t v2, int16_t v3, int16_t v4);

/*
 * センサー種別ごとにハンドラを1つ登録する。
 * 同じ種別に対して再度呼ぶと、以前のハンドラを上書きする。
 * handler に NULL を渡すと、その種別の登録を解除する。
 */
void lump_command_dispatch_register(lump_sensor_type_t type, lump_command_handler_t handler);

/*
 * キューに溜まっているコマンドを古い順に全て取り出し、それぞれの
 * センサー種別に登録されたハンドラへ振り分けて呼び出す。
 * ハンドラが登録されていない種別のコマンドは、読み捨てられる。
 *
 * メインループやタスクの中で定期的に呼ぶこと。
 */
void lump_command_dispatch_poll(void);

#ifdef __cplusplus
}
#endif
