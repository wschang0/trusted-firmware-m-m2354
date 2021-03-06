/*
 * Copyright (c) 2018-2021, Arm Limited. All rights reserved.
 * Copyright (c) 2022, Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include <stdint.h>
#include "tfm_attest_hal.h"
#include "tfm_plat_boot_seed.h"
#include "tfm_plat_device_id.h"
#include "tfm_plat_otp.h"
#include "tfm_strnlen.h"

#include "NuMicro.h"

typedef union {
    uint32_t id;
    uint8_t code[4];
} ID_T;

#define BOOT_SEED_INIT_KEY  (0xa792542e)
static uint8_t boot_seed[32] = { 0 };
static uint32_t boot_seed_init = 0;

static enum tfm_security_lifecycle_t map_otp_lcs_to_tfm_slc(enum plat_otp_lcs_t lcs) {
    switch (lcs) {
        case PLAT_OTP_LCS_ASSEMBLY_AND_TEST:
            return TFM_SLC_ASSEMBLY_AND_TEST;
        case PLAT_OTP_LCS_PSA_ROT_PROVISIONING:
            return TFM_SLC_PSA_ROT_PROVISIONING;
        case PLAT_OTP_LCS_SECURED:
            return TFM_SLC_SECURED;
        case PLAT_OTP_LCS_DECOMMISSIONED:
            return TFM_SLC_DECOMMISSIONED;
        case PLAT_OTP_LCS_UNKNOWN:
        default:
            return TFM_SLC_UNKNOWN;
    }
}

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
    enum plat_otp_lcs_t otp_lcs;
    enum tfm_plat_err_t err;

    err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(otp_lcs), (uint8_t*)&otp_lcs);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return TFM_SLC_UNKNOWN;
    }

    return map_otp_lcs_to_tfm_slc(otp_lcs);
}

enum tfm_plat_err_t
tfm_attest_hal_get_verification_service(uint32_t *size, uint8_t *buf)
{
    enum tfm_plat_err_t err;
    size_t otp_size;

    err = tfm_plat_otp_read(PLAT_OTP_ID_VERIFICATION_SERVICE_URL, *size, buf);
    if(err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    err =  tfm_plat_otp_get_size(PLAT_OTP_ID_VERIFICATION_SERVICE_URL, &otp_size);
    if(err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    *size = tfm_strnlen((char*)buf, otp_size);

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t
tfm_attest_hal_get_profile_definition(uint32_t *size, uint8_t *buf)
{
    enum tfm_plat_err_t err;
    size_t otp_size;

    err = tfm_plat_otp_read(PLAT_OTP_ID_PROFILE_DEFINITION, *size, buf);
    if(err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    err =  tfm_plat_otp_get_size(PLAT_OTP_ID_PROFILE_DEFINITION, &otp_size);
    if(err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    *size = tfm_strnlen((char*)buf, otp_size);

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_boot_seed(uint32_t size, uint8_t *buf)
{
    extern int mbedtls_hardware_poll(void* data, unsigned char* output, size_t len, size_t * olen);
    uint32_t osize;
    int i;

    /* Boot seed is come from TRNG when system reset */

    if(boot_seed_init != BOOT_SEED_INIT_KEY)
    {
        mbedtls_hardware_poll(0, boot_seed, 32, &osize);
        boot_seed_init = BOOT_SEED_INIT_KEY;
    }

    if(size > 32)
    {
        size = 32;
    }

    for(i = 0; i < size; i++)
    {
        buf[i] = boot_seed[i];
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_implementation_id(uint32_t *size,
                                                   uint8_t  *buf)
{

    ID_T uid;
    int32_t i, j, timeout;

    /* The UID of M2354 is used as implementation id of attenstation token */

    FMC_ISP->ISPCTL |= FMC_ISPCTL_ISPEN_Msk;

    FMC->ISPCMD = FMC_ISPCMD_READ_UID;
    for(i = 0; i < 4; i++)
    {
        FMC->ISPADDR = i * 4;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
        timeout = 0x10000;
        while(FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk) {
            if(timeout-- <= 0)
            {
                return TFM_PLAT_ERR_SYSTEM_ERR;
            }
        }
        uid.id = FMC->ISPDAT;

        for(j = 0; j < 4; j++)
        {
            buf[i * 4 + j] = uid.code[j];
        }
    }

    *size = 16;

    return TFM_PLAT_ERR_SUCCESS;
}


enum tfm_plat_err_t tfm_plat_get_hw_version(uint32_t* size, uint8_t* buf)
{
    enum tfm_plat_err_t err;
    size_t otp_size;

    err = tfm_plat_otp_read(PLAT_OTP_ID_HW_VERSION, *size, buf);
    if(err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    err = tfm_plat_otp_get_size(PLAT_OTP_ID_HW_VERSION, &otp_size);
    if(err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    *size = tfm_strnlen((char*)buf, otp_size);

    return TFM_PLAT_ERR_SUCCESS;
}
