/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* NOTE: For the security of the protected storage system, the bootloader
 * rollback protection, and the protection of cryptographic material it is
 * CRITICAL to use an internal (in-die) persistent memory for the implementation
 * of the OTP_NV_COUNTERS flash area.
 */

#include "nvt_flash_otp_nv_counters_backend.h"

#include "tfm_plat_defs.h"
#include "Driver_Flash.h"
#include "flash_layout.h"
#include "NuMicro.h"

#include <string.h>

static enum tfm_plat_err_t create_or_restore_layout(void);

#if defined(OTP_WRITEABLE)
static enum tfm_plat_err_t make_backup(void);
#endif

/* Compilation time checks to be sure the defines are well defined */
#ifndef TFM_OTP_NV_COUNTERS_AREA_ADDR
#error "TFM_OTP_NV_COUNTERS_AREA_ADDR must be defined in flash_layout.h"
#endif

#if defined(OTP_WRITEABLE)
#ifndef TFM_OTP_NV_COUNTERS_BACKUP_AREA_ADDR
#error "TFM_OTP_NV_COUNTERS_BACKUP_AREA_ADDR must be defined in flash_layout.h"
#endif
#endif

#ifndef TFM_OTP_NV_COUNTERS_AREA_SIZE
#error "TFM_OTP_NV_COUNTERS_AREA_SIZE must be defined in flash_layout.h"
#endif

#if defined(OTP_WRITEABLE)
#ifndef TFM_OTP_NV_COUNTERS_SECTOR_SIZE
#error "TFM_OTP_NV_COUNTERS_SECTOR_SIZE must be defined in flash_layout.h"
#endif
#endif
#ifndef OTP_NV_COUNTERS_FLASH_DEV
#ifndef TFM_HAL_ITS_FLASH_DRIVER
#error "OTP_NV_COUNTERS_FLASH_DEV or TFM_HAL_ITS_FLASH_DRIVER must be defined in flash_layout.h"
#else
#define OTP_NV_COUNTERS_FLASH_DEV TFM_HAL_ITS_FLASH_DRIVER
#endif
#endif

/* Default flash block size is either the PROGRAM_UNIT if it is > 128, or if the
 * program unit is < 128, then it is the closest multiple of the PROGRAM_UNIT to
 * 128. The aim of this is to keep the block size reasonable, to avoid large
 * amounts of read/write calls, while also keeping sensible alignment.
 */
#ifndef OTP_NV_COUNTERS_WRITE_BLOCK_SIZE
#define OTP_NV_COUNTERS_WRITE_BLOCK_SIZE (TFM_HAL_ITS_PROGRAM_UNIT > 128 ? \
                                          TFM_HAL_ITS_PROGRAM_UNIT : \
                                          (128 / TFM_HAL_ITS_PROGRAM_UNIT) * TFM_HAL_ITS_PROGRAM_UNIT)
#endif /* OTP_NV_COUNTERS_WRITE_BLOCK_SIZE */

#if (TFM_OTP_NV_COUNTERS_SECTOR_SIZE % OTP_NV_COUNTERS_WRITE_BLOCK_SIZE != 0) || \
    (OTP_NV_COUNTERS_WRITE_BLOCK_SIZE % TFM_HAL_ITS_PROGRAM_UNIT != 0)
#error "OTP_NV_COUNTERS_WRITE_BLOCK_SIZE has wrong alignment"
#endif
/* End of compilation time checks to be sure the defines are well defined */

static uint8_t block[OTP_NV_COUNTERS_WRITE_BLOCK_SIZE];

/* Import the CMSIS flash device driver */
extern ARM_DRIVER_FLASH OTP_NV_COUNTERS_FLASH_DEV;


enum tfm_plat_err_t read_otp_nv_counters_flash(uint32_t offset, void *data, uint32_t cnt)
{
    enum tfm_plat_err_t err = TFM_PLAT_ERR_SUCCESS;

    //printf("OTP Base %08x Offset %x Size %d\r\n", TFM_OTP_NV_COUNTERS_AREA_ADDR, offset, cnt);

    err = OTP_NV_COUNTERS_FLASH_DEV.ReadData(TFM_OTP_NV_COUNTERS_AREA_ADDR + offset,
            data,
            cnt);
    
    /* NV counter flash is writeable. Need to translate to bit clear format */
    if((offset >= offsetof(struct flash_otp_nv_counters_region_t, flash_nv_counters)) &&
        (offset < offsetof(struct flash_otp_nv_counters_region_t, flash_nv_counters) + FLASH_NV_COUNTER_AM * sizeof(uint32_t)))
    {
        printf("NV counter flash %d=%d\r\n", (offset - offsetof(struct flash_otp_nv_counters_region_t, flash_nv_counters))/4, *((uint32_t*)data));

    }

    if(err != ARM_DRIVER_OK)
    {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return err;
}


enum tfm_plat_err_t init_otp_nv_counters_flash(void)
{
    enum tfm_plat_err_t err = TFM_PLAT_ERR_SUCCESS;
    uint32_t init_value;
    uint32_t is_valid;

    if((TFM_OTP_NV_COUNTERS_AREA_SIZE) < sizeof(struct flash_otp_nv_counters_region_t))
    {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    err = OTP_NV_COUNTERS_FLASH_DEV.Initialize(NULL);
    if(err != ARM_DRIVER_OK)
    {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    err = read_otp_nv_counters_flash(offsetof(struct flash_otp_nv_counters_region_t, init_value),
                                     &init_value, sizeof(init_value));
    if(err != TFM_PLAT_ERR_SUCCESS)
    {
        return err;
    }

    err = read_otp_nv_counters_flash(offsetof(struct flash_otp_nv_counters_region_t, is_valid),
                                     &is_valid, sizeof(is_valid));
    if(err != TFM_PLAT_ERR_SUCCESS)
    {
        return err;
    }


    if(init_value != OTP_NV_COUNTERS_INITIALIZED || is_valid != OTP_NV_COUNTERS_IS_VALID)
    {

        printf("No provisioning data found in OTP.\r\n");
        err = TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return err;
}


static inline uint32_t round_down(uint32_t num, uint32_t boundary)
{
    return num - (num % boundary);
}

static inline uint32_t round_up(uint32_t num, uint32_t boundary)
{
    return (num + boundary - 1) - ((num + boundary - 1) % boundary);
}

static enum tfm_plat_err_t erase_flash_region(size_t start, size_t size)
{
    enum tfm_plat_err_t err = TFM_PLAT_ERR_SUCCESS;
    size_t idx;

    if((start % TFM_OTP_NV_COUNTERS_SECTOR_SIZE) != 0)
    {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    for(idx = round_down(start, TFM_OTP_NV_COUNTERS_SECTOR_SIZE);
            idx < start + size;
            idx += TFM_OTP_NV_COUNTERS_SECTOR_SIZE)
    {
        err = OTP_NV_COUNTERS_FLASH_DEV.EraseSector(idx);
        if(err != ARM_DRIVER_OK)
        {
            return TFM_PLAT_ERR_SYSTEM_ERR;
        }
    }

    return err;
}

enum tfm_plat_err_t write_otp_nv_counters_flash(uint32_t offset, const void *data, uint32_t cnt)
{
    enum tfm_plat_err_t err = TFM_PLAT_ERR_SUCCESS;

    if(offset + cnt >= TFM_OTP_NV_COUNTERS_AREA_SIZE)
    {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    err = OTP_NV_COUNTERS_FLASH_DEV.ProgramData(TFM_OTP_NV_COUNTERS_AREA_ADDR + offset, data, cnt);
    if(err != ARM_DRIVER_OK)
    {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return err;
}
