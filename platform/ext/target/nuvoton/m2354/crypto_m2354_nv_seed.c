/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 * Copyright (c) 2020 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_plat_crypto_dummy_nv_seed.h"
#include "NuMicro.h"

#ifndef MBEDTLS_ENTROPY_BLOCK_SIZE
# define MBEDTLS_ENTROPY_BLOCK_SIZE 64
#endif

#if defined(MBEDTLS_ENTROPY_SHA512_ACCUMULATOR)
unsigned char seed_value[MBEDTLS_ENTROPY_BLOCK_SIZE] = {0};
#else
unsigned char seed_value[MBEDTLS_ENTROPY_BLOCK_SIZE] = {0};
#endif

int tfm_plat_crypto_create_entropy_seed(void)
{
    int32_t i, timeout = 0x1000;
    uint32_t u32Reg;

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
	for(i = 0; i < MBEDTLS_ENTROPY_BLOCK_SIZE; i++)
	{
		TRNG->CTL = TRNG_CTL_TRNGEN_Msk | u32Reg;
		while((TRNG->CTL & TRNG_CTL_DVIF_Msk) == 0);
		seed_value[i] = TRNG->DATA;
	}

    return tfm_plat_crypto_nv_seed_write(seed_value, MBEDTLS_ENTROPY_BLOCK_SIZE);
}
