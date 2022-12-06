/*
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include "tfm_plat_crypto_keys.h"
#include "tfm_plat_otp.h"
/**
 * \file tfm_rotpk.c
 *
 * This file contains the hash value (SHA256) of the public parts of the
 * firmware signing keys in bl2/ext/mcuboot folder (*.pem files).
 * This simulates when the hash of the Root of Trust Public Key is programmed
 * to an immutable device memory to be able to validate the image verification
 * key.
 *
 * \note These key-hash values must be provisioned to the SoC during the
 *       production, independently from firmware binaries. This solution
 *       (hard-coded key-hash values in firmware) is not suited for use in
 *       production!
 */

#if defined(BL2)

enum tfm_plat_err_t
tfm_plat_get_rotpk_hash(uint8_t image_id,
                        uint8_t *rotpk_hash,
                        uint32_t *rotpk_hash_size)
{
    if(*rotpk_hash_size < ROTPK_HASH_LEN) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    if (image_id >= MCUBOOT_IMAGE_NUMBER) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return tfm_plat_otp_read(PLAT_OTP_ID_BL2_ROTPK_0 + image_id, (size_t)*rotpk_hash_size, rotpk_hash);
}

#endif /* BL2 */
