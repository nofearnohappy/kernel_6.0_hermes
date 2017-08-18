/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Ana.c
 *
 * Project:
 * --------
 *   MT6583  Audio Driver ana Register setting
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "AudDrv_Common.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"

// define this to use wrapper to control
#define AUDIO_USING_WRAP_DRIVER
#ifdef AUDIO_USING_WRAP_DRIVER
#include <mach/mt_pmic_wrap.h>
#endif

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

void Ana_Set_Reg(uint32 offset, uint32 value, uint32 mask)
{
    // set pmic register or analog CONTROL_IFACE_PATH
    int ret = 0;
    uint32 Reg_Value;

    PRINTK_ANA_REG("Ana_Set_Reg offset= 0x%x , value = 0x%x mask = 0x%x\n", offset, value, mask);
#ifdef AUDIO_USING_WRAP_DRIVER
    Reg_Value = Ana_Get_Reg(offset);
    Reg_Value &= (~mask);
    Reg_Value |= (value & mask);
    ret = pwrap_write(offset, Reg_Value);
    Reg_Value = Ana_Get_Reg(offset);
    if ((Reg_Value & mask) != (value & mask))
    {
        //printk("Ana_Set_Reg offset= 0x%x , value = 0x%x mask = 0x%x ret = %d Reg_Value = 0x%x\n", offset, value, mask, ret, Reg_Value);
    }
#endif
}

uint32 Ana_Get_Reg(uint32 offset)
{
    // get pmic register
    int ret = 0;
    uint32 Rdata = 0;
#ifdef AUDIO_USING_WRAP_DRIVER
    ret = pwrap_read(offset, &Rdata);
#endif
    PRINTK_ANA_REG ("Ana_Get_Reg offset= 0x%x  Rdata = 0x%x ret = %d\n",offset,Rdata,ret);
    return Rdata;
}

void Ana_Log_Print(void)
{
    AudDrv_ANA_Clk_On();
    printk("AFE_UL_DL_CON0  = 0x%x\n", Ana_Get_Reg(AFE_UL_DL_CON0));
    printk("AFE_DL_SRC2_CON0_H  = 0x%x\n", Ana_Get_Reg(AFE_DL_SRC2_CON0_H));
    printk("AFE_DL_SRC2_CON0_L  = 0x%x\n", Ana_Get_Reg(AFE_DL_SRC2_CON0_L));
    printk("AFE_DL_SDM_CON0  = 0x%x\n", Ana_Get_Reg(AFE_DL_SDM_CON0));
    printk("AFE_DL_SDM_CON1  = 0x%x\n", Ana_Get_Reg(AFE_DL_SDM_CON1));
    printk("AFE_UL_SRC0_CON0_H  = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC0_CON0_H));
    printk("AFE_UL_SRC0_CON0_L  = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC0_CON0_L));
    printk("AFE_UL_SRC1_CON0_H  = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC1_CON0_H));
    printk("AFE_UL_SRC1_CON0_L  = 0x%x\n", Ana_Get_Reg(AFE_UL_SRC1_CON0_L));
    printk("PMIC_AFE_TOP_CON0  = 0x%x\n", Ana_Get_Reg(PMIC_AFE_TOP_CON0));
    printk("AFE_AUDIO_TOP_CON0  = 0x%x\n", Ana_Get_Reg(AFE_AUDIO_TOP_CON0));
    printk("PMIC_AFE_TOP_CON0  = 0x%x\n", Ana_Get_Reg(PMIC_AFE_TOP_CON0));
    printk("AFE_DL_SRC_MON0  = 0x%x\n", Ana_Get_Reg(AFE_DL_SRC_MON0));
    printk("AFE_DL_SDM_TEST0  = 0x%x\n", Ana_Get_Reg(AFE_DL_SDM_TEST0));
    printk("AFE_MON_DEBUG0  = 0x%x\n", Ana_Get_Reg(AFE_MON_DEBUG0));
    printk("AFUNC_AUD_CON0  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON0));
    printk("AFUNC_AUD_CON1  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON1));
    printk("AFUNC_AUD_CON2  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON2));
    printk("AFUNC_AUD_CON3  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON3));
    printk("AFUNC_AUD_CON4  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_CON4));
    printk("AFUNC_AUD_MON0  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_MON0));
    printk("AFUNC_AUD_MON1  = 0x%x\n", Ana_Get_Reg(AFUNC_AUD_MON1));
    printk("AUDRC_TUNE_MON0  = 0x%x\n", Ana_Get_Reg(AUDRC_TUNE_MON0));
    printk("AFE_UP8X_FIFO_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_UP8X_FIFO_CFG0));
    printk("AFE_UP8X_FIFO_LOG_MON0  = 0x%x\n", Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON0));
    printk("AFE_UP8X_FIFO_LOG_MON1  = 0x%x\n", Ana_Get_Reg(AFE_UP8X_FIFO_LOG_MON1));
    printk("AFE_DL_DC_COMP_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG0));
    printk("AFE_DL_DC_COMP_CFG1  = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG1));
    printk("AFE_DL_DC_COMP_CFG2  = 0x%x\n", Ana_Get_Reg(AFE_DL_DC_COMP_CFG2));
    printk("AFE_PMIC_NEWIF_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG0));
    printk("AFE_PMIC_NEWIF_CFG1  = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG1));
    printk("AFE_PMIC_NEWIF_CFG2  = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG2));
    printk("AFE_PMIC_NEWIF_CFG3  = 0x%x\n", Ana_Get_Reg(AFE_PMIC_NEWIF_CFG3));
    printk("AFE_SGEN_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_SGEN_CFG0));
    printk("AFE_SGEN_CFG1  = 0x%x\n", Ana_Get_Reg(AFE_SGEN_CFG1));
    printk("AFE_VOW_TOP  = 0x%x\n", Ana_Get_Reg(AFE_VOW_TOP));
    printk("AFE_VOW_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG0));
    printk("AFE_VOW_CFG1  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG1));
    printk("AFE_VOW_CFG2  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG2));
    printk("AFE_VOW_CFG3  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG3));
    printk("AFE_VOW_CFG4  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG4));
    printk("AFE_VOW_CFG5  = 0x%x\n", Ana_Get_Reg(AFE_VOW_CFG5));
    printk("AFE_VOW_MON0  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON0));
    printk("AFE_VOW_MON1  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON1));
    printk("AFE_VOW_MON2  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON2));
    printk("AFE_VOW_MON3  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON3));
    printk("AFE_VOW_MON4  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON4));
    printk("AFE_VOW_MON5  = 0x%x\n", Ana_Get_Reg(AFE_VOW_MON5));

    printk("AFE_DCCLK_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_DCCLK_CFG0));
    printk("AFE_DCCLK_CFG1  = 0x%x\n", Ana_Get_Reg(AFE_DCCLK_CFG1));

    printk("TOP_CON  = 0x%x\n", Ana_Get_Reg(TOP_CON));
    printk("TOP_STATUS  = 0x%x\n", Ana_Get_Reg(TOP_STATUS));
    printk("TOP_CKPDN_CON0  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON0));
    printk("TOP_CKPDN_CON1  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON1));
    printk("TOP_CKPDN_CON2  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON2));
    printk("TOP_CKPDN_CON3  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON3));
    printk("TOP_CKSEL_CON0  = 0x%x\n", Ana_Get_Reg(TOP_CKSEL_CON0));
    printk("TOP_CKSEL_CON1  = 0x%x\n", Ana_Get_Reg(TOP_CKSEL_CON1));
    printk("TOP_CKSEL_CON2  = 0x%x\n", Ana_Get_Reg(TOP_CKSEL_CON2));
    printk("TOP_CKDIVSEL_CON  = 0x%x\n", Ana_Get_Reg(TOP_CKDIVSEL_CON));
    printk("TOP_CKHWEN_CON  = 0x%x\n", Ana_Get_Reg(TOP_CKHWEN_CON));
    printk("TOP_CKTST_CON0  = 0x%x\n", Ana_Get_Reg(TOP_CKTST_CON0));
    printk("TOP_CKTST_CON1  = 0x%x\n", Ana_Get_Reg(TOP_CKTST_CON1));
    printk("TOP_CKTST_CON2  = 0x%x\n", Ana_Get_Reg(TOP_CKTST_CON2));
    printk("TOP_CLKSQ  = 0x%x\n", Ana_Get_Reg(TOP_CLKSQ));
    printk("TOP_RST_CON0  = 0x%x\n", Ana_Get_Reg(TOP_RST_CON0));
    printk("ZCD_CON0  = 0x%x\n", Ana_Get_Reg(ZCD_CON0));
    printk("ZCD_CON1  = 0x%x\n", Ana_Get_Reg(ZCD_CON1));
    printk("ZCD_CON2  = 0x%x\n", Ana_Get_Reg(ZCD_CON2));
    printk("ZCD_CON3  = 0x%x\n", Ana_Get_Reg(ZCD_CON3));
    printk("ZCD_CON4  = 0x%x\n", Ana_Get_Reg(ZCD_CON4));
    printk("ZCD_CON5  = 0x%x\n", Ana_Get_Reg(ZCD_CON5));
    printk("LDO_CON1  = 0x%x\n", Ana_Get_Reg(LDO_CON1));
    printk("LDO_CON2  = 0x%x\n", Ana_Get_Reg(LDO_CON2));

    printk("LDO_VCON1  = 0x%x\n", Ana_Get_Reg(LDO_VCON1));
    printk("SPK_CON0  = 0x%x\n", Ana_Get_Reg(SPK_CON0));
    printk("SPK_CON1  = 0x%x\n", Ana_Get_Reg(SPK_CON1));
    printk("SPK_CON2  = 0x%x\n", Ana_Get_Reg(SPK_CON2));
    printk("SPK_CON3  = 0x%x\n", Ana_Get_Reg(SPK_CON3));
    printk("SPK_CON4  = 0x%x\n", Ana_Get_Reg(SPK_CON4));
    printk("SPK_CON5  = 0x%x\n", Ana_Get_Reg(SPK_CON5));
    printk("SPK_CON6  = 0x%x\n", Ana_Get_Reg(SPK_CON6));
    printk("SPK_CON7  = 0x%x\n", Ana_Get_Reg(SPK_CON7));
    printk("SPK_CON8  = 0x%x\n", Ana_Get_Reg(SPK_CON8));
    printk("SPK_CON9  = 0x%x\n", Ana_Get_Reg(SPK_CON9));
    printk("SPK_CON10  = 0x%x\n", Ana_Get_Reg(SPK_CON10));
    printk("SPK_CON11  = 0x%x\n", Ana_Get_Reg(SPK_CON11));
    printk("SPK_CON12  = 0x%x\n", Ana_Get_Reg(SPK_CON12));
    printk("SPK_CON13  = 0x%x\n", Ana_Get_Reg(SPK_CON13));
    printk("SPK_CON14  = 0x%x\n", Ana_Get_Reg(SPK_CON14));
    printk("SPK_CON15  = 0x%x\n", Ana_Get_Reg(SPK_CON15));
    printk("SPK_CON16  = 0x%x\n", Ana_Get_Reg(SPK_CON16));
    printk("SPK_ANA_CON0  = 0x%x\n", Ana_Get_Reg(SPK_ANA_CON0));
    printk("SPK_ANA_CON1  = 0x%x\n", Ana_Get_Reg(SPK_ANA_CON1));
    printk("SPK_ANA_CON3  = 0x%x\n", Ana_Get_Reg(SPK_ANA_CON3));
    printk("AUDDEC_ANA_CON0  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON0));
    printk("AUDDEC_ANA_CON1  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON1));
    printk("AUDDEC_ANA_CON2  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON2));
    printk("AUDDEC_ANA_CON3  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON3));
    printk("AUDDEC_ANA_CON4  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON4));
    printk("AUDDEC_ANA_CON5  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON5));
    printk("AUDDEC_ANA_CON6  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON6));
    printk("AUDDEC_ANA_CON7  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON7));
    printk("AUDDEC_ANA_CON8  = 0x%x\n", Ana_Get_Reg(AUDDEC_ANA_CON8));

    printk("AUDENC_ANA_CON0  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON0));
    printk("AUDENC_ANA_CON1  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON1));
    printk("AUDENC_ANA_CON2  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON2));
    printk("AUDENC_ANA_CON3  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON3));
    printk("AUDENC_ANA_CON4  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON4));
    printk("AUDENC_ANA_CON5  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON5));
    printk("AUDENC_ANA_CON6  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON6));
    printk("AUDENC_ANA_CON7  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON7));
    printk("AUDENC_ANA_CON8  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON8));
    printk("AUDENC_ANA_CON9  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON9));
    printk("AUDENC_ANA_CON11  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON11));
    printk("AUDENC_ANA_CON12  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON12));
    printk("AUDENC_ANA_CON13  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON13));
    printk("AUDENC_ANA_CON14  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON14));
    printk("AUDENC_ANA_CON15  = 0x%x\n", Ana_Get_Reg(AUDENC_ANA_CON15));

    printk("AUDNCP_CLKDIV_CON0  = 0x%x\n", Ana_Get_Reg(AUDNCP_CLKDIV_CON0));
    printk("AUDNCP_CLKDIV_CON1  = 0x%x\n", Ana_Get_Reg(AUDNCP_CLKDIV_CON1));
    printk("AUDNCP_CLKDIV_CON2  = 0x%x\n", Ana_Get_Reg(AUDNCP_CLKDIV_CON2));
    printk("AUDNCP_CLKDIV_CON3  = 0x%x\n", Ana_Get_Reg(AUDNCP_CLKDIV_CON3));
    printk("AUDNCP_CLKDIV_CON4  = 0x%x\n", Ana_Get_Reg(AUDNCP_CLKDIV_CON4));

    printk("TOP_CKPDN_CON0  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN_CON0));
    printk("GPIO_MODE3  = 0x%x\n", Ana_Get_Reg(GPIO_MODE3));
    printk("AFE_VOW_POSDIV_CFG0  = 0x%x\n", Ana_Get_Reg(AFE_VOW_POSDIV_CFG0));
    AudDrv_ANA_Clk_Off();
    printk("-Ana_Log_Print \n");
}


// export symbols for other module using
EXPORT_SYMBOL(Ana_Log_Print);
EXPORT_SYMBOL(Ana_Set_Reg);
EXPORT_SYMBOL(Ana_Get_Reg);


