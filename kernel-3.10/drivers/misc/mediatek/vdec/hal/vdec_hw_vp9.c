/********************************************************************************************
 *     LEGAL DISCLAIMER
 *
 *     (Header of MediaTek Software/Firmware Release or Documentation)
 *
 *     BY OPENING OR USING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *     THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE") RECEIVED
 *     FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON AN "AS-IS" BASIS
 *     ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED,
 *     INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
 *     A PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY
 *     WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 *     INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK
 *     ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *     NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION
 *     OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *     BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH
 *     RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION,
 *     TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE
 *     FEES OR SERVICE CHARGE PAID BY BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *     THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE WITH THE LAWS
 *     OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF LAWS PRINCIPLES.
 ************************************************************************************************/

//#include <mach/mt_typedefs.h>
#include "../include/vdec_info_vp9.h"
#include "vdec_hw_common.h"
#include "vdec_hw_vp9.h"
//#include "../include/vdec_info_vp9.h"

/* RISC Pattern Common Part Settings...*/

#if 0
#define MC_REG_OFFSET0    0x2000
#define MC_REG_OFFSET1    0x2F000
#define HEVC_MV_REG_OFFSET0       0x4000
#define HEVC_PP_REG_OFFSET0       0x5000
#define VLD_REG_OFFSET0   0x1000
#define VLD_REG_OFFSET1   0x2E000
#define VLD_TOP_REG_OFFSET0   (VLD_REG_OFFSET0 + 0x800)
#define VLD_TOP_REG_OFFSET1   (VLD_REG_OFFSET1 + 0x800)
#define HEVC_MISC_REG_OFFSET0    0x0
#else
#define MC_REG_OFFSET0    0x1000
#define MC_REG_OFFSET1    0x2F000
#define HEVC_MV_REG_OFFSET0       0x3000
#define HEVC_PP_REG_OFFSET0       0x4000
#define VLD_REG_OFFSET0   0x0000
#define VLD_REG_OFFSET1   0x2E000
#define VLD_TOP_REG_OFFSET0   (VLD_REG_OFFSET0 + 0x800)
#define VLD_TOP_REG_OFFSET1   (VLD_REG_OFFSET1 + 0x800)
#define HEVC_MISC_REG_OFFSET0    0x5000

#endif

#if 1
void vVP9RISCWrite_MC(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId )
{
    UINT32 VP9_MC_BASE = 0;

    if(u4CoreId == 0)
    {
        VP9_MC_BASE = MC_REG_OFFSET0;
    }
    else
    {
        VP9_MC_BASE = MC_REG_OFFSET1;
    }
    vWriteReg(VP9_MC_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( MC_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_MC(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_MC(UINT32 u4Addr, UINT32* pu4Value , UINT32 u4CoreId)
{
    UINT32 VP9_MC_BASE = 0;

    if(u4CoreId == 0)
    {
        VP9_MC_BASE = MC_REG_OFFSET0;
    }
    else
    {
        VP9_MC_BASE = MC_REG_OFFSET1;
    }

    (*pu4Value) = u4ReadReg(VP9_MC_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( MC_BASE, u4Addr*4  );
    SIM_PRINT("        RISCRead_MC(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCWrite_MV(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_MV_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_MV_BASE = HEVC_MV_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //MV_BASE = HEVC_MV_REG_OFFSET1;
    }
    else
    {
        //MV_BASE = LAE_MV_OFFSET0;
    }
    vWriteReg(VP9_MV_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( MV_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_MV(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_MV(UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId)
{
    UINT32 VP9_MV_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_MV_BASE = HEVC_MV_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //MV_BASE = HEVC_MV_REG_OFFSET1;
    }
    else
    {
        //MV_BASE = LAE_MV_OFFSET0;
    }

    (*pu4Value) = u4ReadReg(VP9_MV_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( MV_BASE, u4Addr*4  );
    SIM_PRINT("        RISCRead_MV( %u, %u); /* return 0x%08x */\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCWrite_PP(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_PP_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_PP_BASE = HEVC_PP_REG_OFFSET0;
    }
    else
    {
        //PP_BASE = HEVC_PP_REG_OFFSET1;
    }

    vWriteReg(VP9_PP_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( PP_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_PP(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_PP(UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId )
{
    UINT32 VP9_PP_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_PP_BASE = HEVC_PP_REG_OFFSET0;
    }
    else
    {
        //PP_BASE = HEVC_PP_REG_OFFSET1;
    }

    (*pu4Value) = u4ReadReg(VP9_PP_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( PP_BASE, u4Addr*4  );
    SIM_PRINT("        RISCRead_PP(%u, %u); // 0x%08x", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCWrite_VLD_TOP(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_VLD_TOP_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_VLD_TOP_BASE = VLD_TOP_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        VP9_VLD_TOP_BASE = VLD_TOP_REG_OFFSET1;
    }
    else
    {
        //VLD_TOP_BASE = LAE_VLDTOP_OFFSET0;
    }

    vWriteReg(VP9_VLD_TOP_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( VLD_TOP_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_VLD_TOP(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_VLD_TOP(UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId )
{
    UINT32 VP9_VLD_TOP_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_VLD_TOP_BASE = VLD_TOP_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        VP9_VLD_TOP_BASE = VLD_TOP_REG_OFFSET1;
    }
    else
    {
        //VLD_TOP_BASE = LAE_VLDTOP_OFFSET0;
    }

    (*pu4Value) = u4ReadReg(VP9_VLD_TOP_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( VLD_TOP_BASE, u4Addr*4  );

    SIM_PRINT("        RISCRead_VLD_TOP(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCWrite_VLD(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_VLD_BASE = VLD_REG_OFFSET0;

    if( u4CoreId == 0)
    {
        VP9_VLD_BASE = VLD_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        VP9_VLD_BASE = VLD_REG_OFFSET1;
    }
    else
    {
        //VLD_BASE = LAE_VLD_OFFSET0;
    }

    vWriteReg(VP9_VLD_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( VLD_BASE , u4Addr*4 , u4Value);

    SIM_PRINT("        RISCWrite_VLD(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_VLD(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId)
{
    UINT32 VP9_VLD_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_VLD_BASE = VLD_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        VP9_VLD_BASE = VLD_REG_OFFSET1;
    }
    else
    {
        //VLD_BASE = LAE_VLD_OFFSET0;
    }

    (*pu4Value) = u4ReadReg(VP9_VLD_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( VLD_BASE, u4Addr*4 );
    SIM_PRINT("        RISCRead_VLD(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCRead_MISC(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId)
{
    UINT32 VP9_MISC_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_MISC_BASE = HEVC_MISC_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //MISC_BASE = HEVC_MISC_REG_OFFSET1;
    }
    else
    {
        //MISC_BASE = LAE_MISC_OFFSET0;
    }

    (*pu4Value) = u4ReadReg(VP9_MISC_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( MISC_BASE, u4Addr*4 );
    SIM_PRINT("        RISCRead_MISC(%u, %u); // 0x%08x\n", u4Addr, u4CoreId,(*pu4Value));
}

void vVP9RISCWrite_MISC(UINT32 u4Addr, UINT32 u4Value , UINT32 u4CoreId)
{
    UINT32 VP9_MISC_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_MISC_BASE = HEVC_MISC_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //MISC_BASE = HEVC_MISC_REG_OFFSET1;
    }
    else
    {
        //MISC_BASE = LAE_MISC_OFFSET0;
    }

    vWriteReg(VP9_MISC_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( MISC_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_MISC(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_VDEC_TOP(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId)
{
    UINT32 VP9_MISC_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_MISC_BASE = 0xF6020000;
    }
    else // currently do not know core 1 settings
    {
        VP9_MISC_BASE = 0xF6020000;
    }

    (*pu4Value) = u4ReadReg(VP9_MISC_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( MISC_BASE, u4Addr*4 );
    SIM_PRINT("        RISCRead_VDEC_TOP(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCWrite_VDEC_TOP( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_MISC_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_MISC_BASE = 0xF6020000;
    }
    else // currently do not know core 1 settings
    {
        VP9_MISC_BASE = 0xF6020000;
    }

    vWriteReg(VP9_MISC_BASE + u4Addr*4, u4Value);
    //DRV_WriteReg( MISC_BASE , u4Addr*4 , u4Value);
    SIM_PRINT ("          RISCWrite_VDEC_TOP(%u , %-10u, %u); // 0x%08x\n",u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCWrite_VP9_VLD(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_VLD_BASE = 0;

    if( u4CoreId == CORE_0_ID)
    {
        VP9_VLD_BASE = VP9_VLD_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //VLD_BASE = VP9_VLD_REG_OFFSET1;
    }
    else
    {
        //VLD_BASE = LAE_VP9_VLD_OFFSET0;
    }

    vWriteReg(VP9_VLD_REG_OFFSET0 + u4Addr*4, u4Value);
    //DRV_WriteReg( VLD_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_VP9_VLD(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

void vVP9RISCRead_VP9_VLD(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId)
{
    UINT32 VP9_VLD_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_VLD_BASE = VP9_VLD_REG_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //VLD_BASE = VP9_VLD_REG_OFFSET1;
    }
    else
    {
        //VLD_BASE = LAE_VP9_VLD_OFFSET0;
    }

    (*pu4Value) = u4ReadReg(VP9_VLD_REG_OFFSET0 + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( VLD_BASE, u4Addr*4 );
    SIM_PRINT("        RISCRead_VP9_VLD(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCRead_BS2(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId)
{
    UINT32 VP9_BS2_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        VP9_BS2_BASE = VDEC_BS2_OFFSET0;
    }
    else if(u4CoreId == CORE_1_ID)
    {
        //BS2_BASE = VDEC_BS2_OFFSET1;
    }
    else
    {
        //BS2_BASE = LAE_BS2_OFFSET0;
    }

    (*pu4Value) = u4ReadReg(VP9_BS2_BASE + u4Addr*4);
    //(*pu4Value) = DRV_ReadReg( BS2_BASE, u4Addr*4 );
    SIM_PRINT("        RISCRead_BS2(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
}

void vVP9RISCWrite_BS2(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
    UINT32 VP9_BS2_BASE = 0;

    if( u4CoreId == CORE_0_ID)
    {
        VP9_BS2_BASE = VDEC_BS2_OFFSET0;
    }
    else if( u4CoreId == 1)
    {
        //BS2_BASE = VDEC_BS2_OFFSET1;
    }
    else
    {
        //BS2_BASE = LAE_BS2_OFFSET0;
    }

    vWriteReg(VDEC_BS2_OFFSET0 + u4Addr*4, u4Value);
    //DRV_WriteReg( BS2_BASE , u4Addr*4 , u4Value);
    SIM_PRINT("        RISCWrite_BS2(%u, %-10u, %u); // 0x%08x\n", u4Addr, u4Value, u4CoreId, u4Value);
}

#endif
void vVP9RISCWrite_MCore_TOP( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId)
{
#if 0
    UINT32 MCORE_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        MCORE_BASE = MVDEC_TOP_OFFSET0;
    }
    else
    {
        MCORE_BASE = MVDEC_TOP_OFFSET0;
    }

    DRV_WriteReg( MCORE_BASE , u4Addr*4 , u4Value);
    SIM_PRINT ("          RISCWrite_MCORE_TOP(%u, %-10u, %u); // 0x%08x\n",u4Addr, u4Value, u4CoreId, u4Value);
#endif
}

void vVP9RISCRead_MCore_TOP(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId)
{
#if 0
    UINT32 MCORE_BASE = 0;

    if(u4CoreId == CORE_0_ID)
    {
        MCORE_BASE = MVDEC_TOP_OFFSET0;
    }
    else
    {
        MCORE_BASE = MVDEC_TOP_OFFSET0;
    }

    (*pu4Value) = DRV_ReadReg( MCORE_BASE, u4Addr*4 );
    SIM_PRINT("        RISCRead_MCORE_TOP(%u, %u); // 0x%08x\n", u4Addr, u4CoreId, (*pu4Value));
#endif
}

