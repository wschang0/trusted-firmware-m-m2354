/**************************************************************************//**
 * @file     timer.c
 * @version  V3.00
 * @brief    Timer PWM Controller(Timer PWM) driver source file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "NuMicro.h"


/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup TIMER_PWM_Driver TIMER PWM Driver
  @{
*/

/** @addtogroup TIMER_PWM_EXPORTED_FUNCTIONS TIMER PWM Exported Functions
  @{
*/

/**
  * @brief      Set PWM Counter Clock Source
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32CntClkSrc    PWM counter clock source, could be one of following source
  *                                 - \ref TPWM_CNTR_CLKSRC_TMR_CLK
  *                                 - \ref TPWM_CNTR_CLKSRC_TIMER0_INT
  *                                 - \ref TPWM_CNTR_CLKSRC_TIMER1_INT
  *                                 - \ref TPWM_CNTR_CLKSRC_TIMER2_INT
  *                                 - \ref TPWM_CNTR_CLKSRC_TIMER3_INT
  *
  *
  *
  * @details    This function is used to set PWM counter clock source.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_SetCounterClockSource(TIMER_T *timer, uint32_t u32CntClkSrc)
{
    (timer)->PWMCLKSRC = ((timer)->PWMCLKSRC & ~TIMER_PWMCLKSRC_CLKSRC_Msk) | u32CntClkSrc;
}

/** @cond HIDDEN_SYMBOLS */
/**
  * @brief      Get Timer PWM Clock Frequency
  *
  * @param[in]  timer   The pointer of the specified Timer module. It could be TIMER0 ~ TIMER5.
  *
  * @return     Timer clock frequency
  *
  * @details    This API is used to get the timer pwm clock frequency.
  * @note       This API cannot return correct clock rate if timer source is from external clock input.
  */
static uint32_t TPWM_GetModuleClockFreq(TIMER_T *timer)
{
    uint32_t u32Src, u32ClkFreq = __HIRC;
    const uint32_t au32Clk[] = {__HXT, __LXT, 0UL, 0UL, __MIRC, __LIRC, 0UL, __HIRC};

    if(timer == TIMER0)
    {
        u32Src = CLK_GetModuleClockSource(TMR0_MODULE);
    }
    else if(timer == TIMER1)
    {
        u32Src = CLK_GetModuleClockSource(TMR1_MODULE);
    }
    else if((timer == TIMER2) || (timer == TIMER2_NS))
    {
        u32Src = CLK_GetModuleClockSource(TMR2_MODULE);
    }
    else if((timer == TIMER3) || (timer == TIMER3_NS))
    {
        u32Src = CLK_GetModuleClockSource(TMR3_MODULE);
    }
    else if((timer == TIMER4) || (timer == TIMER4_NS))
    {
        u32Src = CLK_GetModuleClockSource(TMR4_MODULE);
    }
    else if((timer == TIMER5) || (timer == TIMER5_NS))
    {
        u32Src = CLK_GetModuleClockSource(TMR5_MODULE);
    }
    else
    {
        return 0UL;
    }

    if(u32Src == 2UL)
    {
        if((timer == TIMER0) || (timer == TIMER1) ||
                (timer == TIMER4) || (timer == TIMER4_NS) || (timer == TIMER5) || (timer == TIMER5_NS))
        {
            u32ClkFreq = CLK_GetPCLK0Freq();
        }
        else
        {
            u32ClkFreq = CLK_GetPCLK1Freq();
        }
    }
    else
    {
        u32ClkFreq = au32Clk[u32Src];
    }

    return u32ClkFreq;
}
/** @endcond HIDDEN_SYMBOLS */

/**
  * @brief      Configure PWM Output Frequency and Duty Cycle
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0 ~ TIMER5.
  * @param[in]  u32Frequency    Target generator frequency.
  * @param[in]  u32DutyCycle    Target generator duty cycle percentage. Valid range are between 0~100. 10 means 10%, 20 means 20%...
  *
  * @return     Nearest frequency clock in nano second
  *
  * @details    This API is used to configure PWM output frequency and duty cycle in up count type and auto-reload operation mode.
  * @note       This API is only available if Timer PWM counter clock source is from TMRx_CLK.
  */
uint32_t TPWM_ConfigOutputFreqAndDuty(TIMER_T *timer, uint32_t u32Frequency, uint32_t u32DutyCycle)
{
    uint32_t u32PWMClockFreq, u32TargetFreq;
    uint32_t u32Prescaler = 0x1000UL, u32Period = 1UL, u32CMP, u32MaxCLKPSC = 0x1000UL;

    if((timer == TIMER0) || (timer == TIMER1))
    {
        u32PWMClockFreq = CLK_GetPCLK0Freq();
    }
    else if((timer == TIMER4) || (timer == TIMER4_NS) || (timer == TIMER5) || (timer == TIMER5_NS))
    {
        u32PWMClockFreq = TPWM_GetModuleClockFreq(timer);
    }
    else
    {
        u32PWMClockFreq = CLK_GetPCLK1Freq();
    }

    if((timer == TIMER4) || (timer == TIMER4_NS) || (timer == TIMER5) || (timer == TIMER5_NS))
        u32MaxCLKPSC = 0x100UL; // 8-bit clock prescale on TIMER4 and TIMER5 PWM
    else
        u32MaxCLKPSC = 0x1000UL; // 12-bit clock prescale on TIMER0 ~ TIMER3 PWM

    /* Calculate u16PERIOD and u16PSC */
    for(u32Prescaler = 1UL; u32Prescaler <= u32MaxCLKPSC; u32Prescaler++)
    {
        u32Period = (u32PWMClockFreq / u32Prescaler) / u32Frequency;

        /* If target u32Period is larger than 0x10000, need to use a larger prescaler */
        if(u32Period <= 0x10000UL)
        {
            break;
        }
    }
    /* Store return value here 'cos we're gonna change u32Prescaler & u32Period to the real value to fill into register */
    u32TargetFreq = (u32PWMClockFreq / u32Prescaler) / u32Period;

    /* Set PWM to up count type */
    timer->PWMCTL = (timer->PWMCTL & ~TIMER_PWMCTL_CNTTYPE_Msk) | (TPWM_UP_COUNT << TIMER_PWMCTL_CNTTYPE_Pos);

    /* Set PWM to auto-reload mode */
    timer->PWMCTL = (timer->PWMCTL & ~TIMER_PWMCTL_CNTMODE_Msk) | (TPWM_AUTO_RELOAD_MODE << TIMER_PWMCTL_CNTMODE_Pos);

    /* Convert to real register value */
    TPWM_SET_PERIOD(timer, (u32Period - 1UL));
    if(u32DutyCycle)
    {
        u32CMP = (u32DutyCycle * u32Period) / 100UL;
    }
    else
    {
        u32CMP = 0UL;
    }

    TPWM_SET_CMPDAT(timer, u32CMP);

    TPWM_SET_PRESCALER(timer, (u32Prescaler - 1UL));

    return (u32TargetFreq);
}

/**
  * @brief      Enable Dead-Time Function
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32DTCount  Dead-Time duration in PWM clock count, valid values are between 0x0~0xFFF, but 0x0 means there is no Dead-Time insertion.
  *
  *
  *
  * @details    This function is used to enable Dead-Time function and counter source is the same as Timer PWM clock source.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_EnableDeadTime(TIMER_T *timer, uint32_t u32DTCount)
{
    timer->PWMDTCTL = TIMER_PWMDTCTL_DTEN_Msk | u32DTCount;
}

/**
  * @brief      Enable Dead-Time Function
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32DTCount  Dead-Time duration in PWM clock count, valid values are between 0x0~0xFFF, but 0x0 means there is no Dead-Time insertion.
  *
  *
  *
  * @details    This function is used to enable Dead-Time function and counter source is the Timer PWM clock source with prescale.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_EnableDeadTimeWithPrescale(TIMER_T *timer, uint32_t u32DTCount)
{
    timer->PWMDTCTL = TIMER_PWMDTCTL_DTCKSEL_Msk | TIMER_PWMDTCTL_DTEN_Msk | u32DTCount;
}

/**
  * @brief      Disable Dead-Time Function
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  *
  *
  *
  * @details    This function is used to disable Dead-time of selected channel.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_DisableDeadTime(TIMER_T *timer)
{
    timer->PWMDTCTL = 0x0UL;
}

/**
  * @brief      Enable PWM Counter
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0 ~ TIMER5.
  *
  *
  *
  * @details    This function is used to enable PWM generator and start counter counting.
  */
void TPWM_EnableCounter(TIMER_T *timer)
{
    timer->PWMCTL |= TIMER_PWMCTL_CNTEN_Msk;
}

/**
  * @brief      Disable PWM Generator
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0 ~ TIMER5.
  *
  *
  *
  * @details This function is used to disable PWM counter immediately by clear CNTEN (TIMERx_PWMCTL[0]) bit.
  */
void TPWM_DisableCounter(TIMER_T *timer)
{
    timer->PWMCTL &= ~TIMER_PWMCTL_CNTEN_Msk;
}

/**
  * @brief      Enable Trigger ADC
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0 ~ TIMER5.
  * @param[in]  u32Condition    The condition to trigger ADC. It could be one of following conditions:
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_ZERO_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_PERIOD_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_ZERO_OR_PERIOD_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_COMPARE_UP_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_COMPARE_DOWN_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_PERIOD_OR_COMPARE_UP_POINT
  *
  *
  *
  * @details    This function is used to enable specified counter compare event to trigger ADC.
  * @note       TIMER4 and TIMER5 only supports \ref TPWM_TRIGGER_EVENT_AT_PERIOD_POINT, \ref TPWM_TRIGGER_EVENT_AT_COMPARE_UP_POINT,
  *             and \ref TPWM_TRIGGER_EVENT_AT_PERIOD_OR_COMPARE_UP_POINT.
  */
void TPWM_EnableTriggerADC(TIMER_T *timer, uint32_t u32Condition)
{
    timer->PWMTRGCTL = ((timer->PWMTRGCTL & ~TIMER_PWMTRGCTL_TRGSEL_Msk) | (u32Condition)) | TIMER_PWMTRGCTL_TRGEADC_Msk;
}

/**
  * @brief      Disable Trigger ADC
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0 ~ TIMER5.
  *
  *
  *
  * @details    This function is used to disable counter compare event to trigger ADC.
  */
void TPWM_DisableTriggerADC(TIMER_T *timer)
{
    timer->PWMTRGCTL &= ~TIMER_PWMTRGCTL_TRGEADC_Msk;
}

/**
  * @brief      Enable Trigger PDMA
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER4 or TIMER5.
  * @param[in]  u32Condition    The condition to trigger PDMA. It could be one of following conditions:
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_PERIOD_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_COMPARE_UP_POINT
  *                                 - \ref TPWM_TRIGGER_EVENT_AT_PERIOD_OR_COMPARE_UP_POINT
  *
  *
  *
  * @details    This function is used to enable specified counter compare event to trigger PDMA.
  * @note       Only available on TIMER4 and TIMER5.
  */
void TPWM_EnableTriggerPDMA(TIMER_T *timer, uint32_t u32Condition)
{
    timer->PWMTRGCTL = ((timer->PWMTRGCTL & ~TIMER_PWMTRGCTL_TRGSEL_Msk) | (u32Condition)) | TIMER_PWMTRGCTL_TRGPDMA_Msk;
}

/**
  * @brief      Disable Trigger PDMA
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER4 or TIMER5.
  *
  *
  *
  * @details    This function is used to disable counter compare event to trigger ADC.
  * @note       Only available on TIMER4 and TIMER5.
  */
void TPWM_DisableTriggerPDMA(TIMER_T *timer)
{
    timer->PWMTRGCTL &= ~TIMER_PWMTRGCTL_TRGPDMA_Msk;
}

/**
  * @brief      Enable Fault Brake Function
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32CH0Level     PWMx_CH0 output level while fault brake event occurs. Valid value is one of following setting
  *                                 - \ref TPWM_OUTPUT_TOGGLE
  *                                 - \ref TPWM_OUTPUT_NOTHING
  *                                 - \ref TPWM_OUTPUT_LOW
  *                                 - \ref TPWM_OUTPUT_HIGH
  * @param[in]  u32CH1Level     PWMx_CH1 output level while fault brake event occurs. Valid value is one of following setting
  *                                 - \ref TPWM_OUTPUT_TOGGLE
  *                                 - \ref TPWM_OUTPUT_NOTHING
  *                                 - \ref TPWM_OUTPUT_LOW
  *                                 - \ref TPWM_OUTPUT_HIGH
  * @param[in]  u32BrakeSource  Fault brake source, combination of following source
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_ACMP0
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_ACMP1
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_BKPIN
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_SYS_CSS
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_SYS_BOD
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_SYS_COR
  *                                 - \ref TPWM_BRAKE_SOURCE_EDGE_SYS_RAM
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_ACMP0
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_ACMP1
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_BKPIN
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_SYS_CSS
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_SYS_BOD
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_SYS_COR
  *                                 - \ref TPWM_BRAKE_SOURCE_LEVEL_SYS_RAM
  *
  *
  *
  * @details    This function is used to enable fault brake function.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_EnableFaultBrake(TIMER_T *timer, uint32_t u32CH0Level, uint32_t u32CH1Level, uint32_t u32BrakeSource)
{
    timer->PWMFAILBRK |= ((u32BrakeSource >> 16) & 0xFUL);
    timer->PWMBRKCTL = (timer->PWMBRKCTL & ~(TIMER_PWMBRKCTL_BRKAEVEN_Msk | TIMER_PWMBRKCTL_BRKAODD_Msk)) |
                       (u32BrakeSource & 0xFFFFUL) | (u32CH0Level << TIMER_PWMBRKCTL_BRKAEVEN_Pos) | (u32CH1Level << TIMER_PWMBRKCTL_BRKAODD_Pos);
}

/**
  * @brief      Enable Fault Brake Interrupt
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32IntSource    Interrupt source, could be one of following source
  *                                 - \ref TPWM_BRAKE_EDGE
  *                                 - \ref TPWM_BRAKE_LEVEL
  *
  *
  *
  * @details    This function is used to enable fault brake interrupt.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_EnableFaultBrakeInt(TIMER_T *timer, uint32_t u32IntSource)
{
    timer->PWMINTEN1 |= u32IntSource;
}

/**
  * @brief      Disable Fault Brake Interrupt
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32IntSource    Interrupt source, could be one of following source
  *                                 - \ref TPWM_BRAKE_EDGE
  *                                 - \ref TPWM_BRAKE_LEVEL
  *
  *
  *
  * @details    This function is used to disable fault brake interrupt.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_DisableFaultBrakeInt(TIMER_T *timer, uint32_t u32IntSource)
{
    timer->PWMINTEN1 &= ~u32IntSource;
}

/**
  * @brief      Indicate Fault Brake Interrupt Flag
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32IntSource    Interrupt source, could be one of following source
  *                                 - \ref TPWM_BRAKE_EDGE
  *                                 - \ref TPWM_BRAKE_LEVEL
  *
  * @return     Fault brake interrupt flag of specified source
  * @retval     0       Fault brake interrupt did not occurred
  * @retval     1       Fault brake interrupt occurred
  *
  * @details    This function is used to indicate fault brake interrupt flag occurred or not of selected source.
  * @note       NOT available on TIMER4 and TIMER5.
  */
uint32_t TPWM_GetFaultBrakeIntFlag(TIMER_T *timer, uint32_t u32IntSource)
{
    return ((timer->PWMINTSTS1 & (0x3UL << u32IntSource)) ? 1UL : 0UL);
}

/**
  * @brief      Clear Fault Brake Interrupt Flags
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32IntSource    Interrupt source, could be one of following source
  *                                 - \ref TPWM_BRAKE_EDGE
  *                                 - \ref TPWM_BRAKE_LEVEL
  *
  *
  *
  * @details    This function is used to clear fault brake interrupt flags of selected source.
  * @note       The register write-protection function should be disabled before using this function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_ClearFaultBrakeIntFlag(TIMER_T *timer, uint32_t u32IntSource)
{
    timer->PWMINTSTS1 = (0x3UL << u32IntSource);
}

/**
  * @brief      Enable Load Mode
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32LoadMode  Timer PWM counter loading mode, could be one of following mode
  *                             - \ref TPWM_LOAD_MODE_PERIOD
  *                             - \ref TPWM_LOAD_MODE_IMMEDIATE
  *                             - \ref TPWM_LOAD_MODE_CENTER
  *
  *
  *
  * @details    This function is used to enable load mode of selected channel.
  * @note       The default loading mode is period loading mode.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_SetLoadMode(TIMER_T *timer, uint32_t u32LoadMode)
{
    timer->PWMCTL = (timer->PWMCTL & ~(TIMER_PWMCTL_IMMLDEN_Msk | TIMER_PWMCTL_CTRLD_Msk)) | u32LoadMode;
}

/**
  * @brief      Enable Brake Pin Noise Filter Function
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32BrakePinSrc  The external brake pin source, could be one of following source
  *                                 - \ref TPWM_TM_BRAKE0
  *                                 - \ref TPWM_TM_BRAKE1
  *                                 - \ref TPWM_TM_BRAKE2
  *                                 - \ref TPWM_TM_BRAKE3
  * @param[in]  u32DebounceCnt  This value controls the real debounce sample time.
  *                             The target debounce sample time is (debounce sample clock period) * (u32DebounceCnt).
  * @param[in]  u32ClkSrcSel    Brake pin detector debounce clock source, could be one of following source
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_1
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_2
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_4
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_8
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_16
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_32
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_64
  *                                 - \ref TPWM_BKP_DBCLK_PCLK_DIV_128
  *
  *
  *
  * @details    This function is used to enable external brake pin detector noise filter function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_EnableBrakePinDebounce(TIMER_T *timer, uint32_t u32BrakePinSrc, uint32_t u32DebounceCnt, uint32_t u32ClkSrcSel)
{
    timer->PWMBNF = (timer->PWMBNF & ~(TIMER_PWMBNF_BKPINSRC_Msk | TIMER_PWMBNF_BRKFCNT_Msk | TIMER_PWMBNF_BRKNFSEL_Msk)) |
                    (u32BrakePinSrc << TIMER_PWMBNF_BKPINSRC_Pos) |
                    (u32DebounceCnt << TIMER_PWMBNF_BRKFCNT_Pos) |
                    (u32ClkSrcSel << TIMER_PWMBNF_BRKNFSEL_Pos) | TIMER_PWMBNF_BRKNFEN_Msk;
}

/**
  * @brief      Disable Brake Pin Noise Filter Function
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  *
  *
  *
  * @details    This function is used to disable external brake pin detector noise filter function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_DisableBrakePinDebounce(TIMER_T *timer)
{
    timer->PWMBNF &= ~TIMER_PWMBNF_BRKNFEN_Msk;
}

/**
  * @brief      Enable Brake Pin Inverse Function
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  *
  *
  *
  * @details    This function is used to enable PWM brake pin inverse function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_EnableBrakePinInverse(TIMER_T *timer)
{
    timer->PWMBNF |= TIMER_PWMBNF_BRKPINV_Msk;
}

/**
  * @brief      Disable Brake Pin Inverse Function
  *
  * @param[in]  timer       The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  *
  *
  *
  * @details    This function is used to disable PWM brake pin inverse function.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_DisableBrakePinInverse(TIMER_T *timer)
{
    timer->PWMBNF &= ~TIMER_PWMBNF_BRKPINV_Msk;
}

/**
  * @brief      Set Brake Pin Source
  *
  * @param[in]  timer           The pointer of the specified Timer module. It could be TIMER0, TIMER1, TIMER2, TIMER3.
  * @param[in]  u32BrakePinNum  Brake pin selection. One of the following:
  *                                 - \ref TPWM_TM_BRAKE0
  *                                 - \ref TPWM_TM_BRAKE1
  *                                 - \ref TPWM_TM_BRAKE2
  *                                 - \ref TPWM_TM_BRAKE3
  *
  *
  *
  * @details    This function is used to set PWM brake pin source.
  * @note       NOT available on TIMER4 and TIMER5.
  */
void TPWM_SetBrakePinSource(TIMER_T *timer, uint32_t u32BrakePinNum)
{
    timer->PWMBNF = (((timer)->PWMBNF & ~TIMER_PWMBNF_BKPINSRC_Msk) | (u32BrakePinNum << TIMER_PWMBNF_BKPINSRC_Pos));
}


/**@}*/ /* end of group TIMER_PWM_EXPORTED_FUNCTIONS */

/**@}*/ /* end of group TIMER_PWM_Driver */

/**@}*/ /* end of group Standard_Driver */
