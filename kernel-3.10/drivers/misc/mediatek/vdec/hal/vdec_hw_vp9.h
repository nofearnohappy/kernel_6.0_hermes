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




// 2013/03/28 Chia-Mao Hung For hevc early emulation

#ifndef _VDEC_HW_VP9_H_
#define _VDEC_HW_VP9_H_

#include <mach/mt_typedefs.h>

extern UINT32 _u4VP9LogOption;

#if 0
#define SIM_PRINT(x, ...) \
    do {                           \
           printk(x, ##__VA_ARGS__);             \
    }while(0)
#else
#define SIM_PRINT(x, ...) \
            do {                           \
                if(_u4VP9LogOption == 2 || _u4VP9LogOption == 3)    \
                   printk(x, ##__VA_ARGS__);             \
            }while(0)
#endif
//#define IO_BASE 0xF6000000
//#define DRV_WriteReg(IO_BASE, dAddr, dVal)  *(volatile UINT32 *)(IO_BASE +  dAddr) = dVal
//#define DRV_ReadReg(IO_BASE, dAddr)        *(volatile UINT32 *)(IO_BASE + dAddr)
//#define vWriteReg(dAddr, dVal)  mt_reg_sync_writel(dVal, IO_BASE + dAddr) //*(volatile UINT32 *)(IO_BASE + dAddr) = dVal
//#define u4ReadReg(dAddr)        *(volatile UINT32 *)(IO_BASE + dAddr)
void vVP9RISCWrite_MC( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_MC( UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_MV( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_MV(UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_PP( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_PP( UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_VLD_TOP( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_VLD_TOP( UINT32 u4Addr, UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCRead_MISC ( UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_MISC( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCWrite_VLD( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_VLD ( UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCRead_VDEC_TOP ( UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_VDEC_TOP( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_VP9_VLD(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_VP9_VLD(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_BS2(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId);
void vVP9RISCWrite_BS2(UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCWrite_MCore_TOP( UINT32 u4Addr, UINT32 u4Value, UINT32 u4CoreId);
void vVP9RISCRead_MCore_TOP(UINT32 u4Addr , UINT32* pu4Value, UINT32 u4CoreId);

void vVP9RISCRead_GCON ( UINT32 u4Addr , UINT32* pu4Value);
void vVP9RISCWrite_GCON( UINT32 u4Addr, UINT32 u4Value );
void vVP9RISC_instructions();


#endif
