#include "vdec_hw_common.h"
#include "../include/vdec_info_h264.h"
#include "vdec_hw_h264.h"
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
extern ULONG VA_VDEC_BASE;
extern BOOL bEnable_UFO[2];
extern BOOL bMode_MCORE[2];

enum u4RegBaseSeqID
{
    AVC_COM_VLD,
    AVC_VLD_TOP,
    AVC_MC,
    AVC_VLD,
    AVC_MV,
    AVC_PP,
    AVC_MISC,
    AVC_BS2,
    AVC_GCON
};

UINT32 vAVCCheckCoreBase(UINT32 u4VDecID, UINT32 u4SeqID)
{
    if (u4VDecID == 0 ){
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_CORE_0_ID ){
            switch (u4SeqID)
            {
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
                case AVC_COM_VLD:   return 0x20000;
                case AVC_VLD_TOP:   return 0x20800;
                case AVC_MC:        return 0x21000;
                case AVC_VLD:       return 0x22000;
                case AVC_MV:        return 0x23000;
                case AVC_PP:        return 0x24000;
                case AVC_MISC:      return 0x25000;
                case AVC_BS2:       return 0x26800;
                case AVC_GCON:      return 0x2F000;
#else
                case AVC_COM_VLD:   return VLD_REG_OFFSET0;
                case AVC_VLD_TOP:   return VLD_TOP_REG_OFFSET0;
                case AVC_MC:        return MC_REG_OFFSET0;
                case AVC_VLD:       return AVC_VLD_REG_OFFSET0;
                case AVC_MV:        return AVC_MV_REG_OFFSET0;
                case AVC_PP:        return PP_REG_OFFSET0;
                case AVC_MISC:      return DV_REG_OFFSET0;
#endif
                default:               
                    printk("[WARNNING] AVC_CORE_0_ID invalid register u4SeqID %d!!!!\n", u4SeqID);
                    return -1;
            }
        } else if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_CORE_1_ID ){
            switch (u4SeqID)
            {
                case AVC_COM_VLD:   return 0x30000;
                case AVC_VLD_TOP:   return 0x30800;
                case AVC_MC:        return 0x31000;
                case AVC_VLD:       return 0x32000;
                case AVC_MV:        return 0x33000;
                case AVC_PP:        return 0x34000;
                case AVC_MISC:      return 0x35000;
                case AVC_BS2:       return 0x36800;
                case AVC_GCON:      return 0x3F000;
                default:               
                    printk("[WARNNING] AVC_CORE_1_ID invalid register u4SeqID %d!!!!\n", u4SeqID);
                    return -1;
            }
        } else if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_LAE_0_ID ){
            switch (u4SeqID)
            {
                case AVC_MISC:      return 0x10000;
                case AVC_COM_VLD:   return 0x11000;
                case AVC_VLD_TOP:   return 0x11800;
                case AVC_VLD:       return 0x12000;
                case AVC_MV:        return 0x13000;
                case AVC_GCON:      return 0x0F000;
                default:               
                    printk("[WARNNING] AVC_LAE_0_ID invalid register u4SeqID %d!!!!\n", u4SeqID);
                    return -1;
            }
        } else {
            printk("[WARNNING] invalid u4CoreID  %d!!!!\n", _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID);
            return -1;
        }
    } else {
        switch (u4SeqID)
        {
            case AVC_COM_VLD:   return VLD_REG_OFFSET1;
            case AVC_VLD_TOP:   return VLD_TOP_REG_OFFSET1;
            case AVC_MC:        return MC_REG_OFFSET1;
            case AVC_MV:        return AVC_MV_REG_OFFSET1;
            case AVC_PP:        return PP_REG_OFFSET0;
            case AVC_MISC:      return 0x0;
            case AVC_VLD:       return AVC_VLD_REG_OFFSET1;
            default:               
                printk("[WARNNING] AVC u4VDecID=1 invalid register u4SeqID %d!!!!\n", u4SeqID);
                return -1;
        }
    }
}
void vWriteAVCGconReg(UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(0, AVC_GCON);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_GCON(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif

#else
    vWriteGconReg(u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_GCON(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_GCON_BASE);
#endif

#endif
}
UINT32 u4ReadAVCGconReg(UINT32 u4Addr)
{
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
    UINT32 u4Val;
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(0, AVC_GCON);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = u4ReadReg(u4RegSegBase + u4Addr);
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_GCON(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif

#else
    u4Val = u4ReadGconReg(u4Addr);
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_GCON(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_GCON_BASE);
#endif

#endif
    return u4Val;
}
void vVDecWriteAVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_COM_VLD);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_COM_VLD(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
        //vVDecSimDump(u4VDecID, AVC_VLD_REG_OFFSET0, u4Addr, u4Val);
#endif
    }

UINT32 u4VDecReadAVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_COM_VLD);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = u4ReadReg(u4RegSegBase + u4Addr);
    if (u4VDecID == 0) {
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_COM_VLD(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    } else {
#ifdef VDEC_PIP_WITH_ONE_HW
    printk("PIP_ONE_HW: Wrong HW ID!!!\n");
    VDEC_ASSERT(0);
#endif
    }
    return u4Val;
}
void vVDecWriteAVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_VLD);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_VLD(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}

UINT32 u4VDecReadAVCVLD(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_VLD);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_VLD(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);
}
void vVDecWriteAVCMC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MC);
    if (u4RegSegBase == -1)
    {
        return;
    }
    if (u4VDecID != 0) {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_MC(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}
UINT32 u4VDecReadAVCMC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_MC(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);   
}

void vVDecWriteAVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MV);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_MV((%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}
UINT32 u4VDecReadAVCMV(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MV);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_MV(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
        return (u4Val);
}
void vVDecWriteAVCPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_PP);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_PP(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}

UINT32 u4VDecReadAVCPP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_PP);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_PP(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
        return (u4Val);
}
void vVDecWriteAVCMISC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MISC);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_MISC(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}

UINT32 u4VDecReadAVCMISC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MISC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_MISC(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);
}

void vVDecWriteAVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_VLD_TOP);
    if (u4RegSegBase == -1)
    {
        return;
    }
    if (u4VDecID != 0) {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_VLD_TOP(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    }

UINT32 u4VDecReadAVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_VLD_TOP);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    if (u4VDecID != 0) {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
    }
    u4Val = u4ReadReg(u4RegSegBase + u4Addr);
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_VLD_TOP(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return u4Val;
    }
void vVDecWriteAVCUFO(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    vWriteReg(UFO_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_UFO(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + UFO_REG_OFFSET0);
#endif
}

UINT32 u4VDecReadAVCUFO(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    u4Val = (u4ReadReg(UFO_REG_OFFSET0 + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_UFO(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + UFO_REG_OFFSET0);
#endif
    return (u4Val);
}
void vVDecWriteAVCCRC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MISC);
    if (u4RegSegBase == -1)
    {
        return;
    }
    
    if (u4VDecID != 0) {
#ifdef VDEC_PIP_WITH_ONE_HW
        printk("PIP_ONE_HW: Wrong HW ID!!!\n");
        VDEC_ASSERT(0);
#endif
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_CRC(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}

UINT32 u4VDecReadAVCCRC(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_MISC);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = u4ReadReg(u4RegSegBase + u4Addr);
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_CRC(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return u4Val;
}

void vVDecWriteAVCFG(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(AVC_FG_REG_OFFSET0 + u4Addr, u4Val);
    }
    else
    {
        vWriteReg(AVC_FG_REG_OFFSET1 + u4Addr, u4Val);
    }
}

UINT32 u4VDecReadAVCFG(UINT32 u4VDecID, UINT32 u4Addr)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        return (u4ReadReg(AVC_FG_REG_OFFSET0 + u4Addr));
    }
    else
    {
        return (u4ReadReg(AVC_FG_REG_OFFSET1 + u4Addr));
    }
}

void vVDecWriteAVCBS2(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_BS2);
    if (u4RegSegBase == -1)
    {
        return;
    }
    vWriteReg(u4RegSegBase + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_BS2(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
}
UINT32 u4VDecReadAVCBS2(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    UINT32 u4RegSegBase;
    u4RegSegBase = vAVCCheckCoreBase(u4VDecID, AVC_BS2);
    if (u4RegSegBase == -1)
    {
        return 0;
    }
    u4Val = (u4ReadReg(u4RegSegBase + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_BS2(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + u4RegSegBase);
#endif
    return (u4Val);
}
void vVDecWriteAVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    vWriteReg(MCORE_TOP_REG_OFFSET0 + u4Addr, u4Val);
#ifdef VDEC_SIM_DUMP
    printk("        RISCWrite_AVC_MCORE_TOP(%d, 32'h%x); /* BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + MCORE_TOP_REG_OFFSET0);
#endif
}
UINT32 u4VDecReadAVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    u4Val = (u4ReadReg(MCORE_TOP_REG_OFFSET0 + u4Addr));
#ifdef VDEC_SIM_DUMP
    printk("        RISCRead_AVC_MCORE_TOP(%d); /* return 0x%x BASE 0x%x */\n", u4Addr >> 2, u4Val, VA_VDEC_BASE + MCORE_TOP_REG_OFFSET0);
#endif
    return (u4Val);
}
void vVDecAVCSetCoreState(UCHAR u4VDecID, UINT32 u4CoreID)
{
    _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID = u4CoreID;
    printk("[INFO] vVDEC_HAL_H264_Set_Core_State u4CoreID = %d\n", u4CoreID);
}
void vVDecAVCResetHW(UINT32 u4VDecID, UINT32 u4VDecType)
{
    UINT32 u4Cnt = 0;
    UINT32 u4VDecPDNCtrl;
    UINT32 u4VDecSysClk;
    UINT32 u4VDecPDNCtrlSpec;
    UINT32 u4VDecPDNCtrlModule;
    UINT32 u4VDecPDNCtrlModule2;
    
#ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);  //HWReset WaitSramStable [AVC]\n");
#endif

    // HW issue, wait for read pointer stable
    u4Cnt = 50000;
    if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
    {
        while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
    }
    if (bMode_MCORE[u4VDecID]) {
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_CORE_0_ID ){
            vVDecAVCSetCoreState(u4VDecID, AVC_CORE_1_ID);
            u4Cnt = 50000;
            if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & PROCESS_FLAG)
            {
                while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL)&AA_FIT_TARGET_SCLK)) && (u4Cnt--));
            }
            vVDecAVCSetCoreState(u4VDecID, AVC_CORE_0_ID);
        }
        if ( _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_LAE_0_ID ){
            vVDecWriteAVCMCORE_TOP(u4VDecID, 14*4, u4VDecReadAVCMCORE_TOP(u4VDecID, 14*4) | 1 << 24);
        }
    }
    vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_SRST, (0x1 | (0x1 << 8)));
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
    if (bEnable_UFO[u4VDecID]) {
        vVDecWriteAVCUFO(u4VDecID, 36 * 4, u4VDecReadAVCUFO(u4VDecID, 36 * 4) | 0x1);
    }
#endif
    u4VDecPDNCtrl = 0x00000FF0;
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT6735)
    u4VDecPDNCtrlSpec = 0x0;
    u4VDecPDNCtrlModule = 0x0;
    u4VDecPDNCtrlModule2 = 0x60;
#else
    u4VDecPDNCtrlSpec = 0xF7;
    u4VDecPDNCtrlModule = 0x13A20100;
    u4VDecPDNCtrlModule2 = 0x1f;
#endif
    u4VDecSysClk = 0x00000002;
    vVDecWriteAVCMISC(u4VDecID, RW_SYS_CLK_SEL, u4VDecSysClk);
#ifdef REALCHIP_DVT
    // Per spec configuration
    vVDecWriteAVCMISC(u4VDecID, 0xC8, u4VDecPDNCtrlSpec);
    vVDecWriteAVCMISC(u4VDecID, 0xCC, u4VDecPDNCtrlModule);
    vVDecWriteAVCMISC(u4VDecID, 0x178, u4VDecPDNCtrlModule2 /*0x60*/);  // MISC(94)
    // Turn off auto-power-off after IRQ
    vVDecWriteAVCMISC(u4VDecID, 0xf4, 0);  // MISC(61)
    vWriteAVCGconReg(6 * 4, 1);
    vVDecWriteAVCMISC(u4VDecID, 59 * 4, 1);
#endif
    vVDecWriteAVCMISC(u4VDecID, RW_PDN_CTRL, ((u4VDecReadAVCMISC(u4VDecID, RW_PDN_CTRL) & 0xFF400000) | u4VDecPDNCtrl));   // for VP8 bit 23 can not be 1

    // printk("sysclk = 0x%x,pwd = 0x%x,HWID = %d,codec = 0x%x\n",u4VDecReadDV(u4VDecID, RW_SYS_CLK_SEL),u4VDecReadDV(u4VDecID, RW_PDN_CTRL)&0x3fffff,u4VDecID,u4VDecType);
    vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_SRST, 0);
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
    if (bEnable_UFO[u4VDecID]) {
        vVDecWriteAVCUFO(u4VDecID, 36 * 4, u4VDecReadAVCUFO(u4VDecID, 36 * 4) & ~0x1);
    }
#endif

    if ( bMode_MCORE[u4VDecID] && _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_LAE_0_ID ){
        vVDecWriteAVCMCORE_TOP(u4VDecID, 14*4, u4VDecReadAVCMCORE_TOP(u4VDecID, 14*4) & ~(1 << 24));
    }

    //@ For fake engine test
#ifdef VDEC_BW_FAKE_ENGINE_ON
    extern UINT32 _u4PicCnt[2];
    extern UCHAR *_pucFAKE[2];
    UINT32 u4Val;
    u4Val = u4VDecReadAVCMISC(u4VDecID, 0x228);
    if (((u4Val >> 16) & 0x1))
    {
        printk("[INFO] Fake engine test! [frame %d] \n", _u4PicCnt[u4VDecID]);
        UINT32 addr_offset = PHYSICAL((UINT32)(_pucFAKE[u4VDecID])) >> 4;
        vVDecWriteAVCMISC(u4VDecID, 0x210, addr_offset);   //MISC_132fake engine start address offset (size:256*16 align)
        vVDecWriteAVCMISC(u4VDecID, 0x214, 0x01000008);    //MISC_133[15:0]= 8; MISC_133[31:16]= 256,      8 requests & sleep 256 cycles
        vVDecWriteAVCMISC(u4VDecID, 0x218, 0x10001);       //MISC_134[0]= 0x10001 ([0]alw_rd, [16]fake start)
        printk("[INFO] MISC_132 u4Val 0x%x \n", u4VDecReadAVCMISC(u4VDecID, 0x210));
        printk("[INFO] MISC_133 u4Val 0x%x \n", u4VDecReadAVCMISC(u4VDecID, 0x214));
        printk("[INFO] MISC_134 u4Val 0x%x !\n", u4VDecReadAVCMISC(u4VDecID, 0x218));
    }
    else
    {
        printk("[INFO] Fake engine still activated!! [frame %d] \n", _u4PicCnt[u4VDecID]);
    }
#endif
}
// *********************************************************************
// Function : UINT32 dVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit)
// Description : Get Bitstream from VLD barrel shifter
// Parameter : dShiftBit: Bits to shift (0-32)
// Return    : barrel shifter
// *********************************************************************
UINT32 u4VDecAVCVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 dShiftBit)
{

    UINT32 u4RegVal;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif
    if (u4BSID == 0)
    {
        u4RegVal = u4VDecReadAVCVLD(u4VDecID, RO_AVLD_BARL  + (dShiftBit << 2));
    }
    else
    {
        u4RegVal = u4VDecReadAVCVLD(u4VDecID, RO_AVLD_2ND_BARL  + (dShiftBit << 2));
    }
    return (u4RegVal);
}

// *********************************************************************
// Function : UINT32 u4VDecAVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Get AVCVLD shift bits %64
// Parameter : None
// Return    : VLD Sum
// *********************************************************************
UINT32 u4VDecAVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

    if (u4BSID == 0)
    {
        return ((u4VDecReadAVCVLD(u4VDecID, RW_AVLD_CTRL) >> 16) & 0x3F);
    }
    else
    {
        return (u4VDecReadAVCVLD(u4VDecID, RW_AVLD_2ND_CTRL) & 0x3F);
    }
}

// *********************************************************************
// Function : void vInitFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm)
// Description : Get VLD Current Dram pointer
// Parameter : None
// Return    : VLD Sum
// *********************************************************************
void vInitFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm)
{
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
    u4VDecID = 0;
#endif

#if CONFIG_DRV_VIRTUAL_ADDR
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEED_ADDR, (PHYSICAL((UINT32) prH264VDecInitPrm->u4FGSeedbase)) >> 4);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEI_ADDR_A, (PHYSICAL((UINT32) prH264VDecInitPrm->u4CompModelValue)) >> 4);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_DATABASE_ADDR, (PHYSICAL((UINT32) prH264VDecInitPrm->u4FGDatabase)) >> 4);
#else
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEED_ADDR, (u4AbsDramANc((UINT32) prH264VDecInitPrm->u4FGSeedbase)) >> 4);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_SEI_ADDR_A, (u4AbsDramANc((UINT32) prH264VDecInitPrm->u4CompModelValue)) >> 4);
    vVDecWriteAVCFG(u4VDecID, RW_FGT_DATABASE_ADDR, (u4AbsDramANc((UINT32) prH264VDecInitPrm->u4FGDatabase)) >> 4);
#endif
}

// *********************************************************************
// Function : BOOL fgVDecAVCIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
// Description : Check if VLD fetch is done
// Parameter : None
// Return    : TRUE: VLD fetch OK, FALSE: not OK
// *********************************************************************
BOOL fgVDecAVCIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
{
    if ((u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_FETCHOK + (u4BSID << 10)) & VLD_FETCH_OK) == 0)
    {
        return (FALSE);
    }
    return (TRUE);
}
BOOL fgVDecAVCWaitVldFetchOk(UINT32 u4BSID, UINT32 u4VDecID)
{
    UINT32 u4Cnt;
#ifdef VDEC_SIM_DUMP
    printk("wait(`VDEC_INI_FETCH_RDY == 1);\n");
#endif
    if (!fgVDecAVCIsVLDFetchOk(u4BSID, u4VDecID))
    {
        u4Cnt = 0;
        while (!fgVDecAVCIsVLDFetchOk(u4BSID, u4VDecID))
        {
            u4Cnt++;
            if (u4Cnt >= 0x1000)
            {
                return (FALSE);
            }
        }
    }
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#else
    u4Cnt = 0;
    // HW modification
    // read point may not stable after read fetck ok flag
    while (!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10)) & 0x10000))
    {
        u4Cnt++;
        if (u4Cnt >= 0x1000)
        {
            return (FALSE);
        }
    }
#endif
    return (TRUE);
}
// Function : BOOL fgH264VLDInitBarrelShifter1(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
// Description : Init HW Barrel Shifter
// Parameter : u4Ptr: Physical DRAM Start Address to fill Barrel Shifter
// Return    : TRUE: Initial Success, Fail: Initial Fail
// *********************************************************************
BOOL fgH264VLDInitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4FIFOSa, UINT32 u4VLDRdPtr, UINT32 u4VLDWrPtr)
{
    UINT32 u4ByteAddr;
    UINT32 u4TgtByteAddr;
    UINT32 u4BSREMOVE03;
    //UINT32 u4Bits;
    INT32 i;
    BOOL fgFetchOK = FALSE;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4Cnt;
#endif
    // prevent initialize barrel fail
    for (i = 0; i < 5; i++)
    {
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
        u4Cnt = 50000;
        if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1 << 15))
        {
            while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
        }

        if ( bMode_MCORE[u4VDecID] && _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_CORE_0_ID ){
            vVDecAVCSetCoreState(u4VDecID, AVC_CORE_1_ID);
            u4Cnt = 50000;
            if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1 << 15))
            {
                while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
            }
            vVDecAVCSetCoreState(u4VDecID, AVC_CORE_0_ID);
        }
#endif
        vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadAVCCOMVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), u4VLDRdPtr);
        vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), u4VLDRdPtr);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
        vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), u4VLDWrPtr);
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
        vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadAVCCOMVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);
#endif
#else
        vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10), ((u4VLDWrPtr << 4) | 0x2));
#endif

        if (u4BSID == 0)
        {
            vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_ON);
        }

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
        vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_SRST , 1 << 8);
        vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_SRST , 0);
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
        if (bEnable_UFO[u4VDecID]) {
            vVDecWriteAVCUFO(u4VDecID, 36 * 4, u4VDecReadAVCUFO(u4VDecID, 36 * 4) | 0x1);
            vVDecWriteAVCUFO(u4VDecID, 36 * 4, u4VDecReadAVCUFO(u4VDecID, 36 * 4) & ~0x1);
        }
#endif
#endif

#if (CONFIG_DRV_FPGA_BOARD)
        vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_BS_SPEEDUP, 0);
#endif

        // start to fetch data
        vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);
        if (fgVDecAVCWaitVldFetchOk(u4BSID, u4VDecID))
        {
            fgFetchOK = TRUE;

            if ( bMode_MCORE[u4VDecID] && _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_CORE_0_ID ){
                vVDecAVCSetCoreState(u4VDecID, AVC_CORE_1_ID);
                if (!fgVDecAVCWaitVldFetchOk(u4BSID, u4VDecID)) {
                    fgFetchOK = FALSE;
                }
                vVDecAVCSetCoreState(u4VDecID, AVC_CORE_0_ID);
            }

            if (fgFetchOK)
            break;
        }
    }

    if (!fgFetchOK)
    {
        return (FALSE);
    }

    vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);

    if (u4BSID == 0)
    {
        // HW workaround
        // can not reset sum off until barrel shifter finish initialization
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)

#ifdef MEM_PAUSE_SUPPORT
        while (0x01000001 != (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x03000003));
#else
        while (0x00000001 != (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x03000003));
#endif

#else

#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8530)
#else

#ifdef MEM_PAUSE_SUPPORT
        while (5 != (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x3f));
#else
        while (4 != (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x3f));
#endif

#endif

#endif
#endif
        vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_OFF);
    }

    //while (u4VDecReadH264VldRPtr(u4BSID, u4VDecID, &u4Bits, u4FIFOSa) < (u4VLDRdPtr - u4FIFOSa))
    //{
    //    u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 8);
    //}

    // move range 0~15 bytes
    u4TgtByteAddr = u4VLDRdPtr & 0xf;
    u4ByteAddr = u4VLDRdPtr & 0xfffffff0;
    printk("@@@@ 0 >>> u4TgtByteAddr %d, u4ByteAddr %x\n", u4TgtByteAddr, u4ByteAddr);
#if 1
    i = 0;
    while (u4TgtByteAddr)
    {
        u4TgtByteAddr --;
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
        if (((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0] & 0xFFFFFF) == 0x030000) &&
            ((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0] >> 24) <= 0x03)
            && u4TgtByteAddr)
#else
        printk("before --");
        u4BSREMOVE03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R);
        printk("after\n");
        //printk("@@@@ >>> u4TgtByteAddr %x \n",  u4BSREMOVE03);
        //if( ((u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R)&RO_ALVD_FIND_03)) && u4TgtByteAddr)
        if ((u4BSREMOVE03 & RO_ALVD_FIND_03) && u4TgtByteAddr)
#endif
        {
            if ((i > 2) || (u4TgtByteAddr > 1))
            {
                u4TgtByteAddr --;
                i++;
                printk("@@@@ 1 >>> u4TgtByteAddr %d, i %d\n", u4TgtByteAddr, i);
            }
        }
        u4ByteAddr ++;
        i++;
        u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 8);
        printk("@@@@ 2 >>> u4TgtByteAddr %d, i %d\n", u4TgtByteAddr, i);
    }
#else  //for 6589
#if 1
    while (u4TgtByteAddr)
    {
        u4TgtByteAddr --;
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
        if (((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0] & 0xFFFFFF) == 0x030000) &&
            ((((UINT32 *)(VIRTUAL((UINT32) u4ByteAddr)))[0] >> 24) <= 0x03)
            && u4TgtByteAddr)
#else
        u4BSREMOVE03 = u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R);
        //printk("@@@@ >>> u4TgtByteAddr %x \n",  u4BSREMOVE03);
        //if( ((u4VDecReadAVCVLD(u4VDecID, RW_AVLD_RM03R)&RO_ALVD_FIND_03)) && u4TgtByteAddr)
        if ((u4BSREMOVE03 & RO_ALVD_FIND_03) && u4TgtByteAddr)
#endif
        {
            u4TgtByteAddr --;
            //printk("@@@@ 1 >>> u4TgtByteAddr %d\n", u4TgtByteAddr);
        }
    }
    u4ByteAddr ++;
    u4VDecAVCVLDGetBitS(u4BSID, u4VDecID, 8);

    //printk("@@@@ 2 >>> u4TgtByteAddr %d\n", u4TgtByteAddr);
}
#endif
#endif

    printk("{H264 DEC >> } u4InstID = 0x%x, Input Window: 0x%08x\n", u4VDecID, u4VDecReadAVCVLD(u4VDecID, RO_AVLD_BARL));
    return (TRUE);
}


// *********************************************************************
// Function : BOOL fgInitH264BarrelShift1(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
// Description : Reset VLD2
// Parameter : None
// Return    : None
// *********************************************************************
BOOL fgInitH264BarrelShift1(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
{
    vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_RDY_SWTICH, READY_TO_RISC_1);

    //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN | AVC_DEC_CYCLE_EN | AVC_SUM6_APPEND_INV |AVC_NOT_CHK_DATA_VALID);
    //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN | AVC_SUM6_APPEND_INV |AVC_NOT_CHK_DATA_VALID);
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN
                     | AVC_SUM6_APPEND_INV | AVC_NOT_CHK_DATA_VALID | AVC_RBSP_CHK_INV | AVC_READ_FLAG_CHK_INV | AVC_ERR_BYPASS /*| AVC_ERR_CONCEALMENT*/
#if (CONFIG_DRV_VERIFY_SUPPORT)

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#else
                     | AVC_NON_SPEC_SWITCH
#endif
#ifdef NO_COMPARE
                     | AVC_DEC_CYCLE_EN
#endif
                     | AVC_ERR_CONCEALMENT

#endif
                    );
#else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, AVC_EN | AVC_RDY_WITH_CNT | AVC_RDY_CNT_THD | AVLD_MEM_PAUSE_MOD_EN
                     | AVC_SUM6_APPEND_INV | AVC_NOT_CHK_DATA_VALID | AVC_RBSP_CHK_INV | AVC_ERR_BYPASS /*| AVC_ERR_CONCEALMENT*/
#if (CONFIG_DRV_VERIFY_SUPPORT)

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#else
                     | AVC_NON_SPEC_SWITCH
#endif
#ifdef NO_COMPARE
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
                     | AVC_DEC_CYCLE_EN
#endif
#endif
                     | AVC_ERR_CONCEALMENT

#endif
                    );
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteAVCVLDTOP(u4VDecID, RW_VLD_TOP_TIMEOUT_SW, u4VDecReadAVCVLDTOP(u4VDecID, RW_VLD_TOP_TIMEOUT_SW) | VLD_TOP_DEC_CYCLE_EN);
#endif
#endif

    if ( !bMode_MCORE[u4VDecID] || _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID != AVC_LAE_0_ID )
    {
        vVDecWriteAVCMC(u4VDecID, RW_MC_OPBUF, 6);
    }

    if ( bMode_MCORE[u4VDecID] && _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID != AVC_LAE_0_ID ){
        vVDecWriteAVCCOMVLD(u4VDecID, 80* 4, u4VDecReadAVCCOMVLD(u4VDecID, 80* 4) | (0x1 << 20));
        vVDecWriteAVCBS2(u4VDecID, 80 * 4, u4VDecReadAVCBS2(u4VDecID, 80 * 4) | (0x1 << 20));
        vVDecWriteAVCBS2(u4VDecID, RW_VLD_VSTART + (u4BSID << 10), PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoSa) >> 6);
        vVDecWriteAVCBS2(u4VDecID, RW_VLD_VEND + (u4BSID << 10), PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoEa) >> 6);
    }

    vVDecAVCSetVLDVFIFO(0, u4VDecID, PHYSICAL((ULONG)prH264BSInitPrm->u4VFifoSa), PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoEa));
#if CONFIG_DRV_VIRTUAL_ADDR
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteAVCVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, (prH264BSInitPrm->u4PredSa ? PHYSICAL((ULONG) prH264BSInitPrm->u4PredSa) : 0));
#else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PRED_ADDR, (prH264BSInitPrm->u4PredSa ? PHYSICAL((ULONG) prH264BSInitPrm->u4PredSa) : 0));
#endif
#else
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVDecWriteAVCVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, u4AbsDramANc((ULONG) prH264BSInitPrm->u4PredSa));
#else
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_PRED_ADDR, u4AbsDramANc((ULONG) prH264BSInitPrm->u4PredSa));
#endif
#endif

    // Reset AVC VLD Sum
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_ON);
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_RESET_SUM, AVLD_RESET_SUM_OFF);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    // CB test
    //vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC, u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | (VLD_NEW_DRAM_DISABLE));
#endif

    if (!fgH264VLDInitBarrelShifter(0, u4VDecID, PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoSa), PHYSICAL((ULONG) prH264BSInitPrm->u4VLDRdPtr), PHYSICAL((ULONG)prH264BSInitPrm->u4VLDWrPtr)))
    {
        return FALSE;
    }
    return TRUE;
}

// *********************************************************************
// Function : BOOL fgInitH264BarrelShift2(UINT32 u4RDPtrAddr)
// Description : Reset VLD2
// Parameter : None
// Return    : None
// *********************************************************************
BOOL fgInitH264BarrelShift2(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm)
{
    // reset barrel shifter 2
    while (!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL | (1 << 10)) & 0x10000));
    vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_SRST , 1 << 8);
    vVDecWriteAVCCOMVLD(u4VDecID, WO_VLD_SRST , 0);
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_ROCKY)
    if (bEnable_UFO[u4VDecID]) {
        vVDecWriteAVCUFO(u4VDecID, 36 * 4, u4VDecReadAVCUFO(u4VDecID, 36 * 4) | 0x1);
        vVDecWriteAVCUFO(u4VDecID, 36 * 4, u4VDecReadAVCUFO(u4VDecID, 36 * 4) & ~0x1);
    }
#endif
    vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_2ND_RDY_SWTICH, READY_TO_RISC_1);

    // temporarily workaround, will be fixed by ECO
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_CTRL, 1 << 30);
    vVDecWriteAVCVLD(u4VDecID, RW_AVLD_2ND_BARL_CTRL, AVLD_2ND_BARL_EN);

    if ( bMode_MCORE[u4VDecID] && _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID != AVC_LAE_0_ID ){
        vVDecWriteAVCCOMVLD(u4VDecID, 80* 4, u4VDecReadAVCCOMVLD(u4VDecID, 80* 4) | (0x1 << 20));
        vVDecWriteAVCBS2(u4VDecID, 80 * 4, u4VDecReadAVCBS2(u4VDecID, 80 * 4) | (0x1 << 20));
        vVDecWriteAVCBS2(u4VDecID, RW_VLD_VSTART + (u4BSID << 10), PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoSa) >> 6);
        vVDecWriteAVCBS2(u4VDecID, RW_VLD_VEND + (u4BSID << 10), PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoEa) >> 6);
    }

    vVDecAVCSetVLDVFIFO(1, u4VDecID, PHYSICAL(prH264BSInitPrm->u4VFifoSa), PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoEa));

    if (!fgH264VLDInitBarrelShifter(1, u4VDecID, PHYSICAL((ULONG) prH264BSInitPrm->u4VFifoSa), PHYSICAL((ULONG) prH264BSInitPrm->u4VLDRdPtr), PHYSICAL(prH264BSInitPrm->u4VLDWrPtr)))
    {
        return FALSE;
    }
    return TRUE;
}

#if 1
UINT32 u4VDecReadH264VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
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
    //if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))

#ifdef VDEC_SIM_DUMP
    printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
#endif

    if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (PROCESS_FLAG))
    {
        while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
    }

    if ( bMode_MCORE[u4VDecID] && _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecH264DecPrm.u4CoreID == AVC_CORE_0_ID ){
        vVDecAVCSetCoreState(u4VDecID, AVC_CORE_1_ID);
        u4Cnt = 50000;
        if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (PROCESS_FLAG))
        {
            while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
        }
        vVDecAVCSetCoreState(u4VDecID, AVC_CORE_0_ID);
    }

    u4RegVal = u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10));
    vb_sram_ra = u4RegVal & 0x1F;
    vb_sram_wa = (u4RegVal >> 8) & 0x1F;
    seg_rcnt = (u4RegVal >> 24) & 0x3;

    u4SramRptr = vb_sram_ra;
    u4SramWptr = vb_sram_wa;
    u4SramCtr = seg_rcnt;
    u4DramRptr = u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));

    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 32 - (u4SramRptr - u4SramWptr);
    }

    //*pu4Bits = u4VDecReadVLD(u4VDecID, RW_VLD_BITCOUNT + (u4BSID << 10)) & 0x3f;
    *pu4Bits = u4VDecAVCVLDShiftBits(u4BSID, u4VDecID);

#ifdef VDEC_SIM_DUMP
    printk("<vdec> ReadH264VldRPtr, dRptr:0x%08X, sra:0x%08X, swa:0x%08X, scnt:0x%08X, sum:0x%08X\n",
           u4DramRptr, vb_sram_ra, vb_sram_wa, seg_rcnt, *pu4Bits);
#endif

    if (u4VDecReadAVCCOMVLD(u4VDecID, 4 * 59) & (0x1 << 28))
    {
        u4BsBufLen = 7 * 4;
    }
    else // old case
    {
        u4BsBufLen = 6 * 4;
    }

    u4ByteAddr = u4DramRptr - u4SramDataSz * 16 + u4SramCtr * 4 - u4BsBufLen + *pu4Bits / 8;

    *pu4Bits &= 0x7;

    if (u4ByteAddr < u4VFIFOSa)
    {
        u4ByteAddr = u4ByteAddr
                     + ((u4VDecReadAVCCOMVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10)) << 6) - ((UINT32)u4VFIFOSa))
                     - u4VFIFOSa;
    }
    else
    {
        u4ByteAddr -= ((UINT32)u4VFIFOSa);
    }

#ifdef VDEC_SIM_DUMP
    //printk("<vdec> ReadH264VldRPtr, RdPtr=0x%08X (%u) @(%s, %d)\n", u4ByteAddr, u4ByteAddr);
#endif

    return (u4ByteAddr);
}
#else
UINT32 u4VDecReadH264VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa)
{
    UINT32 u4DramRptr;
    UINT32 u4SramRptr, u4SramWptr;
    UINT32 u4SramDataSz;
    UINT32 u4ByteAddr;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4Cnt;
#endif
    // HW issue, wait for read pointer stable
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    u4Cnt = 50000;
    if (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1 << 15))
    {
        while ((!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL) & 0x1)) && (u4Cnt--));
    }
#elif (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    while (!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10)) & 0x1));
#else
    while (!(u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_SRAMCTRL + (u4BSID << 10)) & 0x10000));
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    u4DramRptr = u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
    //  u4SramRptr = (u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0xf) * 4; //count in 128bits
    u4SramRptr = ((u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))) & 0xf) * 4 +
                 (((u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) >> 24)) & 0x3); //count in 128bits
    //u4SramRptrIn32Bits = 4 - ((u4VDecReadVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10))>>24) & 0x3); //count in 32bits
    u4SramWptr = (((u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) >> 8)) & 0xf) * 4;
    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;  // 128bits
    }
    else
    {
        u4SramDataSz = 64 - (u4SramRptr - u4SramWptr);
    }

    //*pu4Bits = u4VDecReadVLD(u4VDecID, RO_VLD_SUM)& 0x3f;
    *pu4Bits = u4VDecAVCVLDShiftBits(u4BSID, u4VDecID);

#ifdef MEM_PAUSE_SUPPORT
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    u4ByteAddr = u4DramRptr - ((u4SramDataSz + 6) * 4) + ((*pu4Bits) / 8);
#else
    u4ByteAddr = u4DramRptr - ((u4SramDataSz + 5) * 4) + ((*pu4Bits) / 8);
#endif
#else
    u4ByteAddr = u4DramRptr - (u4SramDataSz + 4) * 4 + ((*pu4Bits) / 8);
#endif

    if (u4ByteAddr < u4VFIFOSa)
    {
        u4ByteAddr = u4ByteAddr +
                     ((u4VDecReadAVCCOMVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10)) << 6) - ((UINT32)u4VFIFOSa))
                     - u4VFIFOSa;
    }
    else
    {
        u4ByteAddr -= ((UINT32)u4VFIFOSa);
    }
    *pu4Bits &= 0x7;
#else
    u4DramRptr = u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VRPTR + (u4BSID << 10));
    u4SramRptr = (u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) & 0x3f);
    u4SramWptr = ((u4VDecReadAVCCOMVLD(u4VDecID, RO_VLD_VBAR + (u4BSID << 10)) >> 16) & 0x3f);
    if (u4SramWptr > u4SramRptr)
    {
        u4SramDataSz = u4SramWptr - u4SramRptr;
    }
    else
    {
        u4SramDataSz = 64 - (u4SramRptr - u4SramWptr);
    }

    //*pu4Bits = u4VDecReadVLD(u4VDecID, RO_VLD_SUM)& 0x3f;
    *pu4Bits = u4VDecAVCVLDShiftBits(u4BSID, u4VDecID);

#ifdef MEM_PAUSE_SUPPORT
    u4ByteAddr = u4DramRptr - (u4SramDataSz + 5) * 4 + ((*pu4Bits) / 8);
#else
    u4ByteAddr = u4DramRptr - (u4SramDataSz + 4) * 4 + ((*pu4Bits) / 8);
#endif

    if (u4ByteAddr < u4VFIFOSa)
    {
        u4ByteAddr = u4ByteAddr +
                     ((u4VDecReadAVCCOMVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10)) << 6) - ((UINT32)u4VFIFOSa))
                     - u4VFIFOSa;
    }
    else
    {
        u4ByteAddr -= ((UINT32)u4VFIFOSa);
    }
    *pu4Bits &= 0x7;
#endif

    return (u4ByteAddr);

}
#endif

// *********************************************************************
// Function : void vVDecAVCSetVLDVFIFO(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFifoSa, UINT32 u4VFifoEa)
// Description : Set VFIFO start address and end address
// Parameter : u4VDecID : VLD ID
// Return    : None
// *********************************************************************
void vVDecAVCSetVLDVFIFO(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFifoSa, UINT32 u4VFifoEa)
{
    vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_VSTART + (u4BSID << 10), u4VFifoSa >> 6);
    vVDecWriteAVCCOMVLD(u4VDecID, RW_VLD_VEND + (u4BSID << 10), u4VFifoEa >> 6);

}

void vVDecAVCPowerDownHW(UINT32 u4VDecID)
{
    vVDecWriteAVCMISC(u4VDecID, RW_PDN_CTRL, (u4VDecReadAVCMISC(u4VDecID, RW_PDN_CTRL) | 0x003FFFFF));

}

