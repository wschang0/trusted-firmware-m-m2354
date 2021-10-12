/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  Copyright (c) 2020 Nuvoton Technology Corp. All rights reserved.
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file implements STMicroelectronics SHA1 with HW services based on mbed TLS API
 */
/*
 *  The SHA-1 standard was published by NIST in 1993.
 *
 *  http://www.itl.nist.gov/fipspubs/fip180-1.htm
 */

/* Includes ------------------------------------------------------------------*/
#include "mbedtls/sha1.h"

#if defined(MBEDTLS_SHA1_C)
#if defined(MBEDTLS_SHA1_ALT)
#include <string.h>
#include "mbedtls/platform.h"
#include "mbedtls/platform_util.h"

#define SHA1_VALIDATE_RET(cond)                             \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_SHA1_BAD_INPUT_DATA )
#define SHA1_VALIDATE(cond)  MBEDTLS_INTERNAL_VALIDATE( cond )

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

static void mbedtls_zeroize(void *v, size_t n)
{
    volatile unsigned char *p = (unsigned char *)v;
    while (n--)
    {
        *p++ = 0;
    }
}

void mbedtls_sha1_init(mbedtls_sha1_context *ctx)
{
    SHA1_VALIDATE( ctx != NULL );

    mbedtls_zeroize(ctx, sizeof(mbedtls_sha1_context));
}

void mbedtls_sha1_free(mbedtls_sha1_context *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    mbedtls_zeroize(ctx, sizeof(mbedtls_sha1_context));
}

void mbedtls_sha1_clone(mbedtls_sha1_context *dst,
                          const mbedtls_sha1_context *src)
{
    SHA1_VALIDATE( dst != NULL );
    SHA1_VALIDATE( src != NULL );

    *dst = *src;
}

int mbedtls_sha1_starts_ret(mbedtls_sha1_context *ctx)
{
    SHA1_VALIDATE_RET(ctx != NULL);

    ctx->first = 1;
    
    /* Common register settings */
    ctx->ctl = CRPT_HMAC_CTL_DMAEN_Msk | (SHA_MODE_SHA1 << CRPT_HMAC_CTL_OPMODE_Pos) |
        CRPT_HMAC_CTL_INSWAP_Msk | CRPT_HMAC_CTL_OUTSWAP_Msk;

    /* Common DMA settings */
    CRPT->HMAC_SADDR = (uint32_t)&ctx->buffer[0];
    CRPT->HMAC_DMACNT = NU_SHA256_BLOCK_SIZE;

    return 0;
}

int mbedtls_internal_sha1_process( mbedtls_sha1_context *ctx, const unsigned char data[ST_SHA1_BLOCK_SIZE] )
{
    ARG_UNUSED(ctx);
    ARG_UNUSED(data);

    return MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED;
}

int mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen)
{
    int32_t timeout = 0x1000;
    uint32_t ofs = 0;

    SHA256_VALIDATE_RET(ctx != NULL);
    SHA256_VALIDATE_RET(ilen == 0 || input != NULL);

    /* Process all data block by block. the remain data will leave to next time. */
    while(ilen > 0)
    {
        /* Process the buffer if it is full */
        if(ctx->buffer_len >= NU_SHA1_BLOCK_SIZE)
        {
            CRPT->HMAC_DMACNT = NU_SHA1_BLOCK_SIZE;
            if(ctx->first)
            {
                CRPT->HMAC_CTL = ctx->ctl | CRPT_HMAC_CTL_START_Msk | CRPT_HMAC_CTL_DMACSCAD_Msk | CRPT_HMAC_CTL_DMAFIRST_Msk;
                ctx->first = 0;

            }
            else
            {
                CRPT->HMAC_CTL = ctx->ctl | CRPT_HMAC_CTL_START_Msk | CRPT_HMAC_CTL_DMACSCAD_Msk;
            }

            /* Waiting for calculation done */
            while((CRPT->INTSTS & CRPT_INTSTS_HMACIF_Msk) == 0)
            {
                if(timeout-- <= 0)
                    break;
            }

            ctx->buffer_len = 0;

            if(timeout == 0)
                return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

            CRPT->INTSTS = CRPT_INTSTS_HMACIF_Msk;

        }
        else
        {
            /* prepare input block */
            if(ilen > NU_SHA1_BLOCK_SIZE - ctx->buffer_len)
            {
                memcpy(ctx->buffer + ctx->buffer_len, input + ofs, NU_SHA1_BLOCK_SIZE - ctx->buffer_len);
                ilen -= NU_SHA1_BLOCK_SIZE - ctx->buffer_len;
                ofs += NU_SHA1_BLOCK_SIZE - ctx->buffer_len;
                ctx->buffer_len = NU_SHA1_BLOCK_SIZE;
            }
            else
            {
                memcpy(ctx->buffer + ctx->buffer_len, input + ofs, ilen);
                ctx->buffer_len += ilen;
                ilen = 0;
            }
        }
    }

    return 0;
}

int mbedtls_sha1_finish_ret(mbedtls_sha1_context *ctx, unsigned char output[32])
{
    int32_t timeout = 0x1000;

    SHA1_VALIDATE_RET( ctx != NULL );
    SHA1_VALIDATE_RET( (unsigned char *)output != NULL );

    /* Process the buffer if it is not emtpy */
    if(ctx->buffer_len > 0)
    {
        CRPT->HMAC_DMACNT = ctx->buffer_len;

        if(ctx->first)
        {
            /* If it is first, it means we don't need to do casecade */
            CRPT->HMAC_CTL = ctx->ctl | CRPT_HMAC_CTL_START_Msk | CRPT_HMAC_CTL_DMALAST_Msk;
        }
        else
        {
            CRPT->HMAC_CTL = ctx->ctl | CRPT_HMAC_CTL_START_Msk | CRPT_HMAC_CTL_DMACSCAD_Msk | CRPT_HMAC_CTL_DMALAST_Msk;
        }

        /* Waiting for calculation done */
        while((CRPT->INTSTS & CRPT_INTSTS_HMACIF_Msk) == 0)
        {
            if(timeout-- <= 0)
                break;
        }
        ctx->buffer_len = 0;

        if(timeout == 0)
            return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;

        CRPT->INTSTS = CRPT_INTSTS_HMACIF_Msk;

    }

    /* return SHA results */
    memcpy(output, (uint8_t*)&CRPT->HMAC_DGST[0], 20);

    return 0;
}

#endif /* MBEDTLS_SHA1_ALT*/
#endif /* MBEDTLS_SHA1_C */
