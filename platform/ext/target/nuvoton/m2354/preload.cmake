#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# preload.cmake is used to set things that related to the platform that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practise this is normally compiler definitions and
# variables related to hardware.

# Set architecture and CPU
set(TFM_SYSTEM_PROCESSOR cortex-m23)
set(TFM_SYSTEM_ARCHITECTURE armv8-m.base)
set(CRYPTO_HW_ACCELERATOR_TYPE nuvoton)
set(NVT_FLASH_OTP                       OFF         CACHE BOOL       "Use Flash to implement OTP")

add_compile_definitions(
    $<$<BOOL:${NVT_FLASH_OTP}>:NVT_FLASH_OTP>
)

            
