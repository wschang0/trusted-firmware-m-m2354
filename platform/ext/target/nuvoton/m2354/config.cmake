#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2021 Nuvoton Technology Corp. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(PS_MAX_ASSET_SIZE       512    CACHE STRING    "The maximum asset size to be stored in the Protected Storage area")
set(PS_NUM_ASSETS           12     CACHE STRING    "The maximum number of assets to be stored in the Protected Storage area")
set(ITS_NUM_ASSETS          12     CACHE STRING    "The maximum number of assets to be stored in the Internal Trusted Storage area")
set(CRYPTO_HW_ACCELERATOR   ON     CACHE BOOL      "Whether to enable the crypto hardware accelerator on supported platforms")
set(CRYPTO_NV_SEED          OFF    CACHE BOOL      "Use stored NV seed to provide entropy")

set(PLATFORM_DUMMY_NV_SEED     FALSE  CACHE BOOL   "Use dummy NV seed implementation. Should not be used in production.")
set(PLATFORM_DUMMY_CRYPTO_KEYS FALSE  CACHE BOOL   "Use dummy crypto keys. Should not be used in production.")
set(PLATFORM_DUMMY_NV_COUNTERS FALSE  CACHE BOOL   "Use dummy nv counter implementation. Should not be used in production.")
set(PLATFORM_DUMMY_ROTPK       FALSE  CACHE BOOL   "Use dummy root of trust public key. Dummy key is the public key for the default keys in bl2. Should not be used in production.")
set(PLATFORM_DUMMY_IAK         FALSE  CACHE BOOL   "Use dummy initial attestation_key. Should not be used in production.")

set(TFM_NS_CLIENT_IDENTIFICATION    OFF)