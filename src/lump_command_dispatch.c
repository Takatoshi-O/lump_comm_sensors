#include "lump_command_dispatch.h"
#include "lump_command.h"

static lump_command_handler_t s_handlers[LUMP_TYPE_MAX] = {0};

void lump_command_dispatch_register(lump_sensor_type_t type, lump_command_handler_t handler) {
    if (type >= LUMP_TYPE_MAX) return;
    s_handlers[type] = handler;
}

void lump_command_dispatch_poll(void) {
    lump_command_entry_t entry;
    while (lump_command_pop(&entry)) {
        if (entry.type >= LUMP_TYPE_MAX) continue;

        lump_command_handler_t handler = s_handlers[entry.type];
        if (handler != NULL) {
            handler(entry.instance_id, entry.command, entry.seq,
                     entry.v1, entry.v2, entry.v3, entry.v4);
        }
        /* ハンドラが未登録の種別のコマンドは読み捨てる */
    }
}
