/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2020 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#endif

#include "NuMicro.h"

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

int mbedtls_hardware_poll(void* data, unsigned char* output, size_t len, size_t* olen);

int mbedtls_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    int32_t i;
    int32_t timeout = 0x1000;
    uint32_t u32Reg;
    int32_t err = 0;


    ARG_UNUSED(data);

    if(NULL == output)
        return -1;

    if(NULL == olen)
        return -1;

    if(0 == len)
        return -1;

    /* Generate the seed by TRNG */

    CLK->AHBCLK |= CLK_AHBCLK_CRPTCKEN_Msk;
    CLK->APBCLK1 |= CLK_APBCLK1_TRNGCKEN_Msk;
    CLK->APBCLK0 |= CLK_APBCLK0_RTCCKEN_Msk;

    RTC->LXTCTL |= (RTC_LXTCTL_C32KSEL_Msk | RTC_LXTCTL_LIRC32KEN_Msk); //To use LIRC32K

    TRNG->ACT |= TRNG_ACT_ACT_Msk;
    /* Waiting for ready */
    i = 0;
    while((TRNG->CTL & TRNG_CTL_READY_Msk) == 0)
    {
        if(i++ > timeout)
        {
            /* TRNG ready timeout */
            return -1;
        }
    }

    TRNG->CTL = (0 << TRNG_CTL_CLKPSC_Pos);

    u32Reg = TRNG->CTL;
    for(i = 0; i < (int32_t)len; i++)
    {
        TRNG->CTL = TRNG_CTL_TRNGEN_Msk | u32Reg;
        while((TRNG->CTL & TRNG_CTL_DVIF_Msk) == 0);
        output[i] = TRNG->DATA;
    }

    *olen = len;

    return err;
}
