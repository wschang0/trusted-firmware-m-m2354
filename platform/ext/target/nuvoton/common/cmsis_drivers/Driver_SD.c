/*
 * Copyright (c) 2013-2018 ARM Limited. All rights reserved.
 * Copyright (c) 2020 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "Driver_Flash.h"
#include "NuMicro.h"
#include "RTE_Device.h"
#include "flash_layout.h"
#include "region_defs.h"
#include "ff.h"

#define printf(...)

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

/* Driver version */
#define SD_DRV_VERSION    ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0)


static FIL *s_pfile = NULL;
#if (MCUBOOT_IMAGE_NUMBER == 2)
static int32_t s_nsflag = 0;
#endif
static __attribute__((aligned(4))) FIL fobj;
static __attribute__((aligned(4))) FILINFO finfo;

unsigned long get_fattime(void)
{
    return 0;
}

void SDH0_IRQHandler(void)
{
    uint32_t volatile u32Isr;

    /* FMI data abort interrupt */
    if (SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    /* ----- SD interrupt status */
    u32Isr = SDH0->INTSTS;
    if (u32Isr & SDH_INTSTS_BLKDIF_Msk)
    {
        /* Block down */
        g_u8SDDataReadyFlag = TRUE;
        SDH0->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (u32Isr & SDH_INTSTS_CDIF_Msk)    // card detect
    {
        /* ----- SD interrupt status */
        /* it is work to delay 50 times for SD_CLK = 200KHz */
        {
            int volatile i;
            for (i = 0; i < 0x500; i++); /* delay to make sure got updated value from REG_SDISR. */
            u32Isr = SDH0->INTSTS;
        }

        if (u32Isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\n\r***** card remove !\n\r");
            SD0.IsCardInsert = FALSE;   /* SDISR_CD_Card = 1 means card remove for GPIO mode */
            memset(&SD0, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("***** card insert !\n\r");
            SDH_Open(SDH0, CardDetect_From_GPIO);
            //SDH_Open(SDH0, CardDetect_From_DAT3);
            SDH_Probe(SDH0);
        }

        SDH0->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    /* CRC error interrupt */
    if (u32Isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(u32Isr & SDH_INTSTS_CRC16_Msk))
        {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(u32Isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!g_u8R3Flag)
            {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        /* Clear interrupt flag */
        SDH0->INTSTS = SDH_INTSTS_CRCIF_Msk;
    }

    if (u32Isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n\r");
        SDH0->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    /* Response in timeout interrupt */
    if (u32Isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n\r");
        SDH0->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }
}


/*
 * ARM FLASH device structure
 */
struct arm_flash_dev_t
{
    const uint32_t memory_base;   /*!< FLASH memory base address */
    ARM_FLASH_INFO *data;         /*!< FLASH data */
};

/* Flash Status */
static ARM_FLASH_STATUS FlashStatus = {0, 0, 0};

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion =
{
    ARM_FLASH_API_VERSION,
    SD_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_FLASH_CAPABILITIES DriverCapabilities =
{
    0, /* event_ready */
    0, /* data_width = 0:8-bit, 1:16-bit, 2:32-bit */
    0  /* erase_chip */
};

static int32_t is_range_valid(struct arm_flash_dev_t* flash_dev,
    uint32_t offset)
{
    uint32_t flash_size = 0;
    int32_t rc = 0;

    flash_size = (flash_dev->data->sector_count * flash_dev->data->sector_size);

    if(offset > flash_size)
    {
        rc = -1;
    }
    return rc;
}

static ARM_FLASH_INFO NU_SD0_DEV_DATA =
{
    .sector_info  = NULL,                  /* Uniform sector layout */
    .sector_count = FMC_APROM_SIZE / FMC_FLASH_PAGE_SIZE,
    .sector_size  = FMC_FLASH_PAGE_SIZE,
    .page_size    = FMC_FLASH_PAGE_SIZE,
    .program_unit = 1,
    .erased_value = 0xFF
};

static struct arm_flash_dev_t NU_SD0_DEV =
{
    .memory_base = 0,
    .data        = &(NU_SD0_DEV_DATA)
};

struct arm_flash_dev_t *SD0_DEV = &NU_SD0_DEV;

/*
 * Functions
 */

static ARM_DRIVER_VERSION SD_GetVersion(void)
{
    return DriverVersion;
}

static ARM_FLASH_CAPABILITIES SD_GetCapabilities(void)
{
    return DriverCapabilities;
}

static int32_t SD_Initialize(ARM_Flash_SignalEvent_t cb_event)
{

    TCHAR       sd_path[] = { '0', ':', 0 };    /* SD drive started from 0 */
    FRESULT     res;
    TCHAR       *fname   = "fwimg.bin";
    TCHAR       *fnamens = "fwimg_ns.bin";

    ARG_UNUSED(cb_event);
    
    /* The file pointer is not NULL. The file should be opened */
    if(s_pfile)
        return ARM_DRIVER_OK;

    /* SDH0 initialze */
    NVIC_EnableIRQ(SDH0_IRQn);
    SDH_Open_Disk(SDH0, CardDetect_From_GPIO);
    f_chdrive(sd_path);          /* set default path */

    /* Open image file */
    if (SDH_CardDetection(SDH0))
    {
        res = f_stat(fname, &finfo);

        if(res == FR_OK)
        {
            printf("image name %s\n\r", finfo.fname);
            printf("file size = %lu\n\r", finfo.fsize);

            s_pfile = &fobj;
            res = f_open(s_pfile, fname, FA_READ);
            if(res != FR_OK)
            {
                printf("Cannot open image file %s\n\r", fname);
                s_pfile = NULL;
            }
        }
        else
        {
            /* Cannot find fwimage.bin, try to open fwimage_ns.bin here */
            res = f_stat(fnamens, &finfo);
            if(res == FR_OK)
            {
                printf("image name %s\n\r", finfo.fname);
                printf("file size = %lu\n\r", finfo.fsize);

                s_pfile = &fobj;
                res = f_open(s_pfile, fnamens, FA_READ);
                if(res != FR_OK)
                {
                    printf("Cannot open image file %s\n\r", fnamens);
                    s_pfile = NULL;
                }
                /* Set non-secure image flag */
                s_nsflag = 1;
            }
            else
            {
                /* Cannot open file. Just return driver error */
                s_pfile = NULL;
                printf("No image file found(%s or %s).\n\r", fname, fnamens);
            }
        }
    }
    return ARM_DRIVER_OK;
}

static int32_t SD_Uninitialize(void)
{

    if(s_pfile)
    {
        f_close(s_pfile);
        s_pfile = NULL;
    }

    NVIC_DisableIRQ(SDH0_IRQn);

    return ARM_DRIVER_OK;
}

static int32_t SD_PowerControl(ARM_POWER_STATE state)
{
    switch(state)
    {
        case ARM_POWER_FULL:
            /* Nothing to be done */
            return ARM_DRIVER_OK;
            break;

        case ARM_POWER_OFF:
        case ARM_POWER_LOW:
        default:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
}

static int32_t SD_ReadData(uint32_t addr, void *data, uint32_t cnt)
{
    uint32_t start_addr = SD0_DEV->memory_base + addr;
    int32_t rc = 0;
    uint8_t* pu8;
    uint32_t len;
    FRESULT res;

    printf("read file 0x%08x %d\n\r", start_addr, cnt);

    /* Check flash memory boundaries */
    rc = is_range_valid(SD0_DEV, addr + cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    pu8 = (uint8_t*)data;

    if(s_pfile == NULL)
        return ARM_DRIVER_ERROR;
        
#if (MCUBOOT_IMAGE_NUMBER == 2)
    if(s_nsflag)
    {
        /* This is non-secure only image. Just return 0xff if the address within secure image */
        if(start_addr < FLASH_AREA_3_OFFSET)
        {
            /* Should not cross boundary */
            if(start_addr + cnt > FLASH_AREA_3_OFFSET)
                return ARM_DRIVER_ERROR_PARAMETER;

            memset(pu8, 0xff, cnt);
            len = cnt;
            return ARM_DRIVER_OK;
        }

        start_addr -= FLASH_AREA_3_OFFSET;
        printf("Nonsecure image only. change offset to 0x%08x\n\r", start_addr);
    }
#endif

    /* seek and read */
    res = f_lseek(s_pfile, start_addr);
    if(res != FR_OK)
    {
        return ARM_DRIVER_ERROR;
    }

    res = f_read(s_pfile, pu8, cnt, &len);
    if(res != FR_OK)
    {
        return ARM_DRIVER_ERROR;
    }

    printf("read file 0x%08x %d ret %d\n\r", start_addr, cnt, len);

    /* padding 0xff if no enough data to read */
    memset(&pu8[len], 0xff, cnt - len);

    return ARM_DRIVER_OK;
}

static int32_t SD_ProgramData(uint32_t addr, const void *data, uint32_t cnt)
{
    volatile uint32_t mem_base = SD0_DEV->memory_base;
    uint32_t start_addr = mem_base + addr;
    uint32_t len;
    FRESULT res;
    int32_t rc = 0;

    printf("write addr 0x%08x size %d\n\r", start_addr, cnt);

    if(s_pfile == NULL)
        return ARM_DRIVER_ERROR;

    rc = is_range_valid(SD0_DEV, start_addr + cnt);
    if(rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }


    res = f_lseek(s_pfile, start_addr);
    if(res != FR_OK)
    {
        return ARM_DRIVER_ERROR;
    }

    res = f_write(s_pfile, data, cnt, &len);
    if(res != FR_OK)
    {
        return ARM_DRIVER_ERROR;
    }

    return ARM_DRIVER_OK;
}

static int32_t SD_EraseSector(uint32_t addr)
{
    return ARM_DRIVER_OK;
}

static int32_t SD_EraseChip(void)
{
    return ARM_DRIVER_OK;
}

static ARM_FLASH_STATUS SD_GetStatus(void)
{
    return FlashStatus;
}

static ARM_FLASH_INFO * SD_GetInfo(void)
{
    return SD0_DEV->data;
}

ARM_DRIVER_FLASH Driver_SD =
{
    SD_GetVersion,
    SD_GetCapabilities,
    SD_Initialize,
    SD_Uninitialize,
    SD_PowerControl,
    SD_ReadData,
    SD_ProgramData,
    SD_EraseSector,
    SD_EraseChip,
    SD_GetStatus,
    SD_GetInfo
};

