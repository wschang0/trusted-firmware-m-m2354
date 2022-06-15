/*
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 * Copyright (C) 2022, Nuvoton Technology Corporation, All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stddef.h>
#include <string.h>
#include "crypto_hw.h"
#include "psa/crypto_types.h"
#include "tfm_plat_crypto_keys.h"
#include "tfm_plat_otp.h"
#include "mbedtls/hkdf.h"
#include "NuMicro.h"


int crypto_hw_accelerator_huk_derive_key(const uint8_t* label, size_t label_size, const uint8_t* context, size_t context_size, uint8_t* key, size_t key_size)
{
    int err = -1;
    uint8_t keybuf[32] = {0};

    if(key == NULL) 
        return -1;

    if((label_size > 0) && (label == NULL))
        return -1;

    if((context_size > 0) && (context == NULL))
        return -1;
    
    /* get huk */
    err = tfm_plat_otp_read(PLAT_OTP_ID_HUK, sizeof(keybuf), keybuf);
    if(err)
    {
        goto lexit;
    }

    /* Generate key by hkdf */
    err = mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), label, label_size, keybuf, sizeof(keybuf), context, context_size, key, key_size);
    if(err)
    {
        goto lexit;
    }

lexit:
    /* clean key buffer */
    memset(keybuf, 0, sizeof(keybuf));

    return err;
}

