/*
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cmsis.h"
#include "region.h"
#include "target_cfg.h"
#include "cmsis.h"
#include "boot_hal.h"
#include "Driver_Flash.h"
#include "flash_layout.h"

 /* Flash device name must be specified by target */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;

REGION_DECLARE(Image$$, ER_DATA, $$Base)[];
REGION_DECLARE(Image$$, ARM_LIB_HEAP, $$ZI$$Limit)[];

__attribute__((naked)) void boot_clear_bl2_ram_area(void)
{
    __ASM volatile(
#ifndef __ICCARM__
        ".syntax unified                             \n"
#endif
        "movs    r0, #0                              \n"
        "subs    %1, %1, %0                          \n"
        "Loop:                                       \n"
        "subs    %1, #4                              \n"
        "blt     Clear_done                          \n"
        "str     r0, [%0, %1]                        \n"
        "b       Loop                                \n"
        "Clear_done:                                 \n"
        "bx      lr                                  \n"
        :
        : "r" (REGION_NAME(Image$$, ER_DATA, $$Base)),
          "r" (REGION_NAME(Image$$, ARM_LIB_HEAP, $$ZI$$Limit))
        : "r0", "memory"
    );
}


/* bootloader platform-specific hw initialization */
int32_t boot_platform_init(void)
{
    int32_t result;

    result = FLASH_DEV_NAME.Initialize(NULL);
    if(result != ARM_DRIVER_OK) {
        return 1;
    }
    
    result = FLASH_DEV_NAME_2.Initialize(NULL);
    if(result != ARM_DRIVER_OK) {
        return 1;
    }

    return 0;
}

