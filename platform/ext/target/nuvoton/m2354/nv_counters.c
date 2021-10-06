/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 * Copyright (c) 2020 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* NOTE: This API should be implemented by platform vendor. For the security of
 * the protected storage system's and the bootloader's rollback protection etc.
 * it is CRITICAL to use a internal (in-die) persistent memory for multiple time
 * programmable (MTP) non-volatile counters or use a One-time Programmable (OTP)
 * non-volatile counters solution.
 *
 * This dummy implementation assumes that the NV counters are the only data in
 * the flash sector. To use it, one flash sector should be allocated exclusively
 * for the NV counters.
 *
 * The current software dummy implementation is not resistant to asynchronous
 * power failures and should not be used in production code. It is exclusively
 * for testing purposes.
 */

#include "NuMicro.h"
#include "tfm_plat_nv_counters.h"

#include <limits.h>
#include "Driver_Flash.h"
#include "flash_layout.h"


/* Compilation time checks to be sure the defines are well defined */
#ifndef TFM_NV_COUNTERS_AREA_ADDR
#error "TFM_NV_COUNTERS_AREA_ADDR must be defined in flash_layout.h"
#endif

#ifndef TFM_NV_COUNTERS_AREA_SIZE
#error "TFM_NV_COUNTERS_AREA_SIZE must be defined in flash_layout.h"
#endif

#ifndef TFM_NV_COUNTERS_SECTOR_ADDR
#error "TFM_NV_COUNTERS_SECTOR_ADDR must be defined in flash_layout.h"
#endif

#ifndef TFM_NV_COUNTERS_SECTOR_SIZE
#error "TFM_NV_COUNTERS_SECTOR_SIZE must be defined in flash_layout.h"
#endif

#ifndef NV_COUNTERS_FLASH_DEV_NAME
    #ifndef FLASH_DEV_NAME
    #error "NV_COUNTERS_FLASH_DEV_NAME or FLASH_DEV_NAME must be defined in flash_layout.h"
    #else
    #define NV_COUNTERS_FLASH_DEV_NAME FLASH_DEV_NAME
    #endif
#endif
/* End of compilation time checks to be sure the defines are well defined */

#define NV_COUNTER_SIZE  (1)
#define INIT_VALUE_SIZE  NV_COUNTER_SIZE
#define NUM_NV_COUNTERS  (4)
                         

#define NV_COUNTERS_INITIALIZED 0xC0DE0042U

#define VCODE   0x77100000U


/**
 * \brief Struct representing the NV counter data in flash.
 */
struct nv_counters_t {
    uint32_t counters[NUM_NV_COUNTERS]; /**< Array of NV counters */
    uint32_t init_value; /**< Watermark to indicate if the NV counters have been
                          *   initialised
                          */
};

enum tfm_plat_err_t tfm_plat_init_nv_counter(void)
{
    int32_t timeout;

    if(FVC->CTL & FVC_CTL_INIT_Msk)
        return TFM_PLAT_ERR_SUCCESS;
    else
    {
        /* Inital firmware version counter */
        //FVC->CTL = VCODE | FVC_CTL_MONOEN_Msk | FVC_CTL_INIT_Msk;
        FVC->CTL = VCODE | FVC_CTL_INIT_Msk;
        
        /* Check if init ok */
        timeout = 0x1000;
        while(1)
        {
            if(FVC->STS & FVC_STS_RDY_Msk)
                break;
            if(timeout-- <= 0)
            {
                /* FVC init timeout */
                return TFM_PLAT_ERR_SYSTEM_ERR;
            }
        }
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_read_nv_counter(enum tfm_nv_counter_t counter_id,
                                             uint32_t size, uint8_t *val)
{

    if (size != NV_COUNTER_SIZE) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    switch(counter_id)
    {
    case PLAT_NV_COUNTER_0:
        return FVC->NVC[0];
    case PLAT_NV_COUNTER_1:
        return FVC->NVC[1];
    case PLAT_NV_COUNTER_2:
        return TFM_PLAT_ERR_SYSTEM_ERR;
    case PLAT_NV_COUNTER_3:
        return FVC->NVC[4];
    case PLAT_NV_COUNTER_4:
        return FVC->NVC[5];
    default:
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_set_nv_counter(enum tfm_nv_counter_t counter_id,
                                            uint32_t value)
{
    int32_t timeout;

    switch(counter_id)
    {
    case PLAT_NV_COUNTER_0:
        if(value < FVC->NVC[0])
            return TFM_PLAT_ERR_INVALID_INPUT;
        FVC->NVC[0] = (FVC->NVC[0] << 16) | value;
        break;
    case PLAT_NV_COUNTER_1:
        if(value < FVC->NVC[1])
            return TFM_PLAT_ERR_INVALID_INPUT;
        FVC->NVC[1] = (FVC->NVC[1] << 16) | value;
        break;
    case PLAT_NV_COUNTER_2:
        return TFM_PLAT_ERR_SYSTEM_ERR;
    case PLAT_NV_COUNTER_3:
        if(value < FVC->NVC[4])
            return TFM_PLAT_ERR_INVALID_INPUT;
        FVC->NVC[4] = (FVC->NVC[4] << 16) | value;
        break;
    case PLAT_NV_COUNTER_4:
        if(value < FVC->NVC[5])
            return TFM_PLAT_ERR_INVALID_INPUT;
        FVC->NVC[5] = (FVC->NVC[5] << 16) | value;
        break;
    default:
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Waiting for ready */
    timeout = 0x1000;
    while(1)
    {
        if((FVC->STS & FVC_STS_BUSY_Msk) == 0)
            break;
        if(timeout-- <= 0)
            return TFM_PLAT_ERR_SUCCESS;
    }

    return TFM_PLAT_ERR_SUCCESS;

}

enum tfm_plat_err_t tfm_plat_increment_nv_counter(
                                           enum tfm_nv_counter_t counter_id)
{
    uint32_t security_cnt;
    enum tfm_plat_err_t err;

    err = tfm_plat_read_nv_counter(counter_id,
                                   sizeof(security_cnt),
                                   (uint8_t *)&security_cnt);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    if (security_cnt == UINT32_MAX) {
        return TFM_PLAT_ERR_MAX_VALUE;
    }

    return tfm_plat_set_nv_counter(counter_id, security_cnt + 1u);
}
