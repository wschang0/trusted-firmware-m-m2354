/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include <stdint.h>
#include "tfm_attest_hal.h"
#include "tfm_plat_boot_seed.h"
#include "tfm_plat_device_id.h"

/*!
 * \def BOOT_SEED
 *
 * \brief Fixed value for boot seed used for test.
 */
#define BOOT_SEED   0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, \
                    0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, \
                    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, \
                    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF

static const uint8_t boot_seed[BOOT_SEED_SIZE] = {BOOT_SEED};

/* Example verification service URL for initial attestation token */
static const char verification_service_url[] = "www.trustedfirmware.org";

/* Example profile definition document for initial attestation token */
static const char attestation_profile_definition[] = "PSA_IOT_PROFILE_1";

static const uint8_t example_ean_13[] = "060456527282910010";

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
    return TFM_SLC_SECURED;
}

const char *
tfm_attest_hal_get_verification_service(uint32_t *size)
{
    *size = sizeof(verification_service_url) - 1;

    return verification_service_url;
}

const char *
tfm_attest_hal_get_profile_definition(uint32_t *size)
{
    *size = sizeof(attestation_profile_definition) - 1;

    return attestation_profile_definition;
}

/**
 * \brief Copy data in source buffer to the destination buffer
 *
 * \param[out]  p_dst  Pointer to destation buffer
 * \param[in]   p_src  Pointer to source buffer
 * \param[in]   size   Length of data to be copied
 */
static inline void copy_buf(uint8_t *p_dst, const uint8_t *p_src, size_t size)
{
    uint32_t i;

    for (i = size; i > 0; i--) {
        *p_dst = *p_src;
        p_src++;
        p_dst++;
    }
}

enum tfm_plat_err_t tfm_plat_get_boot_seed(uint32_t size, uint8_t *buf)
{
    /* FixMe: - This getter function must be ported per target platform.
     *        - Platform service shall provide an API to further interact this
     *          getter function to retrieve the boot seed.
     */

    uint8_t *p_dst = buf;
    const uint8_t *p_src = boot_seed;

    if (size != BOOT_SEED_SIZE) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    copy_buf(p_dst, p_src, size);

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_implementation_id(uint32_t *size,
                                                   uint8_t  *buf)
{
    uint32_t u32Buf[6];

    /* return CID:DID:UID */

    if(*size < 24) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    copy_buf(buf, p_impl_id, impl_id_size);
    *size = impl_id_size;

    /* Get UID */
    FMC->ISPCTL |= FMC_ISPCTL_ISPEN_Msk;
    for(i = 0; i < 6; i++)
    {
        if(i == 0)
        {
            FMC->ISPCMD = FMC_ISPCMD_READ_CID;
            FMC->ISPADDR = 0;
        }
        else if(i == 1)
        {
            FMC->ISPCMD = FMC_ISPCMD_READ_DID;
            FMC->ISPADDR = 0;
        }
        else
        {
            FMC->ISPCMD = FMC_ISPCMD_READ_UID;
            FMC->ISPADDR = (i - 2) * 4;
        }

        FMC->ISPTRG = 1;
        while(FMC->ISPTRG)
        {
            if(timeout-- <= 0)
                return TFM_PLAT_ERR_SYSTEM_ERR;
        }

        u32Buf[i] = FMC->ISPDAT;
    }

    copy_buf(buf, u32Buf, 24);
    *size = 24;

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_hw_version(uint32_t *size, uint8_t *buf)
{
    const uint8_t *p_hw_version = example_ean_13;
    uint32_t hw_version_size = sizeof(example_ean_13) - 1;

    if (*size < hw_version_size) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    copy_buf(buf, p_hw_version, hw_version_size);
    *size = hw_version_size;

    return TFM_PLAT_ERR_SUCCESS;
}
