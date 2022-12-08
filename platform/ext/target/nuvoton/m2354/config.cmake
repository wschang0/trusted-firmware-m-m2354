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

set(TFM_NS_CLIENT_IDENTIFICATION    OFF)

set(MBEDCRYPTO_BUILD_TYPE               minsizerel  CACHE STRING    "Build type of Mbed Crypto library")
set(TFM_DUMMY_PROVISIONING              OFF         CACHE BOOL      "Provision with dummy values. NOT to be used in production")              
set(PLATFORM_DEFAULT_OTP_WRITEABLE      ON          CACHE BOOL      "Use on chip flash with write support")
set(PLATFORM_DEFAULT_NV_COUNTERS        OFF         CACHE BOOL      "Use default nv counter implementation.")
set(ATTEST_TEST_GET_PUBLIC_KEY          OFF         CACHE BOOL      "Retrieve initial attestation public key for initial attestation test in runtime")
set(PS_CRYPTO_AEAD_ALG                  PSA_ALG_GCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
set(NVT_ENABLE_ETM_DEBUG                OFF         CACHE BOOL      "Enable ETM debug interface")
set(NVT_USE_HIRC48M                     OFF         CACHE BOOL      "Set sytem clock as HIRC48M")
