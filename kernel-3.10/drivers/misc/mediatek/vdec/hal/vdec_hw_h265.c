#include "vdec_hw_common.h"
#include "../include/vdec_info_h265.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include "vdec_hw_h265.h"

//#include "x_hal_ic.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_info_verify.h"

#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#endif

extern void vVDecOutputDebugString(const CHAR *format, ...);
extern BOOL fgWrMsg2PC(void *pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
extern void vVDecOutputDebugString(const CHAR *format, ...);
#endif

extern VDEC_INFO_DEC_PRM_T _tVerMpvDecPrm[2];
extern ULONG  VA_VDEC_BASE;
extern ULONG  VA_VDEC_GCON_BASE;


#ifdef HEVC_MULTICORE_HW_IP
UINT32 vCheckCoreBase(UINT32 u4VDecID, UINT32 u4SeqID)
{
    if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_CORE_0_ID ){
        switch (u4SeqID)
        {
            case HEVC_COM_VLD:
                return 0x20000;
            case HEVC_VLD_TOP:
                return 0x20800;
            case HEVC_MC:
                return 0x21000;
            case HEVC_MV:
                return 0x23000;
            case HEVC_PP:
                return 0x24000;
            case HEVC_MISC:
                return 0x25000;
            case HEVC_BS2:
                return 0x26800;
            case HEVC_VLD:
                return 0x28000;
            case HEVC_GCON:
                return 0x2F000;
            default:
                printk("[ERROR] HEVC_CORE_0_ID invalid register u4SeqID %d!!!!\n", u4SeqID);
                return -1;
        }
    } else if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_CORE_1_ID ){
        switch (u4SeqID)
        {
            case HEVC_COM_VLD:
                return 0x30000;
            case HEVC_VLD_TOP:
                return 0x30800;
            case HEVC_MC:
                return 0x31000;
            case HEVC_MV:
                return 0x33000;
            case HEVC_PP:
                return 0x34000;
            case HEVC_MISC:
                return 0x35000;
            case HEVC_BS2:
                return 0x36800;
            case HEVC_VLD:
                return 0x38000;
            case HEVC_GCON:
                return 0x3F000;
            default:
                printk("[ERROR] HEVC_CORE_1_ID invalid register u4SeqID %d!!!!\n", u4SeqID);
                return -1;
        }
    } else if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_LAE_0_ID ){
        switch (u4SeqID)
        {
            case HEVC_MISC:
                return 0x10000;
            case HEVC_COM_VLD:
                return 0x11000;
            case HEVC_VLD_TOP:
                return 0x11800;
            case HEVC_MV:
                return 0x13000;
            case HEVC_VLD:
                return 0x15000;
            case HEVC_GCON:
                return 0x0F000;
            default:
                printk("[ERROR] HEVC_LAE_0_ID invalid register u4SeqID %d!!!!\n", u4SeqID);
                return -1;
        }
    } else {
        printk("[WARNNING] invalid u4CoreID  %d!!!!\n", _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID);
    }
}

#else
UINT32 vCheckCoreBase(UINT32 u4VDecID, UINT32 u4SeqID)
{
    switch (u4SeqID)
    {
        case HEVC_COM_VLD:
            return 0x1000;
        case HEVC_VLD_TOP:
            return 0x1800;
        case HEVC_MC:
            return 0x2000;
        case HEVC_MV:
            return 0x4000;
        case HEVC_PP:
            return 0x5000;
        case HEVC_MISC:
            return 0x0;
        case HEVC_VLD:
            return 0x8000;
        case HEVC_GCON:
            return  0;
        default:
            printk("[ERROR] invalid register u4SeqID %d!!!!\n", u4SeqID);
            return -1;
    }
}
#endif

void vWriteHEVCGconReg(UINT32 dAddr, UINT32 dVal)
{

    INT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(0, HEVC_GCON);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
#ifdef HEVC_MULTICORE_HW_IP
    mt_reg_sync_writel(dVal, (ULONG)(VA_VDEC_BASE+u4RegSegBase + dAddr)); //*(volatile UINT32 *)(IO_BASE + dAddr) = dVal
    #ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_GCON(%d, 32'h%x); /* BASE 0x%x */\n", dAddr >> 2, dVal, VA_VDEC_BASE + u4RegSegBase);
    #endif
#else
    mt_reg_sync_writel(dVal, (ULONG)(VA_VDEC_GCON_BASE+u4RegSegBase + dAddr)); //*(volatile UINT32 *)(IO_BASE + dAddr) = dVal
    #ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_GCON(%d, 32'h%x); /* BASE 0x%x */\n", dAddr >> 2, dVal, VA_VDEC_GCON_BASE + u4RegSegBase);
    #endif
#endif
}

UINT32 u4ReadHEVCGconReg(UINT32 dAddr)
{
    UINT32 u4Val;
    INT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(0, HEVC_GCON);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
#ifdef HEVC_MULTICORE_HW_IP
    u4Val = (UINT32)(*(volatile UINT32 *)(VA_VDEC_BASE+u4RegSegBase + dAddr));
    #ifdef VDEC_SIM_DUMP
            printk("        RISCRead_GCON(%d); /* return 0x%x BASE 0x%x */\n", dAddr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
    #endif
#else
    u4Val = (UINT32)(*(volatile UINT32 *)(VA_VDEC_GCON_BASE+u4RegSegBase + dAddr));
    #ifdef VDEC_SIM_DUMP
        printk("        RISCRead_GCON(%d); /* return 0x%x BASE 0x%x */\n", dAddr >> 2, u4Val, VA_VDEC_GCON_BASE + u4RegSegBase);
    #endif

#endif
    return u4Val;
}


void vVDecWriteHEVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_COM_VLD);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_VLD(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}


UINT32 u4VDecReadHEVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_COM_VLD);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = u4ReadReg(u4RegSegBase + u4Addr);
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_VLD(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return u4Val;

}


void vVDecWriteHEVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_VLD);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_HEVC_VLD(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}

UINT32 u4VDecReadHEVCVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_VLD);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_HEVC_VLD(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);

}


void vVDecWriteHEVCMC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_MC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_MC(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}


UINT32 u4VDecReadHEVCMC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_MC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_MC(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);
}



void vVDecWriteHEVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_MV);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_MV((%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif

}

UINT32 u4VDecReadHEVCMV(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_MV);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_MV(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);

}


void vVDecWriteHEVCPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_PP);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_PP(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif

}


UINT32 u4VDecReadHEVCPP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_PP);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_PP(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);
}


void vVDecWriteHEVCMISC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_MISC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_MISC(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif

}


UINT32 u4VDecReadHEVCMISC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_MISC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_MISC(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);
}


void vVDecWriteHEVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_VLD_TOP);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_VLD_TOP(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}

UINT32 u4VDecReadHEVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_VLD_TOP);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = u4ReadReg(u4RegSegBase + u4Addr);
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_VLD_TOP(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return u4Val;

}

void vVDecWriteHEVCBS2(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#ifdef HEVC_MULTICORE_HW_IP
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_BS2);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_BS2(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
#endif
}

UINT32 u4VDecReadHEVCBS2(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val = 0;
#ifdef HEVC_MULTICORE_HW_IP
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vCheckCoreBase(u4VDecID, HEVC_BS2);
    if (u4RegSegBase == -1)
    {
        return 0;
    }

    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_BS2(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
#endif
    return (u4Val);

}


void vVDecWriteHEVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#ifdef HEVC_MULTICORE_HW_IP
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    vWriteReg(HEVC_MCORE_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_MCORE_TOP(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE+HEVC_MCORE_REG_OFFSET0);
#endif
#endif
}

UINT32 u4VDecReadHEVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val = 0;
#ifdef HEVC_MULTICORE_HW_IP
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    u4Val = (u4ReadReg(HEVC_MCORE_REG_OFFSET0 + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_MCORE_TOP(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE+HEVC_MCORE_REG_OFFSET0);
#endif
#endif
    return (u4Val);
}

void vVDecWriteHEVCMCORE_UFO_ENC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    vWriteReg(HEVC_MCORE_UFO_ENC_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
        printk("        RISCWrite_UFO_ENC(%d, 32'h%x); /* BASE 0x%x UFO enc*/\n", u4Addr >> 2, u4Val, VA_VDEC_BASE+HEVC_MCORE_UFO_ENC_REG_OFFSET0);
#endif
}

UINT32 u4VDecReadHEVCMCORE_UFO_ENC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    u4Val = (u4ReadReg(HEVC_MCORE_UFO_ENC_REG_OFFSET0 + u4Addr));
#ifdef VDEC_SIM_DUMP
        printk("        RISCRead_UFO_ENC(%d); /* return 0x%x BASE 0x%x UFO enc*/\n", u4Addr >> 2, u4Val, VA_VDEC_BASE+HEVC_MCORE_UFO_ENC_REG_OFFSET0);
#endif
    return (u4Val);
}

void vVDecHEVCSetCoreState(UCHAR u4VDecID, UINT32 u4CoreID)
{
    _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID = u4CoreID;
    printk("[INFO] vVDEC_HAL_H265_Set_Core_State u4CoreID = %d\n", u4CoreID);
}


void vVDecHEVCUFOEncBreak(UINT32 u4VDecID)
{
    UINT32 u4Cnt = 0;
    UINT32 u4RetVal;
    UINT32 u4RetVal2;
    UINT32 u4RetryCount, u4RetryMax = 5000;

    u4RetVal = u4VDecReadHEVCMCORE_UFO_ENC(u4VDecID, 36 * 4);
    vVDecWriteHEVCMCORE_UFO_ENC(u4VDecID, 36 * 4, 1<<4 | u4RetVal);

    u4RetVal = (u4VDecReadHEVCMCORE_UFO_ENC(u4VDecID, 35*4)>>12 & 0x1);
    u4RetVal2  = (u4VDecReadHEVCMCORE_UFO_ENC(u4VDecID, 35*4)>>16 & 0x1);

    while (u4RetVal== 0 || u4RetVal2== 0) //Polling Larb Ready
    {
        u4RetVal = (u4VDecReadHEVCMCORE_UFO_ENC(u4VDecID, 35*4)>>12 & 0x1);
        u4RetVal2  = (u4VDecReadHEVCMCORE_UFO_ENC(u4VDecID, 35*4)>>16 & 0x1);

        u4RetryCount++;
        if (u4RetryCount>u4RetryMax)
        {
            printk("[ERROR] UFO break Polling Ready timeout!!!\n");
            break;
        }
    }

    u4RetVal = u4VDecReadHEVCMCORE_UFO_ENC(u4VDecID, 36 * 4);
    vVDecWriteHEVCMCORE_UFO_ENC(u4VDecID, 36 * 4, (~(1<<4)) & u4RetVal);

}


// *********************************************************************
// Function : void vVDecResetVLDHW(UINT32 u4VDecID)
// Description : Reset Video decode HW
// Parameter : u4VDecID : VLD ID
// Return    : None
// *********************************************************************
void vVDecHEVCResetHW(UINT32 u4VDecID, UINT32 u4VDecType)
{
    UINT32 u4Cnt = 0;
    UINT32 u4RetVal;
    UINT32 u4VDecPDNCtrl;
    UINT32 u4VDecSysClk;
    UINT32 u4VDecPDNCtrlSpec;
    UINT32 u4VDecPDNCtrlModule;

    // VDEC Power on check
    vWriteHEVCGconReg(0 * 4, 0x1);
    u4ReadHEVCGconReg (0);

    u4Cnt = 50000;
#ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);  //HWReset WaitSramStable\n");
#endif
    if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
    {
        while ((!(u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
    }

    if (bMode_MCORE[u4VDecID]){
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_CORE_0_ID ){
            vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_1_ID);
            u4Cnt = 50000;
            if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
            {
                while ((!(u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
            }
            vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_0_ID);

            //MCORE_TOP Soft Reset
            vVDecWriteHEVCMCORE_TOP(u4VDecID, 7*4, 0x1);
            vVDecWriteHEVCMCORE_TOP(u4VDecID, 7*4, 0x0);
        }

        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_LAE_0_ID ){
            u4RetVal = u4VDecReadHEVCMCORE_TOP(u4VDecID, 14*4);
            vVDecWriteHEVCMCORE_TOP(u4VDecID, 14*4, (u4RetVal | (1 << 24)));
        }
    }

    if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.bIsUFOMode == 1)
    {
        vVDecHEVCUFOEncBreak(u4VDecID);
        // Multi core UFO enc reset
        vVDecWriteHEVCMCORE_UFO_ENC(u4VDecID, 36 * 4, 0x1);
        vVDecWriteHEVCMCORE_UFO_ENC(u4VDecID, 36 * 4, 0x0);
    }

    u4VDecPDNCtrlSpec = 0x000000FF;
    u4VDecPDNCtrlModule = 0x37E30180;
    u4VDecSysClk = 0x00000002;

    vVDecWriteHEVCMISC(u4VDecID, 0xF4, 0x0);
    vVDecWriteHEVCCOMVLD(u4VDecID, WO_VLD_SRST, 0x101);
    vVDecWriteHEVCMISC(u4VDecID, 0xC8,  0x000000FF);
    vVDecWriteHEVCMISC(u4VDecID, 0xCC,  0x37E30180);

    // all platform
    vVDecWriteHEVCMISC(u4VDecID, 0xC8,  0);
    vVDecWriteHEVCMISC(u4VDecID, 0xCC,  0);

    vVDecWriteHEVCMISC(u4VDecID, 0x178, 0x00000020);
    //vVDecWriteHEVCMISC(u4VDecID, 0x4, 0x10);    //K_2 turn on MISC_2-9 CRC access
    vVDecWriteHEVCMISC(u4VDecID, 0x4, 0x110);    //EVE_REST turn on MISC_95-102 CRC access
    vVDecWriteHEVCMCORE_TOP(u4VDecID, 192*4, 0x1);  //RO_CKY turn on MCORE_TOP_193 -200 (Core0)  MCORE_TOP_201 -208 (Core1) CRC access
    vVDecWriteHEVCMISC(u4VDecID, RW_SYS_CLK_SEL, u4VDecSysClk);

    // Turn off auto-power-off (DCM) after IRQ
    vVDecWriteHEVCMISC(u4VDecID, 61 * 4, 0x0);  // MISC(61)
    vVDecWriteHEVCMISC(u4VDecID, 59 * 4, 0x1);
    vWriteHEVCGconReg(6 * 4, 0x1);
    vVDecWriteHEVCMCORE_TOP(u4VDecID, 0xF000+6*4, 0x1);    // MCore GCON
    vVDecWriteHEVCMISC(u4VDecID, RW_PDN_CTRL, ((u4VDecReadHEVCMISC(u4VDecID, RW_PDN_CTRL) & 0xFF400000) | u4VDecPDNCtrl));   // for VP8 bit 23 can not be 1

    //printk("sysclk = 0x%x,pwd = 0x%x,HWID = %d,codec = 0x%x\n",u4VDecReadDV(u4VDecID, RW_SYS_CLK_SEL),u4VDecReadDV(u4VDecID, RW_PDN_CTRL)&0x3fffff,u4VDecID,u4VDecType);
    vVDecWriteHEVCCOMVLD(u4VDecID, WO_VLD_SRST, 0);

    if (bMode_MCORE[u4VDecID]){
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_LAE_0_ID ){
            u4RetVal = u4VDecReadHEVCMCORE_TOP(u4VDecID, 14*4);
            vVDecWriteHEVCMCORE_TOP(u4VDecID, 14*4, (u4RetVal & ~(1 << 24)));
        }
        //Disable Mcore Decode: mcore_mode off
        vVDecWriteHEVCMCORE_TOP(u4VDecID, 24*4, 0x0);
    }

    //@ For fake engine test
#ifdef VDEC_BW_FAKE_ENGINE_ON
    extern UINT32 _u4PicCnt[2];
    extern UCHAR *_pucFAKE[2];
    UINT32 u4Val;

    u4Val = u4VDecReadHEVCMISC(u4VDecID, 0x228);
    if (((u4Val >> 16) & 0x1))
    {
        printk("[INFO] Fake engine test! [frame %d] \n", _u4PicCnt[u4VDecID]);
        UINT32 addr_offset = PHYSICAL((UINT32)(_pucFAKE[u4VDecID])) >> 4;
        vVDecWriteHEVCMISC(u4VDecID, 0x210, addr_offset);   //MISC_132fake engine start address offset (size:256*16 align)
        //vVDecWriteHEVCMISC(u4VDecID, 0x214, 0x00100010);    //MISC_133[15:0]= 16; MISC_133[31:16]= 16,       16 requests & sleep 16 cycles
        //vVDecWriteHEVCMISC(u4VDecID, 0x214, 0x01000010);    //MISC_133[15:0]= 16; MISC_133[31:16]= 256,      16 requests & sleep 256 cycles
        vVDecWriteHEVCMISC(u4VDecID, 0x214, 0x01000008);    //MISC_133[15:0]= 8; MISC_133[31:16]= 256,      8 requests & sleep 256 cycles
        vVDecWriteHEVCMISC(u4VDecID, 0x218, 0x10001);       //MISC_134[0]= 0x10001 ([0]alw_rd, [16]fake start)
        u4Val = u4VDecReadHEVCMISC(u4VDecID, 0x210);
        printk("[INFO] MISC_132 u4Val 0x%x \n", u4Val);
        u4Val = u4VDecReadHEVCMISC(u4VDecID, 0x214);
        printk("[INFO] MISC_133 u4Val 0x%x \n", u4Val);
        u4Val = u4VDecReadHEVCMISC(u4VDecID, 0x218);
        printk("[INFO] MISC_134 u4Val 0x%x !\n", u4Val);
    }
    else
    {
        printk("[INFO] Fake engine still activated!! [frame %d] \n", _u4PicCnt[u4VDecID]);
    }
#endif

/*
    // VDEC register check
   vVDecWriteHEVCMC(u4VDecID, HEVC_Y_OUTPUT, 0xFFFFFFFF);
   u4VDecReadHEVCMC(u4VDecID, HEVC_Y_OUTPUT);
*/

}


// *********************************************************************
// Function : UINT32 u4VDecHEVCVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecHEVCVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 dShiftBit)
{

    UINT32 u4RegVal, u4index;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    for (u4index = 0; u4index < (dShiftBit / 32);  u4index++)
    {
        u4VDecReadHEVCVLD(u4VDecID, RO_HEVLD_BARL + (32 << 2));
    }

    u4RegVal = u4VDecReadHEVCVLD(u4VDecID, RO_HEVLD_BARL + ((dShiftBit % 32) << 2));
    return (u4RegVal);
}

// *********************************************************************
// Function : UINT32 u4VDecHEVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Get HEVCVLD shift bits %64
// Parameter : None
// Return    : VLD Sum
// *********************************************************************
UINT32 u4VDecHEVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    return ((u4VDecReadHEVCVLD(u4VDecID, RW_HEVLD_CTRL) >> 8) & 0x3F);

}

// *********************************************************************
// Function : BOOL fgVDecHEVCIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Check if VLD fetch is done
// Parameter : None
// Return    : TRUE: VLD fetch OK, FALSE: not OK
// *********************************************************************
BOOL fgVDecHEVCIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
{
    if ((u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_FETCHOK + (u4BSID << 10)) & VLD_FETCH_OK) == 0)
    {
        return (FALSE);
    }
    return (TRUE);
}


BOOL fgVDecHEVCWaitVldFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4Cnt;

#ifdef VDEC_SIM_DUMP
    printk("wait(`VDEC_INI_FETCH_RDY == 1);\n");
#endif

    if (!fgVDecHEVCIsVLDFetchOk(u4BSID, u4VDecID))
    {
        u4Cnt = 0;
        while (!fgVDecHEVCIsVLDFetchOk(u4BSID, u4VDecID))
        {
            u4Cnt++;
            if (u4Cnt >= 0x1000)
            {
                return (FALSE);
            }
        }
    }
    return (TRUE);
}



// *********************************************************************
// Function : BOOL fgH265VLDInitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
// Description : Init HW Barrel Shifter
// Parameter : u4Ptr: Physical DRAM Start Address to fill Barrel Shifter
// Return    : TRUE: Initial Success, Fail: Initial Fail
// *********************************************************************
BOOL fgH265VLDInitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4FIFOSa, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
{
    UINT32 u4ByteAddr;
    UINT32 u4TgtByteAddr;
    INT32 i;
    BOOL fgFetchOK = FALSE;
    UINT32 u4PreventionBytes, u4RetVal;
    UINT32 u4ShiftBytes, u4ShiftBytesAcc, u4EmuCnt;
    UINT32 u4Cnt;

    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
#ifdef VDEC_SIM_DUMP
        printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);  //InitBS WaitSramStable\n");
#endif

        u4Cnt = 50000;
        if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1 << 15))
        {
            while ((!(u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
        }

        if (bMode_MCORE[u4VDecID]){
            if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_CORE_0_ID ){
                vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_1_ID);
                u4Cnt = 50000;
                if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1 << 15))
                {
                    while ((!(u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
                }
                vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_0_ID);
            }
        }

        // read pointer
        vVDecWriteHEVCCOMVLD(u4VDecID, RW_VLD_RPTR , u4VLDRdPtr);

        // bitstream DMA async_FIFO  local reset
        vVDecWriteHEVCCOMVLD(u4VDecID, WO_VLD_SRST , 1 << 8);
        vVDecWriteHEVCCOMVLD(u4VDecID, WO_VLD_SRST , 0);

        // start to fetch data
        vVDecWriteHEVCCOMVLD(u4VDecID, RW_VLD_PROC, VLD_INIFET);
        if (fgVDecHEVCWaitVldFetchOk(u4BSID, u4VDecID))
        {
            if (bMode_MCORE[u4VDecID]){
                if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_CORE_0_ID ){
                    vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_1_ID);
                    if (fgVDecHEVCWaitVldFetchOk(u4BSID, u4VDecID)){
                        fgFetchOK = TRUE;
                    }
                    vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_0_ID);
                    break;
                }
            }
            fgFetchOK = TRUE;
            break;
        }

    }

    if (!fgFetchOK)
    {
        return (FALSE);
    }

    // initial barrel shifter
    vVDecWriteHEVCCOMVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);
    u4ShiftBytes = (u4VLDRdPtr & 0xF);

    // check next 5bytes status (for "0x03" prevention): precise to  byte
    u4ShiftBytesAcc = 0;
    u4EmuCnt = 0;
    while ((u4ShiftBytesAcc + u4EmuCnt) < u4ShiftBytes)
    {
        u4RetVal = u4VDecReadHEVCVLD(u4VDecID, RW_HEVLD_CTRL);
        if ((u4RetVal >> (24 + (u4ShiftBytesAcc % 4) + u4EmuCnt)) & 0x1)
        {
            u4EmuCnt++;
        }
        else
        {
            u4VDecHEVCVLDGetBitS(u4BSID, u4VDecID,  8);       // FW_shift precise to byte address
            u4ShiftBytesAcc++;

            if (u4ShiftBytesAcc % 4 == 0)
            {
                u4ShiftBytesAcc += (((u4RetVal >> 24) & 0x1) + ((u4RetVal >> 25) & 0x1) + ((u4RetVal >> 26) & 0x1) + ((u4RetVal >> 27) & 0x1));
                u4EmuCnt -= (((u4RetVal >> 24) & 0x1) + ((u4RetVal >> 25) & 0x1) + ((u4RetVal >> 26) & 0x1) + ((u4RetVal >> 27) & 0x1));
            }
        }
        //DBG_H265_PRINTF(pfLogFile,"[Info] u4ShiftBytesAcc %d; u4EmuCnt %d\n", u4ShiftBytesAcc, u4EmuCnt);
    }

#ifdef VDEC_SIM_DUMP
    printk("[INFO] {H265 DEC >> } u4InstID = 0x%x, Input Window: 0x%08x\n", u4VDecID, u4VDecReadHEVCVLD(u4VDecID, RO_HEVLD_BARL));
#endif
    return (TRUE);
}


// *********************************************************************
// Function : BOOL fgInitH265BarrelShift(UINT32 u4VDecID, VDEC_INFO_H265_BS_INIT_PRM_T *prH265BSInitPrm)
// Description : Reset VLD2
// Parameter : None
// Return    : None
// *********************************************************************
BOOL fgInitH265BarrelShift(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H265_BS_INIT_PRM_T *prH265BSInitPrm)
{

    vVDecWriteHEVCVLD(u4VDecID, RW_HEVLD_CTRL, HEVC_EN);
    vVDecWriteHEVCCOMVLD(u4VDecID, 59 * 4, (0x1 << 28));
    if (bMode_MCORE[u4VDecID]){
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID != HEVC_LAE_0_ID ){
            vVDecWriteHEVCCOMVLD(u4VDecID, 80* 4, (0x1 << 20));
            vVDecWriteHEVCBS2(u4VDecID, 59 * 4, (0x1 << 28));
            vVDecWriteHEVCBS2(u4VDecID, 80 * 4, (0x1 << 20));
            vVDecWriteHEVCBS2(u4VDecID, RW_VLD_VSTART + (u4BSID << 10), PHYSICAL((UINT32) prH265BSInitPrm->u4VFifoSa) >> 6);
            vVDecWriteHEVCBS2(u4VDecID, RW_VLD_VEND + (u4BSID << 10), PHYSICAL((UINT32) prH265BSInitPrm->u4VFifoEa) >> 6);
        }
    }

    vVDecWriteHEVCCOMVLD(u4VDecID, RW_VLD_VSTART + (u4BSID << 10), PHYSICAL((UINT32) prH265BSInitPrm->u4VFifoSa) >> 6);
    vVDecWriteHEVCCOMVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10), PHYSICAL((UINT32) prH265BSInitPrm->u4VFifoEa) >> 6);
    //vVDecWriteHEVCCOMVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, u4AbsDramANc((UINT32) prH265BSInitPrm->u4PredSa));

#ifdef VDEC_SIM_DUMP
    printk("[INFO] u4VFifoSa = 0x%08x, u4VFifoEa = 0x%08x\n", PHYSICAL(prH265BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prH265BSInitPrm->u4VFifoEa));
#endif

    // Reset HEVC VLD Sum
    /*
    vVDecWriteHEVCVLD(u4VDecID, RW_HEVLD_RESET_SUM, HEVLD_RESET_SUM_ON);
    vVDecWriteHEVCVLD(u4VDecID, RW_HEVLD_RESET_SUM, HEVLD_RESET_SUM_OFF);
    */

    if (!fgH265VLDInitBarrelShifter(0, u4VDecID, PHYSICAL((UINT32) prH265BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prH265BSInitPrm->u4VLDRdPtr), PHYSICAL(prH265BSInitPrm->u4VLDWrPtr)))
    {
        return FALSE;
    }
    return TRUE;
}


UINT32 u4VDecReadH265VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
    UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
    UINT32 u4RegVal, u4SramCtr;
    UINT32 vb_sram_ra, vb_sram_wa, seg_rcnt;
    UINT32 u4Cnt = 0;
    UINT32 u4BsBufLen = 0;

    // HW issue, wait for read pointer stable
    u4Cnt = 50000;
    //if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))

#ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
#endif

    if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (PROCESS_FLAG))
    {
        while ((!(u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
    }

    if (bMode_MCORE[u4VDecID]){
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH265DecPrm.u4CoreID == HEVC_CORE_0_ID ){
            vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_1_ID);
            if (u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (PROCESS_FLAG))
            {
                while ((!(u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
            }
            vVDecHEVCSetCoreState(u4VDecID, HEVC_CORE_0_ID);
        }
    }

    u4RegVal = u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_VBAR);
    vb_sram_ra = u4RegVal & 0x1F;
    vb_sram_wa = (u4RegVal >> 8) & 0x1F;
    seg_rcnt = (u4RegVal >> 24) & 0x3;


    u4SramRptr = vb_sram_ra;
    u4SramWptr = vb_sram_wa;
    u4SramCtr = seg_rcnt;
    u4DramRptr = u4VDecReadHEVCCOMVLD(u4VDecID, RO_VLD_VRPTR);

    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 32 - (u4SramRptr - u4SramWptr);
    }

    (*pu4Bits) = u4VDecHEVCVLDShiftBits(u4BSID, u4VDecID);
#ifdef VDEC_SIM_DUMP
    printk("[INFO] ReadH265VldRPtr, dRptr:0x%08X, sra:0x%08X, swa:0x%08X, scnt:0x%08X, sum:0x%08X\n",
           u4DramRptr, vb_sram_ra, vb_sram_wa, seg_rcnt, *pu4Bits);
#endif


    u4BsBufLen = 7 * 4;
    u4ByteAddr = u4DramRptr - u4SramDataSz * 16 + u4SramCtr * 4 - u4BsBufLen + (*pu4Bits) / 8;

    //(*pu4Bits) &= 0x7;
    //HEVC "03" consumsion align 4
    u4ByteAddr = (u4ByteAddr >> 2) << 2;

    //printk("[INFO] Calculated u4ByteAddr: 0x%08X,  u4VFIFOSa: 0x%08X\n", u4ByteAddr, u4VFIFOSa );

    if (u4ByteAddr < u4VFIFOSa)
    {

        u4ByteAddr = u4ByteAddr
                     + ((u4VDecReadHEVCCOMVLD(u4VDecID, RW_VLD_VEND) << 6) - ((UINT32)u4VFIFOSa))
                     - u4VFIFOSa;
    }
    else
    {
        u4ByteAddr -= ((UINT32)u4VFIFOSa);
    }

#ifdef VDEC_SIM_DUMP
    //printk("[INFO] ReadH265VldRPtr, RdPtr=0x%08X (%u) @(%s, %d)\n", u4ByteAddr, u4ByteAddr);
    printk("[INFO] Return u4ByteAddr: 0x%08X, Input window: 0x%08X\n", u4ByteAddr, u4VDecReadHEVCVLD(u4VDecID, RO_HEVLD_BARL));
#endif

    return (u4ByteAddr);
}




