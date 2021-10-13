#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(PS_MAX_ASSET_SIZE       512    CACHE STRING    "The maximum asset size to be stored in the Protected Storage area")
set(PS_NUM_ASSETS           12     CACHE STRING    "The maximum number of assets to be stored in the Protected Storage area")
set(ITS_NUM_ASSETS          12     CACHE STRING    "The maximum number of assets to be stored in the Internal Trusted Storage area")
set(PLATFORM_DUMMY_NV_SEED  FALSE  CACHE BOOL      "Use dummy NV seed implementation. Should not be used in production.")
set(CRYPTO_HW_ACCELERATOR   ON     CACHE BOOL      "Whether to enable the crypto hardware accelerator on supported platforms")
