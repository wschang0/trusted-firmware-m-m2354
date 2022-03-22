#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited. All rights reserved.
# Copyright (c) 2022 Nuvoton Technology Corp. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})


#========================= Platform region defs ===============================#
target_include_directories(platform_region_defs
    INTERFACE
        partition
)

#========================= Platform common defs ===============================#

# Specify the location of platform specific build dependencies.
target_sources(tfm_s
    PRIVATE
        $<$<C_COMPILER_ID:ARMClang>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/armclang/startup_cmsdk_m2354_s.s>
        $<$<C_COMPILER_ID:GNU>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/gcc/startup_cmsdk_m2354_s.S>
        $<$<C_COMPILER_ID:IAR>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/iar/startup_cmsdk_m2354_s.s>
)
target_add_scatter_file(tfm_s
    $<$<AND:$<VERSION_LESS:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:ARMClang>>:${CMAKE_SOURCE_DIR}/platform/ext/common/armclang/tfm_common_s.sct>
    $<$<AND:$<VERSION_LESS:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:GNU>>:${CMAKE_SOURCE_DIR}/platform/ext/common/gcc/tfm_common_s.ld>
    $<$<AND:$<VERSION_LESS:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:IAR>>:${CMAKE_SOURCE_DIR}/platform/ext/common/iar/tfm_common_s.icf>
)

if(NS)
    target_sources(tfm_ns
        PRIVATE
            $<$<C_COMPILER_ID:ARMClang>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/armclang/startup_cmsdk_m2354_ns.s>
            $<$<C_COMPILER_ID:GNU>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/gcc/startup_cmsdk_m2354_ns.S>
            $<$<C_COMPILER_ID:IAR>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/iar/startup_cmsdk_m2354_ns.s>
    )
    target_add_scatter_file(tfm_ns
        $<$<C_COMPILER_ID:ARMClang>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/armclang/m2354_ns.sct>
        $<$<C_COMPILER_ID:GNU>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/gcc/m2354_ns.ld>
        $<$<C_COMPILER_ID:IAR>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/iar/m2354_ns.icf>
    )
    target_link_libraries(CMSIS_5_tfm_ns
        INTERFACE
            CMSIS_5_RTX_V8MBN
    )
endif()

if(BL2)
    target_sources(bl2
        PRIVATE
            $<$<C_COMPILER_ID:ARMClang>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/armclang/startup_cmsdk_m2354_bl2.s>
            $<$<C_COMPILER_ID:GNU>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/gcc/startup_cmsdk_m2354_bl2.S>
            $<$<C_COMPILER_ID:IAR>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/iar/startup_cmsdk_m2354_bl2.s>
    )

    target_add_scatter_file(bl2
        $<$<C_COMPILER_ID:ARMClang>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/armclang/m2354_bl2.sct>
        $<$<C_COMPILER_ID:GNU>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/gcc/m2354_bl2.ld>
        $<$<C_COMPILER_ID:IAR>:${CMAKE_CURRENT_SOURCE_DIR}/device/source/iar/m2354_bl2.icf>
    )
endif()

#========================= Platform Secure ====================================#

target_include_directories(platform_s
    PUBLIC
        .
        ../common/
        ../common/cmsis_drivers/config
        ../common/native_drivers
        ../common/third_party/FatFs/src
        partition
        device/include
        device/config
        bsp/Device/Nuvoton/M2354/Include
        bsp/Library/StdDriver/inc
        ../common/bsp/Include
        ../common/bsp/StdDriver/inc
        ${PLATFORM_DIR}/ext/cmsis
        ${PLATFORM_DIR}/ext/driver
)

target_sources(platform_s
    PRIVATE
        $<$<NOT:$<BOOL:${PLATFORM_DUMMY_CRYPTO_KEYS}>>:${CMAKE_CURRENT_SOURCE_DIR}/crypto_keys.c>
        $<$<NOT:$<BOOL:${PLATFORM_DUMMY_ROTPK}>>:${CMAKE_CURRENT_SOURCE_DIR}/tfm_rotpk.c>
        $<$<NOT:$<BOOL:${PLATFORM_DUMMY_IAK}>>:${CMAKE_CURRENT_SOURCE_DIR}/tfm_initial_attestation_key_material.c>
        $<$<AND:$<NOT:$<BOOL:${ATTEST_TEST_GET_PUBLIC_KEY}>>,$<NOT:$<BOOL:${SYMMETRIC_INITIAL_ATTESTATION}>>,$<NOT:$<BOOL:${PLATFORM_DUMMY_IAK}>>,$<BOOL:${TEST_S}>>:ext/common/template/tfm_initial_attest_pub_key.c>
        device/source/system_core_init.c
        device/source/device_definition.c
        ../common/cmsis_drivers/Driver_Flash.c
        ../common/cmsis_drivers/Driver_USART.c
        ../common/native_drivers/arm_uart_drv.c
        ../common/native_drivers/mpu_armv8m_drv.c
        ../common/native_drivers/timer_cmsdk_drv.c
        bsp/Library/StdDriver/src/timer.c
        bsp/Library/StdDriver/src/fmc.c
        bsp/Library/StdDriver/src/rng.c
        ../common/services/src/tfm_platform_system.c
        target_cfg.c
        ../common/attest_hal.c
        ../common/spm_hal.c
        ../common/tfm_hal_isolation.c
        ../common/tfm_hal_platform.c
        ${PLATFORM_DIR}/ext/common/uart_stdout.c
        ${PLATFORM_DIR}/ext/common/tfm_platform.c
        ${CMAKE_SOURCE_DIR}/platform/ext/common/tfm_hal_isolation_mpu_v8m.c
        ${CMAKE_SOURCE_DIR}/platform/ext/common/tfm_hal_nvic.c
        $<$<BOOL:${TFM_PARTITION_PLATFORM}>:${CMAKE_CURRENT_SOURCE_DIR}/plat_test.c>
)

target_compile_options(platform_s
    PUBLIC
        ${COMPILER_CMSE_FLAG}
        $<$<COMPILE_LANGUAGE:C>:-gdwarf-2>
)

#========================= Platform Non-Secure ================================#

target_sources(platform_ns
    PRIVATE
        $<$<AND:$<NOT:$<BOOL:${ATTEST_TEST_GET_PUBLIC_KEY}>>,$<NOT:$<BOOL:${SYMMETRIC_INITIAL_ATTESTATION}>>,$<NOT:$<BOOL:${PLATFORM_DUMMY_IAK}>>,$<BOOL:${TEST_S}>>:ext/common/template/tfm_initial_attest_pub_key.c>
        ../common/native_drivers/arm_uart_drv.c
        ../common/cmsis_drivers/Driver_USART.c
        device/source/device_definition.c
        device/source/system_core_init.c
)

target_include_directories(platform_ns
    PUBLIC
        .
        ../common/
        ${PLATFORM_DIR}/..
        device/config
        device/include
        ../common/native_drivers
        bsp/Device/Nuvoton/M2354/Include
        bsp/Library/StdDriver/inc
        ../common/bsp/StdDriver/inc
        ../common/bsp/Include
        ../common/cmsis_drivers/config
)

target_compile_options(platform_ns
    PUBLIC
        $<$<COMPILE_LANGUAGE:C>:-gdwarf-2>
)

#========================= Platform BL2 =======================================#

if(BL2)
    target_sources(platform_bl2
        PRIVATE
            $<$<NOT:$<BOOL:${PLATFORM_DUMMY_ROTPK}>>:${CMAKE_CURRENT_SOURCE_DIR}/tfm_rotpk.c>
            $<$<NOT:$<BOOL:${PLATFORM_DUMMY_IAK}>>:${CMAKE_CURRENT_SOURCE_DIR}/tfm_initial_attestation_key_material.c>
            device/source/device_definition.c
            device/source/system_core_init.c
            bsp/Library/StdDriver/src/fmc.c
            bsp/Library/StdDriver/src/clk.c
            bsp/Library/StdDriver/src/rng.c
            ../common/boot_hal.c
            ../common/cmsis_drivers/Driver_Flash.c
            ../common/native_drivers/arm_uart_drv.c
            ../common/cmsis_drivers/Driver_SD.c
            ../common/cmsis_drivers/Driver_USART.c
            ../common/bsp/StdDriver/src/sdh.c
            ../common/third_party/FatFs/src/ff.c
            ../common/SDGlue.c
            ../common/diskio.c
    )
    target_include_directories(platform_bl2
        PUBLIC
            partition
            bsp/Device/Nuvoton/M2354/Include
            bsp/Library/StdDriver/inc
            ../common/bsp/Include
            ../common/bsp/StdDriver/inc
            device/include
            device/config
            ../common/cmsis_drivers/config
            ../common/third_party/FatFs/src
        PRIVATE
            .
            ./
            ../common/
            ${PLATFORM_DIR}/..
            ../common/native_drivers
    )
    target_compile_options(platform_bl2
        PUBLIC
            ${COMPILER_CMSE_FLAG}
    )

    target_compile_definitions(platform_bl2
        PRIVATE
            USE_SDH
    )

    target_compile_definitions(bootutil
    PRIVATE
        MBEDTLS_ALLOW_PRIVATE_ACCESS
    )

endif()


#========================= For Nonsecure Regression =======================================#
add_library(tfm_regression_ns 
    STATIC
        $<$<AND:$<NOT:$<BOOL:${ATTEST_TEST_GET_PUBLIC_KEY}>>,$<NOT:$<BOOL:${SYMMETRIC_INITIAL_ATTESTATION}>>,$<NOT:$<BOOL:${PLATFORM_DUMMY_IAK}>>,$<BOOL:${TEST_S}>>:${CMAKE_SOURCE_DIR}/platform/ext/common/template/tfm_initial_attest_pub_key.c>
        ../common/native_drivers/arm_uart_drv.c
        ../common/cmsis_drivers/Driver_USART.c
        device/source/device_definition.c
        device/source/system_core_init.c
        
)

target_link_libraries(tfm_regression_ns
    PUBLIC 
    tfm_api_ns
    tfm_qcbor_ns
    tfm_ns_log
    tfm_test_suite_qcbor_ns
    tfm_qcbor_test
    tfm_t_cose_ns
    tfm_t_cose_test
    tfm_test_suite_attestation_ns
    tfm_test_suite_crypto_ns
    tfm_test_suite_ipc_ns
    tfm_test_suite_platform_ns
    tfm_test_suite_t_cose_ns
    tfm_test_suite_core_ns
    tfm_test_suite_it_ns
    tfm_test_suite_ps_ns
    tfm_ns_integration_test
)

target_include_directories(tfm_regression_ns
    PUBLIC
        partition
        ${PLATFORM_DIR}/ext/driver
        ${PLATFORM_DIR}/ext/cmsis
        bsp/Device/Nuvoton/M2354/Include
        ../common/bsp/Include
        ../common/bsp/StdDriver/inc
        bsp/Library/StdDriver/inc
        device/config
        ../common/native_drivers
        ../common/cmsis_drivers/config
        device/include
)

target_compile_options(tfm_regression_ns
    PUBLIC
        $<$<COMPILE_LANGUAGE:C>:-gdwarf-2>
)

