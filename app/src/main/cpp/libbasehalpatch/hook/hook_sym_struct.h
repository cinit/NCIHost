//
// Created by kinit on 2021-11-18.
//

#ifndef NCI_HOST_NATIVES_HOOK_SYM_STRUCT_H
#define NCI_HOST_NATIVES_HOOK_SYM_STRUCT_H

#include <stdint.h>

struct OriginHookProcedure {
    uint32_t struct_size;
    uint32_t entry_count;
    uint64_t target_base;
    uint32_t off_plt_open_2;
    uint32_t off_plt_open;
    uint32_t off_plt_read_chk;
    uint32_t off_plt_read;
    uint32_t off_plt_write_chk;
    uint32_t off_plt_write;
    uint32_t off_plt_close;
    uint32_t off_plt_ioctl;
    uint32_t off_plt_select;
    uint32_t unused32_0;
};

#endif //NCI_HOST_NATIVES_HOOK_SYM_STRUCT_H
