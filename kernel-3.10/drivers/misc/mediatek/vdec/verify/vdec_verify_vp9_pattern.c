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

#include "vdec_verify_mpv_prov.h"
#include "vdec_verify_file_common.h"
#include "vdec_verify_filesetting.h"
#include "vdec_verify_common.h"
#include "../hal/vdec_hw_common.h"
#include "../hal/vdec_hw_vp9.h"
#include <linux/string.h>

#ifdef CONFIG_TV_DRV_VFY
#include "x_hal_5381.h"
#include "x_hal_926.h"
#endif

//#define x_alloc_aligned_verify_mem(u4Size, u4Align, fgChannelA) VIRTUAL(BSP_AllocAlignedDmaMemory(u4Size,u4Align))
extern void x_free_aligned_verify_mem(void *pvAddr, BOOL fgChannelA);

#define IS_VP9_MC_Y( X ) (( X == 137 ) ? 1:0)
#define IS_VP9_REF_Y( X ) (( X >= 842 && X <= 844) ? 1:0)
#define IS_VP9_MC_C( X ) (( X == 138 ) ? 1:0)
#define IS_VP9_REF_C( X ) (( X >= 845 && X <= 847) ? 1:0)
#define IS_VP9_REF_WIDTH_HEIGHT( X ) (( X >= 851 && X <= 853) ? 1:0)
#define IS_VP9_REF_SCALING_FACT( X ) (( X >= 854 && X <= 859) ? 1:0)
#define IS_VP9_REF_DRAM_PITCH( X ) (( X >= 860 && X <= 862) ? 1:0)
#define IS_VP9_REF_SCALING_STEP( X ) (( X >= 848 && X <= 850) ? 1:0)
#define IS_VP9_SCALING_EN( X ) (( X == 864) ? 1:0)
#define IS_VP9_MAX_BLK_SIZE_RRF( X ) (( X == 863) ? 1:0)
#define IS_VP9_TTL_MI_ROW_COL( X ) (( X == 866) ? 1:0)

#define VP9_I_ONLY 0
#define DECODE_LAE_TIMEOUT 4
#define DECODE_MISSING_DATA 3
#define DECODE_TIMEOUT 1
#define DECODE_MISMATCH 2
#define DECODE_OK 0

#if 0
#define VP9_MV_REG_OFFSET0       0x83000
#define VP9_PP_REG_OFFSET0       0x84000
#define VP9_MISC_REG_OFFSET0     0x85000
#define VP9_VLD_REG_OFFSET0      0x88400
#define VDEC_BS2_OFFSET0        0x86800
#define MC_REG_OFFSET0    0x2000
#define MC_REG_OFFSET1    0x2F000
#define HEVC_MV_REG_OFFSET0       0x4000
#define HEVC_PP_REG_OFFSET0       0x5000
#define VLD_REG_OFFSET0   0x1000
#define VLD_REG_OFFSET1   0x2E000
#define VLD_TOP_REG_OFFSET0   (VLD_REG_OFFSET0 + 0x800)
#define VLD_TOP_REG_OFFSET1   (VLD_REG_OFFSET1 + 0x800)
#define HEVC_MISC_REG_OFFSET0    0x0
#endif


extern void get_random_bytes(void *buf, int nbytes);
extern BOOL fgGoldenCmp(UINT32 u4DecBuf,UINT32 u4GoldenBuf,UINT32 u4Size);

extern UINT32 debug_mode;
extern UINT32 error_rate;
extern char bitstream_name[200];


#if 0
VDEC_INFO_VP9_FB_INFO_T _rVP9_FBInfo[VDEC_INST_MAX];
VDEC_INFO_VP9_STATUS_INFO_T _rVP9_StatusInfo[VDEC_INST_MAX];



VDEC_INFO_VP9_FB_INFO_T* _VP9_GetFBInfo(UINT32 u4InstID)
{
    if(u4InstID >= VDEC_INST_MAX)
    {
        printk("_VP9_GetFBInfo u4InstID(%d) > VDEC_INST_MAX, oops...\n", u4InstID);
        u4InstID = (VDEC_INST_MAX - 1);
    }
    return &(_rVP9_FBInfo[u4InstID]);
}

VDEC_INFO_VP9_STATUS_INFO_T* _VP9_GetStatusInfo(UINT32 u4InstID)
{
    if(u4InstID >= VDEC_INST_MAX)
    {
        printk("_VP9_GetStatusInfo u4InstID(%d) > VDEC_INST_MAX, oops...\n", u4InstID);
        u4InstID = (VDEC_INST_MAX - 1);
    }
    return &(_rVP9_StatusInfo[u4InstID]);
}

void VP9_Check_VFIFO_DATA(char* pData)
{
    char* runner = NULL;
    int i;

    runner = pData;

    for(i = 1; i <= 16*4; i++)
    {
        printf("0x%02x ",*runner++);
        if(i % 16 == 0)
        {
            printf("\n");
        }
    }
}
#endif

#if 0
void VDEC_MC_BUF_CFG(UINT32 u4InstId, UINT32 u4CoreId)
{
    UINT32 i;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    return;
}
#endif

#if 0
// [vpx] confirm whith miles.wu
void VP9_SOFT_RESET(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4Ret;
    UINT32 u4FPGA_Ver = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    u4Ret = 1;

    if( u4CoreId == LAE_ID ) //LAE sw reset
    {
        RISCRead_MCore_TOP(14 , &u4Ret, u4CoreId);
        RISCWrite_MCore_TOP(14, (u4Ret | (1 << 24)) ,u4CoreId);
    }

    RISCRead_VLD_TOP(51, &u4FPGA_Ver, 0);

    printk("Bit-File Version  %d\n", u4FPGA_Ver);

    // ASYNC hw function, repeatedly called for delay
    RISCWrite_VLD(66, 0x101, u4CoreId);
    RISCWrite_VLD(66, 0x101, u4CoreId);
    RISCWrite_VLD(66, 0x101, u4CoreId);
    RISCWrite_VLD(66, 0x101, u4CoreId);
    RISCWrite_VLD(66, 0x101, u4CoreId);

    // [vpx]
    RISCWrite_MISC(33,0x2,u4CoreId); // mid sys_clk selection

    // temporal all truned on
    RISCWrite_MISC(50, 0, u4CoreId);
    RISCWrite_MISC(51, 0, u4CoreId);
    RISCWrite_MISC(94, 0, u4CoreId);

    RISCWrite_VLD(66, 0x0, u4CoreId);

    // enable CRC Check ,1 mc_out, 3: pp_out
    RISCWrite_MISC(1, 0x3, u4CoreId);
    RISCWrite_VP9_VLD(41, 0x1, u4CoreId);

    // COUNT TBL Clear
    RISCWrite_VP9_VLD(106, 1, u4CoreId);

     // polling
    RISCRead_VP9_VLD(106, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0;
    #endif

    // [vpx]
    // need to add count break, avoid infinite loop
    while( ((u4Ret) & 0x1) == 1)
    {
        RISCRead_VP9_VLD(106, &u4Ret, u4CoreId);
    }

    if( u4CoreId == LAE_ID ) //LAE sw reset
    {
        RISCRead_MCore_TOP(14 , &u4Ret, u4CoreId);
        RISCWrite_MCore_TOP(14, (u4Ret & ~(1 << 24)), u4CoreId);
    }

    return;
}

void VP9_INIT_BARREL_SHIFTER(UINT32 u4InstID, UINT32 u4Rptr, UINT32 u4CoreId)
{
    UINT32 i;
    UINT32 u4RetRegValue;
    UINT32 u4VldRptr;
    static UINT32 u4Inited = 0;

    u4VldRptr = PHYSICAL(u4Rptr);

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

#if VP9_RING_VFIFO_SUPPORT
    if(debug_mode > 0)
    {
        printk("%s set fifo[0x%x--0x%x], vldrp:0x%x\n",__FUNCTION__, (PHYSICAL(g2VFIFO[u4InstID]) >> 6), (PHYSICAL(g2VFIFO[u4InstID] + BITSTREAM_BUFF_SIZE) >> 6), u4VldRptr);
    }
    //set fifo start & end
    RISCWrite_VLD( 45, ((PHYSICAL(g2VFIFO[u4InstID])) >> 6), u4CoreId);
    RISCWrite_VLD( 46, ((PHYSICAL(g2VFIFO[u4InstID] + BITSTREAM_BUFF_SIZE)) >> 6), u4CoreId);
    RISCWrite_BS2( 45, ((PHYSICAL(g2VFIFO[u4InstID])) >> 6), u4CoreId);
    RISCWrite_BS2( 46, ((PHYSICAL(g2VFIFO[u4InstID] + BITSTREAM_BUFF_SIZE)) >> 6), u4CoreId);

    if(debug_mode > 0)
    {
        printk("%s set fifo done\n",__FUNCTION__);
    }
#endif
    #if 1
    RISCRead_VLD(59, &u4RetRegValue, u4CoreId);
    RISCWrite_VLD(59, u4RetRegValue | 0x10000000, u4CoreId);
    RISCRead_BS2(59, &u4RetRegValue, u4CoreId);
    RISCWrite_BS2(59, u4RetRegValue | 0x10000000, u4CoreId);
    #endif

    // polling Sram stable
    RISCRead_VLD( 61 , &u4RetRegValue, u4CoreId);

#if SIM_LOG
    //break while
    u4RetRegValue = 0x00;
#endif

    if( (u4RetRegValue >> 15 ) & 0x1 == 1)
    {
       // polling VLD_61  [0] == 1
        RISCRead_VLD( 61 , &u4RetRegValue, u4CoreId );
        while ( (u4RetRegValue & 0x1)   !=  1)
            RISCRead_VLD( 61 , &u4RetRegValue , u4CoreId);
    }

    // read pointer
    RISCWrite_VLD( 44, u4VldRptr, u4CoreId);
#if VP9_RING_VFIFO_SUPPORT
    UINT32 u4VLDWrPtr = (UINT32)g2VFIFO[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2) ? BITSTREAM_BUFF_SIZE : (BITSTREAM_BUFF_SIZE>>1));
    RISCWrite_VLD( 68, (PHYSICAL)(u4VLDWrPtr), u4CoreId);
#endif

    //BITstream DMA async_FIFO  local reset
    RISCRead_VLD( 66 , &u4RetRegValue, u4CoreId );
    // for delay
    RISCWrite_VLD( 66, u4RetRegValue |(1 << 8), u4CoreId );
    RISCWrite_VLD( 66, u4RetRegValue |(1 << 8), u4CoreId );
    RISCWrite_VLD( 66, u4RetRegValue |(1 << 8), u4CoreId );
    RISCWrite_VLD( 66, u4RetRegValue |(1 << 8), u4CoreId );
    RISCWrite_VLD( 66, u4RetRegValue |(1 << 8), u4CoreId );
    RISCWrite_VLD( 66, 0, u4CoreId);

    //initial fetch
    //RISCRead_VLD( 35 , &u4RetRegValue, u4CoreId );
    //RISCWrite_VLD( 35, u4RetRegValue |(1 << 20), u4CoreId );
    RISCWrite_VLD( 35, 0x100000, u4CoreId );

    if (debug_mode>0)
    {
        printk("        wait(`VDEC_INI_FETCH_READY == 1);\n");
    }

    RISCRead_VLD( 58 , &u4RetRegValue, u4CoreId);

     #if SIM_LOG
    //break while
    u4RetRegValue = 0x01;
    #endif

    while ( (u4RetRegValue & 0x1) !=  1)
    {
        RISCRead_VLD( 58, &u4RetRegValue, u4CoreId);
    }

    //initial barrel shifter
    //RISCRead_VLD( 35 , &u4RetRegValue, u4CoreId );
    //RISCWrite_VLD( 35, u4RetRegValue|(1 << 23), u4CoreId );
    RISCWrite_VLD( 35, 0x800000, u4CoreId );

#if 0
    //byte address
    //HEVC_SHIFT_BITS( (r_ptr&0xF) * 8 );
    #if SIM_LOG
    //
    #else
    INIT_SEARCH_START_CODE( (u4VldRptr&0xF) * 8 );
    #endif
#endif

    return;
}

void VP9_VLD_SHIFT_BIT(UINT32 u4InstID, UINT32 u4Offset, UINT32 u4CoreId)
{
    UINT32 i;
    UINT32 u4Ret;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    for ( i = 0; i < u4Offset/32; i++ )
    {
       RISCRead_VP9_VLD( 32 , &u4Ret, u4CoreId);
    }

    if( u4Offset%32 != 0 )
    {
        RISCRead_VP9_VLD( u4Offset%32 , &u4Ret, u4CoreId);
    }
    return;
}

void VP9_INIT_BOOL(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 i;
    UINT32 u4Ret;

    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }
    RISCWrite_VP9_VLD(68, 1, u4CoreId);
    RISCRead_VP9_VLD(68, &u4Ret, u4CoreId);

     #if SIM_LOG
    //break while
    u4Ret = 0x1 << 16;
    #endif

    // [vpx]
    // check bit 16
    // need to add count break, avoid infinite loop
    while (((u4Ret >> 16) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(68, &u4Ret, u4CoreId);
    }

    return;
}

void VP9_READ_LITERAL(UINT32 u4InstID, UINT32 u4ShiftBit,UINT32 u4Gold ,UINT32 u4CoreId)
{
    UINT32 i;
    UINT32 u4Ret;

    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCRead_VP9_VLD(32 + u4ShiftBit, &u4Ret, u4CoreId);

    // SJ need golden check to confirm HW behavior
    // Can ignore after stable.
    #if SIM_LOG
    // do nothing
    #else
    if(u4Ret != u4Gold)
    {
        printk("[VP9] %s %d, Error, Gold = %d\n", __FUNCTION__, u4Ret, u4Gold);
    }
    #endif

    return;
}

void VP9_UPDATE_TX_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 0;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 0
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }

    return;
}

void VP9_UPDATE_COEF_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 1;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 1
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }

    return;
}

void VP9_UPDATE_MBSKIP_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 2;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 2
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_INTER_MODE_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 3;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 3
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_SWITCHABLE_INTERP_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 4;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 4
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_INTRA_INTER_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 5;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 5
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_SINGLE_REF_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 7;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

     #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 7
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_Y_MODE_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 9;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 9
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_PARTITION_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
   UINT32 u4ChkBit;
   UINT32 u4Ret;
   u4ChkBit = 10;
   u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 10
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_MVD_INT_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 11;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 11
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_MVD_FP_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 12;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 12
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_MVD_HP_PROBS(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 13;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 13
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_COMP_REF_PROBS(u4InstID, u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 8;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 13
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

void VP9_UPDATE_COMP_INTER_PROBS(u4InstID, u4CoreId)
{
    UINT32 u4ChkBit;
    UINT32 u4Ret;
    u4ChkBit = 6;
    u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCWrite_VP9_VLD(50, (1 << u4ChkBit), u4CoreId);
    RISCWrite_VP9_VLD(50, 0, u4CoreId);

    // polling
    RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);

    #if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
    #endif

    // [vpx]
    // check bit 13
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(51, &u4Ret, u4CoreId);
    }
    return;
}

UINT32 VP9_IS_KEY_FRAME(UINT32 u4CoreId)
{
    UINT32 u4Ret  = 0;
    if(debug_mode > 0)
    {
        //printk("%s\n",__FUNCTION__);
    }

    RISCRead_VP9_VLD(42, &u4Ret, u4CoreId);

    if((u4Ret & 0x1) == 1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void VDEC_MCORE_INIT(UINT32 u4InstID, UINT32 u4CoreId)
{
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;
    prStatus = _VP9_GetStatusInfo(u4InstID
        );
    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    prStatus->u4DualCore = TRUE;

    printk("Init Dual Core!\n",__FUNCTION__);
    RISCWrite_MCore_TOP(7, 0x1, u4CoreId);
    RISCWrite_MCore_TOP(7, 0x0, u4CoreId);

    // set addr
    RISCWrite_MCore_TOP(8, PHYSICAL(g2LAEBuffer[u4InstID][0]), u4CoreId);
    RISCWrite_MCore_TOP(9, PHYSICAL(g2ErrBuffer[u4InstID][0]), u4CoreId);

    RISCWrite_MCore_TOP(16, PHYSICAL(g2LAEBuffer[u4InstID][0]), u4CoreId);
    RISCWrite_MCore_TOP(17, PHYSICAL(g2ErrBuffer[u4InstID][0]), u4CoreId);


    RISCWrite_MCore_TOP(18, 0x120, u4CoreId); // 288, unit is 16 bytes
    RISCWrite_MCore_TOP(24, 0x1, u4CoreId);
    RISCWrite_MCore_TOP(26, 0x22, u4CoreId);

    return;
}

int VP9_Wait_Decode_Done(unsigned long  start_time, UINT32 u4CoreID)
{
    UINT32 u4Ret = 0;
    UINT32 i = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCRead_MISC( 41 , &u4Ret, u4CoreID);

    #if SIM_LOG
    #else
    while ( ((u4Ret>>16) & 0x1) !=  1)
    {
        RISCRead_MISC(41, &u4Ret, u4CoreID);

        if ( ( jiffies - start_time > 3700) )
        {
            printk("Polling int time out!!!\n");
            return 1;
        }
    }
    #endif

    return 0;
}

int VP9_Wait_MCore_Decode_Done(unsigned long  start_time)
{
    UINT32 u4Ret = 0;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    RISCRead_MCore_TOP( 3 , &u4Ret, 0);

    #if SIM_LOG
    #else
    while ( ((u4Ret>>16) & 0x1) !=  1)
    {
        RISCRead_MCore_TOP( 3, &u4Ret, 0);

        if ( ( jiffies - start_time > 3700) )
        {
            printk("Polling mcore int time out!!!\n");
            return 1;
        }
    }
    #endif

    return 0;
}

UINT32 VDEC_TRIG(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4Ret  = 0;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prStatus = _VP9_GetStatusInfo(u4InstID);

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }
    RISCRead_VP9_VLD(42, &u4Ret, u4CoreId);

#if VP9_I_ONLY
    // p frame skip
    if((u4Ret & 0x1) == 1)
    {
        printk("Decode I Only!! Skip This Inter Picture\n");
        return 1; // skip frame
    }
    else
    {
        RISCWrite_VP9_VLD(46, 0x1, u4CoreId);
    }
#else
    if((u4Ret & 0x1) == 1)
    {
        printk("VDEC Trigger Decode P Picture\n");
    }
    else
    {
        printk("VDEC Trigger Decode I Picture\n");
    }

    if(prStatus->u4DualCore)
    {
        RISCWrite_VLD_TOP(73, 0x0B2003FC, 1);
        RISCWrite_VLD_TOP(73, 0x07200190, 8);
        RISCWrite_VP9_VLD(46, 0x1, u4CoreId);
        prStatus->u4DecodeTimeOut = VP9_Wait_Decode_Done(jiffies, u4CoreId);

        if(!prStatus->u4DecodeTimeOut)
        {
            // disable core0/core1 error detector
            RISCWrite_VP9_VLD(75, 0, 0);
            RISCWrite_VP9_VLD(75, 0, 1);

            // Trigger MCore Decode
            RISCWrite_MCore_TOP(25, 1, 0);
        }
        else // LAE Decode Error
        {
            // Error Handle here
            return 2; //means LAE error
        }
    }
    else
    {
        RISCWrite_VP9_VLD(46, 0x1, u4CoreId);
     }
#endif
    return 0;   // Trigger Decode
}


void VP9_FB_Config(UINT32 u4InstID)
{
    UINT32 i;
    UINT32 u4SBWidth, u4SBHeight;
    UINT32 PIC_SIZE_Y,PIC_SIZE_C, Y_OFFSET;
    UINT32 UFO_LEN_SIZE_Y, UFO_LEN_SIZE_C;
    UINT32 u4MV_BUF_SIZE;
    UINT32 u4DramPicY_Y_LENSize;
    UINT32 u4DramPicC_C_LENSize;
    VDEC_INFO_VP9_FB_INFO_T* prFBInfo;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prFBInfo = _VP9_GetFBInfo(u4InstID);
    prStatus = _VP9_GetStatusInfo(u4InstID);

    u4SBWidth = ((prFBInfo->u4Width + 63) >> 6); // # of SB
    u4SBHeight = ((prFBInfo->u4Height + 63) >> 6); // # of SB

    u4MV_BUF_SIZE = u4SBWidth * u4SBHeight * 36 * 16;

    PIC_SIZE_Y = (u4SBWidth * u4SBHeight) << (6 + 6);
    PIC_SIZE_C = PIC_SIZE_Y >> 1;
    Y_OFFSET = (MAX_VP9_BUF * PIC_SIZE_Y);

    if(prStatus->u4UFOMode)
    {
        UFO_LEN_SIZE_Y = ((((PIC_SIZE_Y + 255)>> 8)+ 63 + (16*8)) >> 6 ) << 6;
        UFO_LEN_SIZE_C = (((UFO_LEN_SIZE_Y >> 1) + 15 + (16*8)) >> 4) << 4;
        u4DramPicY_Y_LENSize  =  (((PIC_SIZE_Y + UFO_LEN_SIZE_Y) + 8191) >> 13) << 13;
        u4DramPicC_C_LENSize  = (((PIC_SIZE_C + UFO_LEN_SIZE_C ) + 8191) >> 13) << 13;

        prFBInfo->u4UFO_LEN_SIZE_Y = UFO_LEN_SIZE_Y;
        prFBInfo->u4UFO_LEN_SIZE_C = UFO_LEN_SIZE_C;
        prFBInfo->PIC_SIZE_Y = PIC_SIZE_Y;
        prFBInfo->PIC_SIZE_C = PIC_SIZE_C;
        prFBInfo->u4DramPicY_Y_LENSize = u4DramPicY_Y_LENSize;
        prFBInfo->u4DramPicC_C_LENSize = u4DramPicC_C_LENSize;

        Y_OFFSET = (MAX_VP9_BUF * u4DramPicY_Y_LENSize);
    }

    printk("=========== VP9 Frame Buffer Configure ===========\n");

    printk("PIC_SIZE_Y %d(0x%08X), PIC_SIZE_C %d(0x%08X)\n",PIC_SIZE_Y,PIC_SIZE_Y,PIC_SIZE_C,PIC_SIZE_C );
    printk("Y_OFFSET %d(0x%08X)\n", Y_OFFSET, Y_OFFSET);

    for ( i = 0; i < MAX_VP9_BUF; i++ )
    {
        // UFO Mode reserved
        if(prStatus->u4UFOMode)
        {
            prFBInfo->VP9_FBM_YAddr[i] = g2DPB[u4InstID] + (i * u4DramPicY_Y_LENSize);
            //_VP9_FBM_YLEN_Addr[i] = g2DPB[u4InstId] + ((i+1) * PIC_SIZE_Y);
            prFBInfo->VP9_FBM_YLEN_Addr[i] = prFBInfo->VP9_FBM_YAddr[i] + PIC_SIZE_Y;

            prFBInfo->VP9_FBM_CAddr[i] = g2DPB[u4InstID] + Y_OFFSET + (i * u4DramPicC_C_LENSize);
            //_VP9_FBM_CLEN_Addr[i] = g2DPB[u4InstId] + Y_OFFSET + ((i+1) * PIC_SIZE_C);
            prFBInfo->VP9_FBM_CLEN_Addr[i] = prFBInfo->VP9_FBM_CAddr[i] + PIC_SIZE_C;

            UTIL_Printf("_VP9_FBM_YAddr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_FBM_YAddr[i], PHYSICAL(prFBInfo->VP9_FBM_YAddr[i]));
            UTIL_Printf("_VP9_FBM_Y_LEN_Addr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_FBM_YLEN_Addr[i], PHYSICAL(prFBInfo->VP9_FBM_YLEN_Addr[i]));

            UTIL_Printf("_VP9_FBM_CAddr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_FBM_CAddr[i], PHYSICAL(prFBInfo->VP9_FBM_CAddr[i]));
            UTIL_Printf("_VP9_FBM_C_LEN_Addr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_FBM_CLEN_Addr[i], PHYSICAL(prFBInfo->VP9_FBM_CLEN_Addr[i]));

            // DRAM Foot Print for UFO Mode Currently
            // ----------------------
            // |        Y           |   Y[0] (8K Alignment for 1 Y_BS)
            // ----------------------
            // |      Y_LEN         |
            // ----------------------
            // |        Y           |   Y[1]
            // ----------------------
            // |      Y_LEN         |
            // ----------------------
            //          .
            //          .
            //
            // ----------------------
            // |        Y           |    Y[8]
            // ----------------------
            // |      Y_LEN         |
            // ----------------------
            // |        C           |    C[0]
            // ----------------------
            // |      Y_LEN         |
            // ----------------------
            // |        C           |    C[1]
            // ----------------------
            // |      C_LEN         |
            // ----------------------
            //          .
            //          .
            // ----------------------
            // |        C           |    C[8]
            // ----------------------
            // |      C_LEN         |
            // ----------------------
            // |      MV_Buffer     |    MV[0]
            // ----------------------
            // |    TILE_Buffer     |    TILE
            // ----------------------

        }
        else
        {
            prFBInfo->VP9_FBM_YAddr[i] = g2DPB[u4InstID] + (i * PIC_SIZE_Y);
            prFBInfo->VP9_FBM_CAddr[i] = g2DPB[u4InstID] + Y_OFFSET + (i * PIC_SIZE_C);

            UTIL_Printf("_VP9_FBM_YAddr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_FBM_YAddr[i], PHYSICAL(prFBInfo->VP9_FBM_YAddr[i]));
            UTIL_Printf("_VP9_FBM_CAddr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_FBM_CAddr[i], PHYSICAL(prFBInfo->VP9_FBM_CAddr[i]));

            #if 0
            if((Dpb_addr[i] + PIC_SIZE) >= (g2DPB[u4InstId] + PATTERN_VP9_FBM_SZ))
            {
                UTIL_Printf("Error ++++++++++++++++++++++++++++++++++\n");
                UTIL_Printf("==========DPB Size Not Enough========\n");
                UTIL_Printf("Error ++++++++++++++++++++++++++++++++++\n");
            }
            #endif
        }
    }

    // append to last c frame as mv buffer
    for( i = 0; i < MAX_VP9_MV_BUF; i++)
    {
        if(prStatus->u4UFOMode)
        {
            prFBInfo->VP9_MV_Addr[i] = (prFBInfo->VP9_FBM_CAddr[MAX_VP9_BUF-1] + u4DramPicC_C_LENSize) + (i*u4MV_BUF_SIZE);
            UTIL_Printf("_VP9_MV_Addr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_MV_Addr[i], PHYSICAL(prFBInfo->VP9_MV_Addr[i]));
        }
        else
        {
            prFBInfo->VP9_MV_Addr[i] = (prFBInfo->VP9_FBM_CAddr[MAX_VP9_BUF-1] + PIC_SIZE_C) + (i*u4MV_BUF_SIZE);
            UTIL_Printf("_VP9_MV_Addr[%d]:0x%08X (0x%08X) \n", i, prFBInfo->VP9_MV_Addr[i], PHYSICAL(prFBInfo->VP9_MV_Addr[i]));
        }
    }

    // tile maxmium # => 4096 >> 6 (SB size) << 2 (MAX Tile Row size)
    // however, hw only need 1 4k physically continous address for tile setting is ok..
    prFBInfo->VP9_TILE_Addr  = prFBInfo->VP9_MV_Addr[MAX_VP9_MV_BUF - 1] + u4MV_BUF_SIZE;
    UTIL_Printf("VP9_TILE_Addr:0x%08X (0x%08X) \n", prFBInfo->VP9_TILE_Addr, PHYSICAL(prFBInfo->VP9_TILE_Addr));

    prFBInfo->VP9_COUNT_TBL_Addr = prFBInfo->VP9_TILE_Addr + VP9_TILE_BUFFER_SIZE;
    UTIL_Printf("VP9_COUNT_TBL_Addr:0x%08X (0x%08X) \n", prFBInfo->VP9_COUNT_TBL_Addr, PHYSICAL(prFBInfo->VP9_COUNT_TBL_Addr));

    printk("=================== End ===================\n");
}

int VP9DumpMem( UINT32 u4InstID, unsigned char* buf, unsigned int size , int  frame_num , unsigned int type , bool isTimeout)
{

    UCHAR fpDumpFile[100] = "d:\\ChkFolder\\vDecY_";
    UCHAR fpDumpFileC[100] = "d:\\ChkFolder\\vDecC_";
    UCHAR fpDumpFileLAE[100] = "d:\\ChkFolder\\vDecLAE_";
    UCHAR fpDumpFileERR[100] = "d:\\ChkFolder\\vDecERR_";
    UCHAR *fpDump;
    UINT32 u4ReadSize;
    UINT32 u4Temp;
    FILE *pFile = NULL;
    UCHAR  ucCaseName[200] = {0};
    UCHAR  ucTmpStr[200] = {0};
    UCHAR  *ucCurrent = NULL;
    UCHAR* const delim = "\\";
    UCHAR* ucToken;
    UINT32 i = 0;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prStatus = _VP9_GetStatusInfo(u4InstID);

    printk("Dump mem %s\n","test");

    strcpy(ucTmpStr, bitstream_name);
    ucCurrent = ucTmpStr;
    while (ucToken = strsep(&ucCurrent, delim))
    {
        if(strlen(ucToken) > 1)
        {
            strcpy(ucCaseName, ucToken);
        }
    }

    if(type == 1)
    {
        fpDump = fpDumpFile;
    }
    else if(type == 2)
    {
        fpDump = fpDumpFileC;
    }
    else if(type == 3)
    {
        fpDump = fpDumpFileLAE;
    }
    else
    {
        fpDump = fpDumpFileERR;
    }
    u4Temp = strlen(fpDump);
    u4Temp += sprintf(fpDump + u4Temp,"stream_%s_frame_", ucCaseName);
    u4Temp += sprintf(fpDump + u4Temp,"%d",frame_num);
    u4Temp += sprintf(fpDump + u4Temp,"%s",".dram");

#ifdef SATA_HDD_READ_SUPPORT
    BOOL fgResult;
    fgResult = fgWrData2PC(buf, size, 7, fpDump);
    if(!fgResult)
        UTIL_Printf("[Write File Error!!]\n");
//      fgOverWrData2PC(buf, size, u4Mode, fpDump);
#else
    pFile = fopen(fpDump,"wb");

    if(pFile == NULL)
    {
        UTIL_Printf("Create file error !\n");
    }
    u4ReadSize = fwrite ((char* )(buf), 1, size, pFile);
    UTIL_Printf("read file len = %d @ 0x%x\n",u4ReadSize,(UINT32)buf);
    fclose(pFile);
#endif
    return 0;
}

int VP9DumpTileMem( UINT32 u4InstID, unsigned char* buf, unsigned int size , int  frame_num)
{
    UCHAR fpDumpFile[100] = "d:\\ChkFolder\\vDecTile_";
    UCHAR *fpDump;
    UINT32 u4ReadSize;
    UINT32 u4Temp;
    FILE *pFile = NULL;
    UCHAR  ucCaseName[200] = {0};
    UCHAR  ucTmpStr[200] = {0};
    UCHAR  *ucCurrent = NULL;
    UCHAR* const delim = "\\";
    UCHAR* ucToken;
    UINT32 i = 0;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prStatus = _VP9_GetStatusInfo(u4InstID);

    printk("Dump mem %s\n","test");

    strcpy(ucTmpStr, bitstream_name);
    ucCurrent = ucTmpStr;
    while (ucToken = strsep(&ucCurrent, delim))
    {
        if(strlen(ucToken) > 1)
        {
            strcpy(ucCaseName, ucToken);
        }
    }

    fpDump = fpDumpFile;

    u4Temp = strlen(fpDump);
    u4Temp += sprintf(fpDump + u4Temp,"stream_%s_frame_", ucCaseName);
    u4Temp += sprintf(fpDump + u4Temp,"%d",frame_num);
    u4Temp += sprintf(fpDump + u4Temp,"%s",".dram");

#ifdef SATA_HDD_READ_SUPPORT
    BOOL fgResult;
    fgResult = fgWrData2PC(buf, size, 7, fpDump);
    if(!fgResult)
        UTIL_Printf("[Write File Error!!]\n");
//      fgOverWrData2PC(buf, size, u4Mode, fpDump);
#else
    pFile = fopen(fpDump,"wb");

    if(pFile == NULL)
    {
        UTIL_Printf("Create file error !\n");
    }
    u4ReadSize = fwrite ((char* )(buf), 1, size, pFile);
    UTIL_Printf("read file len = %d @ 0x%x\n",u4ReadSize,(UINT32)buf);
    fclose(pFile);
#endif
    return 0;
}

int VP9DumpCountTblMem( UINT32 u4InstID, unsigned char* buf, unsigned int size , int  frame_num)
{
    UCHAR fpDumpFile[100] = "d:\\ChkFolder\\vDecCountTbl_";
    UCHAR *fpDump;
    UINT32 u4ReadSize;
    UINT32 u4Temp;
    FILE *pFile = NULL;
    UCHAR  ucCaseName[200] = {0};
    UCHAR  ucTmpStr[200] = {0};
    UCHAR  *ucCurrent = NULL;
    UCHAR* const delim = "\\";
    UCHAR* ucToken;
    UINT32 i = 0;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prStatus = _VP9_GetStatusInfo(u4InstID);

    printk("Dump mem %s\n","test");

    strcpy(ucTmpStr, bitstream_name);
    ucCurrent = ucTmpStr;
    while (ucToken = strsep(&ucCurrent, delim))
    {
        if(strlen(ucToken) > 1)
        {
            strcpy(ucCaseName, ucToken);
        }
    }

    fpDump = fpDumpFile;

    u4Temp = strlen(fpDump);
    u4Temp += sprintf(fpDump + u4Temp,"stream_%s_frame_", ucCaseName);
    u4Temp += sprintf(fpDump + u4Temp,"%d",frame_num);
    u4Temp += sprintf(fpDump + u4Temp,"%s",".dram");

#ifdef SATA_HDD_READ_SUPPORT
    BOOL fgResult;
    fgResult = fgWrData2PC(buf, size, 7, fpDump);
    if(!fgResult)
        UTIL_Printf("[Write File Error!!]\n");
//      fgOverWrData2PC(buf, size, u4Mode, fpDump);
#else
    pFile = fopen(fpDump,"wb");

    if(pFile == NULL)
    {
        UTIL_Printf("Create file error !\n");
    }
    u4ReadSize = fwrite ((char* )(buf), 1, size, pFile);
    UTIL_Printf("read file len = %d @ 0x%x\n",u4ReadSize,(UINT32)buf);
    fclose(pFile);
#endif
    return 0;
}

int  VP9GoldenComparison( UINT32 u4InstID, UINT32  u4FrameNum, UINT32 u4PIC_SIZE_Y, UINT32 u4PP_OUT_Y, UINT32 u4PP_OUT_C , bool isDump,
                          UINT32 u4DPB_UFO_Y_Len_Addr, UINT32 u4DPB_UFO_C_Len_Addr, UINT32 u4UFO_LEN_SIZE_Y, UINT32 u4UFO_LEN_SIZE_C )
{
    //golden result comparison
    unsigned char *ucYGoldenBuf;
    unsigned char *ucCbCrGoldenBuf;
    char *ptr_base = NULL;
    char file_name[200] = {0};
    struct file *fd;
    int file_num, file_len;
    UINT32 u4RetY, u4RetC, u4Ret;
    BOOL fgOpen;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prStatus = _VP9_GetStatusInfo(u4InstID);

    u4Ret = 0;
    u4RetY = 0;
    u4RetC = 0;
    file_len = 0;
    file_num = 0;


    printk("// [INFO] GoldenComparison PIC_SIZE_Y: 0x%08X, PP_OUT_Y_ADDR: 0x%08X,  PP_OUT_C_ADDR: 0x%08X\n", u4PIC_SIZE_Y, u4PP_OUT_Y, u4PP_OUT_C);

#if 0
    if(frame_num < _u4StartCompPicNum[u4InstID])
    {
        printk("skip current frame @ %d\n",frame_num);
        goto Check_End;
    }
#endif

    // Load golden file
    ucYGoldenBuf = g2YGolden[u4InstID];

    // UFO
    if (prStatus->u4UFOMode)
    {
        sprintf(file_name, "%sufo_pat/ufo_%d_bits_Y.out", bitstream_name, u4FrameNum);
    }
    else
    {
        sprintf(file_name, "%spp_pat/frame_%d_Y.dat", bitstream_name, u4FrameNum);
    }

    //dump Y golden
    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = ucYGoldenBuf;
    _tFBufFileInfo[u4InstID].u4TargetSz = u4PIC_SIZE_Y;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    memset ( ucYGoldenBuf , 0 ,u4PIC_SIZE_Y );
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);
    if (fgOpen == FALSE)
    {
        UTIL_Printf("// Open golden file error : %s\n",file_name);
    }
    else
    {

    }

    ucCbCrGoldenBuf = g2CGolden[u4InstID];

    // UFO
    if (prStatus->u4UFOMode)
    {
        sprintf(file_name, "%sufo_pat/ufo_%d_bits_C.out", bitstream_name, u4FrameNum);
    }
    else
    {
        sprintf(file_name, "%spp_pat/frame_%d_C.dat", bitstream_name, u4FrameNum);
    }

    //dump CbCr golden
    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = ucCbCrGoldenBuf;
    _tFBufFileInfo[u4InstID].u4TargetSz = (u4PIC_SIZE_Y >> 1);
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    memset ( ucCbCrGoldenBuf , 0 ,(u4PIC_SIZE_Y >> 1) );
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);
    if (fgOpen == FALSE)
    {
        UTIL_Printf("// Open golden file error : %s\n",file_name);
    }
    else
    {

    }
    HalFlushInvalidateDCache();

    //////////////Y golden comparison////////////////////

    //vVDec_InvDCacheRange(Ptr_output_Y,PIC_SIZE_Y);

    #if SIM_LOG
    printk("\n// ======== Frame %d Golden Y test: %d ========\n", u4FrameNum, u4RetY );
    u4RetY = 0;
    #else
    u4RetY = fgGoldenCmp(u4PP_OUT_Y, ucYGoldenBuf,u4PIC_SIZE_Y);
    printk("\n// ======== Frame %d Golden Y test: %d ========\n", u4FrameNum, u4RetY );
    #endif

//#ifndef SATA_HDD_READ_SUPPORT
    if (u4RetY != 0 )
    {
        if (isDump)
        {
            VP9DumpMem( u4InstID, u4PP_OUT_Y, u4PIC_SIZE_Y, u4FrameNum , 1, 0);
        }
        //set_fs( oldfs );
        //return 1;
    }
//#endif

    //////////////C golden comparison////////////////////

    //vVDec_InvDCacheRange(Ptr_output_C,(PIC_SIZE_Y >> 1));

    #if SIM_LOG
    printk("\n// ======== Frame %d Golden C test: %d ========\n", u4FrameNum, u4RetC );
    u4RetC = 0;
    #else
    u4RetC = fgGoldenCmp(u4PP_OUT_C, ucCbCrGoldenBuf, (u4PIC_SIZE_Y >> 1));
    printk("\n// ======== Frame %d Golden C test: %d ========\n", u4FrameNum, u4RetC );
    #endif

//#ifndef SATA_HDD_READ_SUPPORT
    if (u4RetC !=0 )
    {
        if (isDump)
        {
            VP9DumpMem( u4InstID, u4PP_OUT_C, u4PIC_SIZE_Y >>1, u4FrameNum, 2, 0 );
        }
        //set_fs( oldfs );
        //return 1;
    }
//#endif

    if((u4RetY != 0) || (u4RetC != 0))
    {
        printk("// Compare mismatch here,please check!\n");
        return 1;
    }

    if (prStatus->u4UFOMode)
    {
        UINT32 cmp_size;

         //////////////Y LEN comparison////////////////////
        sprintf(file_name, "%sufo_pat/ufo_%d_len_Y.out", bitstream_name, u4FrameNum);

        //dump Y golden
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
        _tFBufFileInfo[u4InstID].pucTargetAddr = ucYGoldenBuf; // use Y buf
        _tFBufFileInfo[u4InstID].u4TargetSz = u4UFO_LEN_SIZE_Y;
        _tFBufFileInfo[u4InstID].u4FileLength = 0;
        _tFBufFileInfo[u4InstID].u4FileOffset = 0;
        memset ( ucYGoldenBuf , 0 ,u4UFO_LEN_SIZE_Y );
        fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);
        if (fgOpen == FALSE)
        {
            UTIL_Printf("// Open golden file error : %s\n",file_name);
        }
        else
        {

        }
        HalFlushInvalidateDCache();

        u4Ret = memcmp(ucYGoldenBuf, u4DPB_UFO_Y_Len_Addr, u4UFO_LEN_SIZE_Y);

        if ( isDump || u4Ret ==0 )
        {
            printk("\n// ======== Frame %d UFO Y LEN test: %d ========\n", u4FrameNum, u4Ret );
        }

#ifndef SATA_HDD_READ_SUPPORT
        if (u4Ret !=0 )
        {
            if (isDump)
            {
                VP9DumpMem( u4InstID, u4DPB_UFO_Y_Len_Addr, cmp_size, u4FrameNum , 4, 0);
            }
            //set_fs( oldfs );
            return 1;
        }
#else
        if(u4Ret != 0)
        {
            return u4Ret;
        }
#endif
        //////////////C LEN comparison////////////////////
        sprintf(file_name, "%sufo_pat/ufo_%d_len_C.out", bitstream_name, u4FrameNum);

        //dump Y golden
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
        _tFBufFileInfo[u4InstID].pucTargetAddr = ucYGoldenBuf;
        _tFBufFileInfo[u4InstID].u4TargetSz = u4UFO_LEN_SIZE_C;
        _tFBufFileInfo[u4InstID].u4FileLength = 0;
        _tFBufFileInfo[u4InstID].u4FileOffset = 0;
        memset ( ucYGoldenBuf , 0 ,u4UFO_LEN_SIZE_C);
        fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);
        if (fgOpen == FALSE)
        {
            UTIL_Printf("Open golden file error : %s\n",file_name);
        }
        else
        {

        }

        HalFlushInvalidateDCache();
        u4Ret =  memcmp(ucYGoldenBuf, u4DPB_UFO_C_Len_Addr, u4UFO_LEN_SIZE_C);

        if ( isDump || u4Ret == 0 )
        {
            printk("\n// ======== Frame %d UFO C LEN test: %d ========\n", u4FrameNum, u4Ret );
        }

        #ifndef SATA_HDD_READ_SUPPORT
        if (u4Ret !=0 )
        {
            if (isDump)
            {
                VP9DumpMem( u4InstID, u4DPB_UFO_C_Len_Addr, cmp_size, u4FrameNum, 5, 0 );
            }
            return 1;
        }
        #else
        if(u4Ret != 0)
        {
            return u4Ret;
        }
        #endif

    }

    printk("\n");
    return 0;
}

void VP9_TILE_INFO_LOAD(UINT32 u4InstID, UINT32 u4FrameNum, UINT32 u4CoreID , UINT32 u4BistreamOffset)
{
    char * file_name[200] = {0};
    BOOL fgOpen = FALSE;
    unsigned char *pucTileBuff;
    UINT32 i;
    UINT32 u4RefData;
    unsigned int *puiTileDataBuff;
    VDEC_INFO_VP9_FB_INFO_T* prFBInfo;

    prFBInfo = _VP9_GetFBInfo(u4InstID);

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    sprintf(file_name, "%svld_pat/tile_info_%d.raw", bitstream_name,  u4FrameNum);
    pucTileBuff = g2TileBuffer[u4InstID];
    puiTileDataBuff = prFBInfo->VP9_TILE_Addr;

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = pucTileBuff;
    _tFBufFileInfo[u4InstID].u4TargetSz = VP9_TILE_BUFFER_SIZE;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

    if (fgOpen == FALSE)
    {
       UTIL_Printf("[Error] Open Tile raw %s  Fail @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
    }

    HalFlushInvalidateDCache();
    for(i = 0; i < _tFBufFileInfo[u4InstID].u4RealGetBytes; i += 4)
    {
        u4RefData = ((pucTileBuff[i + 3] << 8*3) + (pucTileBuff[i + 2] << 8*2) + (pucTileBuff[i + 1] << 8*1) + (pucTileBuff[i]));

        if( i % 16 == 4)
        {
            while(u4RefData > BITSTREAM_BUFF_SIZE)
            {
                u4RefData -= BITSTREAM_BUFF_SIZE;
            }
            u4RefData = u4RefData + u4BistreamOffset;
        }

        *puiTileDataBuff = u4RefData;
        puiTileDataBuff++;
    }

    RISCWrite_VLD_TOP(46, PHYSICAL(prFBInfo->VP9_TILE_Addr), u4CoreID);

    // for debug dump
    #if 0
    VP9DumpTileMem(u4InstID, prFBInfo->VP9_TILE_Addr, 64 * 16 * 4, u4FrameNum);
    #endif
}

void VP9_COMPARE_COUNT_TBL(UINT32 u4InstID, UINT32 u4FrameNum, UINT32 u4CoreID)
{
    UINT32 i;
    UINT32 u4Ret = 0;
    UINT32 u4ChkBit;
    char * file_name[200] = {0};
    unsigned char *pucCountTblBuff;
    BOOL fgOpen = FALSE;
    BOOL fgCountTbl = FALSE;
    VDEC_INFO_VP9_FB_INFO_T* prFBInfo;

    prFBInfo = _VP9_GetFBInfo(u4InstID);

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    memset(prFBInfo->VP9_COUNT_TBL_Addr, 0, 256 * 16 * 4);

    sprintf(file_name, "%svld_pat/gold_count_tbl_%d.dat", bitstream_name,  u4FrameNum);
    pucCountTblBuff = g2CountTblBuffer[u4InstID];

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = pucCountTblBuff;
    _tFBufFileInfo[u4InstID].u4TargetSz = VP9_COUNT_TBL_SZ;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

    if (fgOpen == FALSE)
    {
       UTIL_Printf("[Error] Open Count TBL raw %s  Fail @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
    }
    HalFlushInvalidateDCache();

    // set vp9_ctx_count_wdma_mode
    RISCWrite_VP9_VLD(119, 1, u4CoreID);
    RISCWrite_VP9_VLD(120, PHYSICAL(prFBInfo->VP9_COUNT_TBL_Addr), u4CoreID);

    RISCWrite_VP9_VLD(122, (1 << 0), u4CoreID);

    u4ChkBit = 0;

    // polling
    RISCRead_VP9_VLD(123, &u4Ret, u4CoreID);

#if SIM_LOG
    //break while
    u4Ret = 0x1 << u4ChkBit;
#endif

    // [vpx]
    // check bit 12
    // need to add count break, avoid infinite loop
    while( ((u4Ret >> u4ChkBit) & 0x1) != 1)
    {
        RISCRead_VP9_VLD(123, &u4Ret, u4CoreID);
    }

    fgCountTbl = fgGoldenCmp(prFBInfo->VP9_COUNT_TBL_Addr, pucCountTblBuff, 256 * 16 * 4);
    printk("\n// ======== Frame %d Count TBL test: %d ========\n", u4FrameNum, fgCountTbl);

    if (fgCountTbl == 1)
    {
        VP9DumpCountTblMem( u4InstID, prFBInfo->VP9_COUNT_TBL_Addr, 256 * 16 * 4, u4FrameNum);
    }
    return;
}

// confirm with HsiuYi
void VP9_UFO_CONFIG(UINT32 u4InstID, UINT32 u4CoreId)
{
    UINT32 u4Pic_w_h;
    UINT32 u4Width, u4Height;
    VDEC_INFO_VP9_FB_INFO_T* prFBInfo;

    prFBInfo = _VP9_GetFBInfo(u4InstID);

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }

    u4Width = ((prFBInfo->u4Width + 63) >> 6) << 6;
    u4Height = ((prFBInfo->u4Height + 63) >> 6) << 6;

    u4Pic_w_h = ((((u4Width) / 16) - 1) << 16) | (((u4Height) / 16) - 1);

    RISCWrite_MC(700, u4Pic_w_h, u4CoreId);

    // enable UFO mode
    RISCWrite_MC(664, 0x11, u4CoreId);

    RISCWrite_MC(698, PHYSICAL(prFBInfo->VP9_FBM_YLEN_Addr[prFBInfo->u4CurrentFBId]), u4CoreId);
    RISCWrite_MC(699, PHYSICAL(prFBInfo->VP9_FBM_CLEN_Addr[prFBInfo->u4CurrentFBId]), u4CoreId);

    RISCWrite_MC(825, prFBInfo->PIC_SIZE_Y, u4CoreId);
    RISCWrite_MC(826, prFBInfo->PIC_SIZE_C, u4CoreId);

    // YC sep, ChunChia comment this is default on
    RISCWrite_MC(727,  0x1, u4CoreId);

    // set reference length map offset
    #if 0
    RISCWrite_MC(663, ,u4CoreId);
    RISCWrite_MC(701, ,u4CoreId);
    RISCWrite_MC(343, ,u4CoreId);
    #endif
}

void VP9_RRF_Calculate_Dram_Size(UINT32 u4InstID, UINT32 *u4PIC_Y_SIZE, UINT32 *u4UFO_Y_SIZE, UINT32 *u4UFO_C_SIZE, UINT32 u4CoreId)
{
    UINT32 u4RegVal;
    UINT32 u4SBWidth_M1, u4SBHeight_M1;

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }
    //SB_HEIGHT:  VLD_TOP_26[27:16]
    //SB_WIDTH:   VLD_TOP_26[11:0]

    RISCRead_VLD_TOP(26, &u4RegVal, u4CoreId);

    u4SBWidth_M1 = u4RegVal & 0xFFF;
    u4SBHeight_M1 = (u4RegVal >> 16) & 0xFFF;

    printk(" RRF Debug u4RegVal = 0x%x, u4SBWidth = 0x%x, u4SBHeight = 0x%x\n", u4RegVal, u4SBWidth_M1 + 1, u4SBHeight_M1 + 1);
    *u4PIC_Y_SIZE = ((u4SBWidth_M1 + 1)  * (u4SBHeight_M1 + 1)) << (6 + 6);
    *u4UFO_Y_SIZE = ((((*u4PIC_Y_SIZE + 255)>> 8)+ 63 + (16*8)) >> 6 ) << 6;
    *u4UFO_C_SIZE = (((*u4UFO_Y_SIZE  >> 1) + 15 + (16*8)) >> 4) << 4;

    return;
}

int VP9_CRC_Check(UINT32 u4InstID, UINT32 u4FrameNum, UINT32 u4CoreId)
{
    UINT32 u4FileNameLen = 0;
    UINT32 i = 0;
    UINT32 u4HW_Y_Result[4] = {0};
    UINT32 u4HW_CbCr_Result[4] = {0};
    UINT32 u4Golden = 0;
    UINT32 u4GoldenOffset = 4 ;
    char file_name[200] = {0};
    BOOL fgOpen = FALSE;
    BOOL fgCmpRet = TRUE;
    BOOL fgDumpY = FALSE;
    BOOL fgDumpC = FALSE;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;
    VDEC_INFO_VP9_FB_INFO_T* prFBInfo;

    prFBInfo = _VP9_GetFBInfo(u4InstID);
    prStatus = _VP9_GetStatusInfo(u4InstID);

    if(debug_mode > 0)
    {
        printk("%s\n",__FUNCTION__);
    }
    if (prStatus->fgVP9CRCOpen == FALSE)
    {
        // Load Y-CRC to DRAM
//	        if(prStatus->u4DualCore)
//	        {
            if(prStatus->u4UFOMode)
            {
                sprintf(file_name, "%scrc/crc_ufo_mcore%d_Y.dat", bitstream_name, 0);
            }
            else // w/o UFO
            {
                sprintf(file_name, "%scrc/crc_ufo_bypass_mcore%d_Y.dat", bitstream_name, 0);
            }

            printk("// Y0 CRC file = %s\n", file_name);
            _tInCRCFileInfo[u4InstID].fgGetFileInfo = TRUE;
            _tInCRCFileInfo[u4InstID].pucTargetAddr = _pucVP9CRCYBuf0[u4InstID];
            _tInCRCFileInfo[u4InstID].u4TargetSz = VP9_CRC_BUFFER_SZ;
            _tInCRCFileInfo[u4InstID].u4FileLength = 0;
            _tInCRCFileInfo[u4InstID].u4FileOffset = 0;

            fgOpen = fgOpenFile(u4InstID, file_name, "r+b", &_tInCRCFileInfo[u4InstID]);
            if (fgOpen == FALSE)
            {
                printk("// [VP9] open Y0 CRC file fail!!\n");
                return TRUE;
            }

            HalFlushInvalidateDCache();
            memset(file_name, 0, 200);

            if(prStatus->u4UFOMode)
            {
                sprintf(file_name, "%scrc/crc_ufo_mcore%d_Y.dat", bitstream_name, 1);
            }
            else
            {
                sprintf(file_name, "%scrc/crc_ufo_bypass_mcore%d_Y.dat", bitstream_name, 1);
            }

            printk("// Y1 CRC file = %s\n", file_name);

            _tInCRCFileInfo[u4InstID].pucTargetAddr = _pucVP9CRCYBuf1[u4InstID];
            _tInCRCFileInfo[u4InstID].u4TargetSz = VP9_CRC_BUFFER_SZ;
            _tInCRCFileInfo[u4InstID].u4FileLength = 0;
            _tInCRCFileInfo[u4InstID].u4FileOffset = 0;

            fgOpen = fgOpenFile(u4InstID, file_name, "r+b", &_tInCRCFileInfo[u4InstID]);


            if (fgOpen == FALSE)
            {
                printk("// [VP9] open Y1 CRC file fail!!\n");
                return TRUE;
            }
//	        }
//	        else // single-core
//	        {
            HalFlushInvalidateDCache();

            if(prStatus->u4UFOMode)
            {
                sprintf(file_name, "%scrc/crc_ufo_single_Y.dat", bitstream_name);
            }
            else // w/o UFO
            {
                sprintf(file_name, "%scrc/crc_ufo_bypass_single_Y.dat", bitstream_name);
            }

            printk("// Y CRC file = %s\n", file_name);
            _tInCRCFileInfo[u4InstID].fgGetFileInfo = TRUE;
            _tInCRCFileInfo[u4InstID].pucTargetAddr = _pucVP9CRCYBuf2[u4InstID];
            _tInCRCFileInfo[u4InstID].u4TargetSz = VP9_CRC_BUFFER_SZ;
            _tInCRCFileInfo[u4InstID].u4FileLength = 0;
            _tInCRCFileInfo[u4InstID].u4FileOffset = 0;

            fgOpen = fgOpenFile(u4InstID, file_name, "r+b", &_tInCRCFileInfo[u4InstID]);

            if (fgOpen == FALSE)
            {
                printk("// [VP9] open Y CRC file fail!!\n");
                return TRUE;
            }
//	        }

        HalFlushInvalidateDCache();
        // Load C-CRC to DRAM
        memset(file_name, 0, 200);

//	        if(prStatus->u4DualCore)
//	        {
            if(prStatus->u4UFOMode)
            {
                sprintf(file_name, "%scrc/crc_ufo_mcore%d_C.dat", bitstream_name, 0);
            }
            else
            {
                sprintf(file_name, "%scrc/crc_ufo_bypass_mcore%d_C.dat", bitstream_name, 0);
            }
            printk("// CRC0 file = %s\n", file_name);

            _tInCRCFileInfo[u4InstID].pucTargetAddr = _pucVP9CRCCBuf0[u4InstID];
            _tInCRCFileInfo[u4InstID].u4TargetSz = VP9_CRC_BUFFER_SZ;
            _tInCRCFileInfo[u4InstID].u4FileLength = 0;
            _tInCRCFileInfo[u4InstID].u4FileOffset = 0;

            fgOpen = fgOpenFile(u4InstID, file_name, "r+b", &_tInCRCFileInfo[u4InstID]);

            if (fgOpen == FALSE)
            {
                printk("// [VP9] open C0 CRC file fail!!\n");
                return TRUE;
            }

    HalFlushInvalidateDCache();
            memset(file_name, 0, 200);

            if(prStatus->u4UFOMode)
            {
                sprintf(file_name, "%scrc/crc_ufo_mcore%d_C.dat", bitstream_name, 1);
            }
            else
            {
                sprintf(file_name, "%scrc/crc_ufo_bypass_mcore%d_C.dat", bitstream_name, 1);
            }
            printk("// CRC1 file = %s\n", file_name);

            _tInCRCFileInfo[u4InstID].pucTargetAddr = _pucVP9CRCCBuf1[u4InstID];
            _tInCRCFileInfo[u4InstID].u4TargetSz = VP9_CRC_BUFFER_SZ;
            _tInCRCFileInfo[u4InstID].u4FileLength = 0;
            _tInCRCFileInfo[u4InstID].u4FileOffset = 0;

            fgOpen = fgOpenFile(u4InstID, file_name, "r+b", &_tInCRCFileInfo[u4InstID]);

            HalFlushInvalidateDCache();
            if (fgOpen == FALSE)
            {
                printk("// [VP9] open C1 CRC file fail!!\n");
                return TRUE;
            }
//	        }
//	        else // single-core
//	        {
            if(prStatus->u4UFOMode)
            {
                sprintf(file_name, "%scrc/crc_ufo_single_C.dat", bitstream_name);
            }
            else
            {
                sprintf(file_name, "%scrc/crc_ufo_bypass_single_C.dat", bitstream_name);
            }
            printk("// CRC file = %s\n", file_name);

            _tInCRCFileInfo[u4InstID].pucTargetAddr = _pucVP9CRCCBuf2[u4InstID];
            _tInCRCFileInfo[u4InstID].u4TargetSz = VP9_CRC_BUFFER_SZ;
            _tInCRCFileInfo[u4InstID].u4FileLength = 0;
            _tInCRCFileInfo[u4InstID].u4FileOffset = 0;

            fgOpen = fgOpenFile(u4InstID, file_name, "r+b", &_tInCRCFileInfo[u4InstID]);

            if (fgOpen == FALSE)
            {
                printk("// [VP9] open C CRC file fail!!\n");
                return TRUE;
            }
//	        }
        HalFlushInvalidateDCache();

        prStatus->fgVP9CRCOpen = TRUE;
    }

    /*
    Core0 (single) Y: mcore_top_193 ~ 196
    Core0 (single) C: mcore_top_197 ~ 200
    Core1 Y:          mcore_top_201 ~ 204
    Core1 C:          mcore_top_205 ~ 208
    10-bit the same
    */

    // check Y CRC
    for (i = 0; i < 4; i++)
    {
        if(prStatus->u4DualCore)
        {
            RISCRead_MCore_TOP(193 + i, &u4HW_Y_Result[i], u4CoreId);

            // core 0 compare
            u4Golden = ((UINT32*)_pucVP9CRCYBuf0[u4InstID])[u4FrameNum*u4GoldenOffset + i];
            if (u4HW_Y_Result[i] != u4Golden)
            {
                fgCmpRet = FALSE;
                fgDumpY = TRUE;
                printk("// [VP9] Y CRC compare fail!!\n");
                printk("// [VP9][Core-%d] i:%d, HW: 0x%x, Golden: 0x%x\n", 0, i, u4HW_Y_Result[i], u4Golden);
            }

            RISCRead_MCore_TOP(201 + i, &u4HW_Y_Result[i], u4CoreId);

            u4Golden = ((UINT32*)_pucVP9CRCYBuf1[u4InstID])[u4FrameNum*u4GoldenOffset + i];
            // core 1 compare
            if (u4HW_Y_Result[i] != u4Golden)
            {
                fgCmpRet = FALSE;
                fgDumpY = TRUE;
                printk("// [VP9] Y CRC compare fail!!\n");
                printk("// [VP9][Core-%d] i:%d, HW: 0x%x, Golden: 0x%x\n", 1, i, u4HW_Y_Result[i], u4Golden);
            }
        }
        else // single core
        {
            RISCRead_MCore_TOP(193 + i, &u4HW_Y_Result[i], u4CoreId);

            u4Golden = ((UINT32*)_pucVP9CRCYBuf2[u4InstID])[u4FrameNum*u4GoldenOffset + i];
            if (u4HW_Y_Result[i] != u4Golden)
            {
                fgCmpRet = FALSE;
                fgDumpY = TRUE;
                printk("// [VP9] Y CRC compare fail!!\n");
                printk("// [VP9] i:%d, HW: 0x%x, Golden: 0x%x\n", i, u4HW_Y_Result[i], u4Golden);
            }
        }
    }

#ifndef SATA_HDD_READ_SUPPORT
    #if 1
        // dump dram
        if( fgDumpY )
        {
            VP9DumpMem( u4InstID, prFBInfo->VP9_FBM_YAddr[prFBInfo->u4CurrentFBId],
                        prFBInfo->PIC_SIZE_Y, u4FrameNum , 1, 0);
        }
    #endif
#endif

    // check C CRC
    for (i = 0; i < 4; i++)
    {
        if(prStatus->u4DualCore)
        {
            RISCRead_MCore_TOP(197 + i, &u4HW_CbCr_Result[i], u4CoreId);

            u4Golden = ((UINT32*)_pucVP9CRCCBuf0[u4InstID])[u4FrameNum*u4GoldenOffset + i];
            if (u4HW_CbCr_Result[i] != u4Golden)
            {
                fgCmpRet = FALSE;
                fgDumpC = TRUE;
                printk("// [VP9] CbCr CRC compare fail!!\n");
                printk("// [VP9][Core-%d] i:%d, HW: 0x%x, Golden: 0x%x\n", 0, i, u4HW_CbCr_Result[i], u4Golden);
            }

            RISCRead_MCore_TOP(205 + i, &u4HW_CbCr_Result[i], u4CoreId);

            u4Golden = ((UINT32*)_pucVP9CRCCBuf1[u4InstID])[u4FrameNum*u4GoldenOffset + i];
            if (u4HW_CbCr_Result[i] != u4Golden)
            {
                fgCmpRet = FALSE;
                fgDumpC = TRUE;
                printk("// [VP9] CbCr CRC compare fail!!\n");
                printk("// [VP9][Core-%d] i:%d, HW: 0x%x, Golden: 0x%x\n", 1, i, u4HW_CbCr_Result[i], u4Golden);
            }
        }
        else // single core
        {
            RISCRead_MCore_TOP(197 + i, &u4HW_CbCr_Result[i], u4CoreId);

            u4Golden = ((UINT32*)_pucVP9CRCCBuf2[u4InstID])[u4FrameNum*u4GoldenOffset + i];
            if (u4HW_CbCr_Result[i] != u4Golden)
            {
                fgCmpRet = FALSE;
                fgDumpC = TRUE;
                printk("// [VP9] CbCr CRC compare fail!!\n");
                printk("// [VP9] i:%d, HW: 0x%x, Golden: 0x%x\n", i, u4HW_CbCr_Result[i], u4Golden);
            }
        }
    }

#ifndef SATA_HDD_READ_SUPPORT
        if( fgDumpC )
        {
            VP9DumpMem( u4InstID, prFBInfo->VP9_FBM_CAddr[prFBInfo->u4CurrentFBId],
                        prFBInfo->PIC_SIZE_C, u4FrameNum , 2, 0);
        }
#endif
        if (!fgCmpRet)
        {
            if(prStatus->u4DualCore)
            {
                VP9DumpMem( u4InstID, g2LAEBuffer[u4InstID][0],
                            VP9_LAE_BUFFER_SZ, u4FrameNum , 3, 0);

                VP9DumpMem( u4InstID, g2ErrBuffer[u4InstID][0],
                        VP9_ERR_BUFFER_SZ, u4FrameNum , 4, 0);
            }
            return TRUE;
        }
        else
        {
            return FALSE;
        }
}

void VP9_MC_Prefetch_Log(UINT32 u4InstID, UINT32 u4FrameNum, UINT32 u4CoreId)
{
    UINT32 u4NBM_DLE_NUM, u4ESA_REQ_DATA_NUM, u4MC_REQ_DATA_NUM, u4UFO_DLE_NUM;
    UCHAR  ucCaseName[200] = {0};
    UCHAR  ucTmpStr[200] = {0};
    UCHAR  *ucCurrent = NULL;
    UCHAR* const delim = "\\";
    UCHAR* ucToken;

    u4NBM_DLE_NUM = 0;
    u4ESA_REQ_DATA_NUM = 0;
    u4MC_REQ_DATA_NUM = 0;
    u4UFO_DLE_NUM = 0;

    strcpy(ucTmpStr, bitstream_name);

    RISCRead_MC(476, &u4NBM_DLE_NUM, u4CoreId);
    RISCRead_MC(558, &u4ESA_REQ_DATA_NUM, u4CoreId);
    RISCRead_MC(650, &u4MC_REQ_DATA_NUM, u4CoreId);
    RISCRead_MC(723, &u4UFO_DLE_NUM, u4CoreId);

    ucCurrent = ucTmpStr;
    while (ucToken = strsep(&ucCurrent, delim))
    {
        if(strlen(ucToken) > 1)
        {
            strcpy(ucCaseName, ucToken);
        }
    }
    printk("@@ VP9 MC_PREFETCH %s %d %d %d %d %d \n", ucCaseName, u4FrameNum,
           u4NBM_DLE_NUM, u4ESA_REQ_DATA_NUM, u4MC_REQ_DATA_NUM, u4UFO_DLE_NUM);

    return;
}

// performance measurement
void VP9_Performance_Log(UINT32 u4InstID, UINT32 u4FrameNum, UINT32 u4CoreId)
{
    UINT32 u4VDEC_CLK_Count = 0;
    UINT32 u4AVG_Cycle_MB = 0;
    UINT32 u4NBM_DLE_Count = 0;
    UINT32 u4MC_BandWidth = 0;
    UINT32 u4SBInfo = 0;
    UINT32 u4Pic_SB_Width_M1 = 0;
    UINT32 u4Pic_SB_Height_M1 = 0;
    UINT32 u4Total_MB = 0;
    UCHAR  ucCaseName[200] = {0};
    UCHAR  ucTmpStr[200] = {0};
    UCHAR  *ucCurrent = NULL;
    UCHAR* const delim = "\\";
    UCHAR* ucToken;

    strcpy(ucTmpStr, bitstream_name);

    RISCRead_VLD_TOP(40, &u4VDEC_CLK_Count, u4CoreId);
    RISCRead_MC(476, &u4NBM_DLE_Count, u4CoreId);
    RISCRead_VLD_TOP(26, &u4SBInfo, u4CoreId);
    u4Pic_SB_Height_M1 = (u4SBInfo >> (8 * 2));
    u4Pic_SB_Width_M1 = u4SBInfo & 0xffff;

    u4Total_MB = (u4Pic_SB_Height_M1 + 1) * (u4Pic_SB_Width_M1 + 1) * 16;
    u4AVG_Cycle_MB = u4VDEC_CLK_Count / u4Total_MB;
    u4MC_BandWidth = ((u4NBM_DLE_Count * 16 * 30)) / u4Total_MB;

    ucCurrent = ucTmpStr;
    while (ucToken = strsep(&ucCurrent, delim))
    {
        if(strlen(ucToken) > 1)
        {
            strcpy(ucCaseName, ucToken);
        }
    }

    printk("u4Total_MB %d  \n", u4Total_MB);
    printk("@@ VP9 Performance %s %d %d %d %d %d \n", ucCaseName, u4FrameNum,
           u4VDEC_CLK_Count, u4AVG_Cycle_MB, u4NBM_DLE_Count, u4MC_BandWidth);

    return;
}

#if VP9_RING_VFIFO_SUPPORT
void VP9_RingFifo(UINT32 u4InstID,  UINT32 u4CoreId)
{
    char * file_name[200] = {0};

    if ( error_rate > 0 && debug_mode !=0 )
    {
        sprintf( file_name, "%s_EB_0x%08X_%d", bitstream_name, 0, error_rate );
    }
    else
    {
        sprintf(file_name, "%svld_pat/bitstream.bin", bitstream_name  );
    }


    UINT32 u4VldRp;
    RISCRead_VLD(63, &u4VldRp, u4CoreId);
    u4VldRp = u4VldRp - (PHYSICAL)(g2VFIFO[u4InstID]);
    printk("Fifo offset:0x%x\n", u4VldRp);
    if((_u4LoadBitstreamCnt[u4InstID]&0x1) && (u4VldRp >((UINT32) (BITSTREAM_BUFF_SIZE/2))))
    {
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
        _tFBufFileInfo[u4InstID].pucTargetAddr = g2VFIFO[u4InstID];
        _tFBufFileInfo[u4InstID].u4FileOffset = ((BITSTREAM_BUFF_SIZE * (_u4LoadBitstreamCnt[u4InstID]+ 1))/2);
        //UTIL_Printf("InstID %d, LoadBitstreamCnt %d, fileoffset 0x%x\n", u4InstID, _u4LoadBitstreamCnt[u4InstID], _tInFileInfo[u4InstID].u4FileOffset);
        _tFBufFileInfo[u4InstID].u4TargetSz = (BITSTREAM_BUFF_SIZE/2);
        _tFBufFileInfo[u4InstID].u4FileLength = 0;

        fgOpenPCFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);


        _tFBufFileInfo[u4InstID].u4FileLength = _tFBufFileInfo[u4InstID].u4FileOffset + _tFBufFileInfo[u4InstID].u4RealGetBytes;
        _u4LoadBitstreamCnt[u4InstID]++;
        printk("[INFO] u4FileOffset =  0x%08X; Read size = 0x%08X bytes\n",_tFBufFileInfo[u4InstID].u4FileOffset, _tFBufFileInfo[u4InstID].u4RealGetBytes);
      }
      else if((!(_u4LoadBitstreamCnt[u4InstID]&0x1)) && (u4VldRp < ( BITSTREAM_BUFF_SIZE/2)))
      {
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
        _tFBufFileInfo[u4InstID].pucTargetAddr = g2VFIFO[u4InstID] + (BITSTREAM_BUFF_SIZE/2);
        _tFBufFileInfo[u4InstID].u4FileOffset = ((BITSTREAM_BUFF_SIZE * (_u4LoadBitstreamCnt[u4InstID]+ 1))/2);
        //UTIL_Printf("2~~InstID %d, LoadBitstreamCnt %d, fileoffset 0x%x\n", u4InstID, _u4LoadBitstreamCnt[u4InstID], _tInFileInfo[u4InstID].u4FileOffset);
        _tFBufFileInfo[u4InstID].u4TargetSz = (BITSTREAM_BUFF_SIZE/2);
        _tFBufFileInfo[u4InstID].u4FileLength = 0;

        fgOpenPCFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

        _tFBufFileInfo[u4InstID].u4FileLength = _tFBufFileInfo[u4InstID].u4FileOffset + _tFBufFileInfo[u4InstID].u4RealGetBytes;
        _u4LoadBitstreamCnt[u4InstID]++;
        printk("[INFO] u4FileOffset =  0x%08X; Read size = 0x%08X bytes\n",_tFBufFileInfo[u4InstID].u4FileOffset, _tFBufFileInfo[u4InstID].u4RealGetBytes);
      }
      HalFlushInvalidateDCache();
}
#endif

void VP9_AllocateWorkingBuffer(UINT32 u4InstID)
{
    // file list buffer allocation
    _pucFileListSa[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(FILE_LIST_SZ,1024, 0);
    UTIL_Printf("_pucFileListSa[u4InstID] = 0x%x, size is 0x%lx, PHYSICAL:0x%x\n", _pucFileListSa[u4InstID],
                 FILE_LIST_SZ, PHYSICAL(_pucFileListSa[u4InstID]));

    g2RiscBuffer[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VP9_RISC_BUFFER_SIZE,1024, 1);
    UTIL_Printf("g2RiscBuffer buffer[%d] 0x%08x size 0x%x\n",u4InstID,g2RiscBuffer[u4InstID],VP9_RISC_BUFFER_SIZE);

    g2TileBuffer[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VP9_TILE_BUFFER_SIZE,1024, 1);
    UTIL_Printf("g2TileBuffer buffer[%d] 0x%08x size 0x%x\n",u4InstID,g2TileBuffer[u4InstID],VP9_TILE_BUFFER_SIZE);

    g2CountTblBuffer[u4InstID] = (UCHAR *)x_alloc_aligned_verify_mem(VP9_COUNT_TBL_SZ,1024, 1);
    UTIL_Printf("g2CountTblBuffer buffer[%d] 0x%08x size 0x%x\n",u4InstID,g2CountTblBuffer[u4InstID],VP9_COUNT_TBL_SZ);
}


void VP9_ReleaseWorkingBuffer(UINT32 u4InstID)
{
    if(_pucFileListSa[u4InstID] != NULL)
    {
        x_free_aligned_verify_mem(_pucFileListSa[u4InstID], 1);
    }

    if( g2RiscBuffer[u4InstID] != NULL)
    {
        x_free_aligned_verify_mem(g2RiscBuffer[u4InstID], 1);
    }

    if( g2TileBuffer[u4InstID] != NULL)
    {
        x_free_aligned_verify_mem(g2TileBuffer[u4InstID], 1);
    }

    if( g2CountTblBuffer[u4InstID] != NULL)
    {
        x_free_aligned_verify_mem(g2CountTblBuffer[u4InstID], 1);
    }
}
#endif
int VP9_RegDump(UINT32 u4Base, UINT32 u4Start, UINT32 u4End , UINT32 frame_number, BOOL bDecodeDone)
{
    int ret,i;
    UINT32 u4Value;

    if (bDecodeDone)
    {
        printk("================== Decode Done register dump ==================\n");
    }
    else
    {
        printk("================== Before trigger decode register dump ==================\n");
    }

    for ( i = u4Start ; i <= u4End ; i++ )
    {
        u4Value = u4ReadReg( u4Base+ i*4 );

        if ( u4Base == VLD_REG_OFFSET0 )
            printk( "VLD[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == VDEC_BS2_OFFSET0 )
            printk( "VLD0_BS2[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == MC_REG_OFFSET0 )
            printk( "MC[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == VP9_VLD_REG_OFFSET0 )
            printk( "VP9_VLD[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
//        if ( u4Base == VP9_VLD_REG_OFFSET1 )
//            printk( "VP9_VLD1[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == HEVC_PP_REG_OFFSET0 )
            printk( "PP[%d] = 0x%08.0X    (Addr: 0x%08.0X)   ",i,u4Value, u4Base + 4*i);
        if ( u4Base == HEVC_MV_REG_OFFSET0 )
            printk( "MV[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == HEVC_MISC_REG_OFFSET0 )
            printk( "MISC[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == VLD_TOP_REG_OFFSET0 )
            printk( "VLD_TOP[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == VLD_REG_OFFSET1 )
            printk( "VLD1[%d] = 0x%08.0X   (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
//        if ( u4Base == VDEC_BS2_OFFSET1 )
//            printk( "VLD1_BS2[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == MC_REG_OFFSET1 )
            printk( "MC1[%d] = 0x%08.0X   (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
//        if ( u4Base == HEVC_VLD_REG_OFFSET1 )
//            printk( "HEVC_VLD1[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
//        if ( u4Base == HEVC_PP_REG_OFFSET1 )
//            printk( "PP1[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
//        if ( u4Base == HEVC_MV_REG_OFFSET1 )
//            printk( "MV1[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
//        if ( u4Base == HEVC_MISC_REG_OFFSET1 )
//            printk( "MISC1[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == VLD_TOP_REG_OFFSET1 )
            printk( "VLD_TOP1[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
#if 0
        if ( u4Base == MVDEC_TOP_OFFSET0 )
            printk( "VLD_MCore[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_VLD_OFFSET0 )
            printk( "LAE_VLD_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_BS2_OFFSET0)
            printk( "LAE_BS2[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_VLDTOP_OFFSET0 )
            printk( "LAE_VLDTOP_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_AVCVLD_OFFSET0 )
            printk( "LAE_AVCVLD_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_MISC_OFFSET0 )
            printk( "LAE_MISC_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_HEVCVLD_OFFSET0 )
            printk( "LAE_HEVCVLD_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        //if ( u4Base == LAE_LARB_OFFSET0 )
        //    printk( "LAE_LARB_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_VP9_VLD_OFFSET0 )
            printk( "LAE_VP9_VLD_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
        if ( u4Base == LAE_MV_OFFSET0)
            printk( "LAE_MV_OFFSET0[%d] = 0x%08.0X    (Addr: 0x%08.0X)  ",i,u4Value, u4Base + 4*i);
#endif
        if((i - u4Start)%2)
        {
            printk("\n");
        }
    }
    printk("\n");
    return 0;
}

#if 0
void VP9_Verify_Thread(void *pParam)
{
    void** ppv_param = (void**)pParam ;
    UINT32 u4InstID;
    BOOL fgOpen,fgExistVerifyLoop;
    char strMessage[512];
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    u4InstID =  (UINT32) ppv_param[0];
    prStatus = _VP9_GetStatusInfo(u4InstID);

    prStatus->u4UFOMode  =  (UINT32) ppv_param[1];
    //prStatus->u4DualCore  =  (UINT32) ppv_param[2];
    // dynamically switch base on risc_settings
    prStatus->u4DualCore  =  0;

    prStatus->u4VP9FileScanned = 0;
    VP9_AllocateWorkingBuffer(u4InstID);

    UTIL_Printf("InstId %d, UFO %d, DualCore %d\n",u4InstID, prStatus->u4UFOMode, prStatus->u4DualCore);

    while(fgVdecReadFileName(u4InstID, &_tFileListInfo[u4InstID], &_tFileListRecInfo[u4InstID], &_u4StartCompPicNum[u4InstID],
          &_u4EndCompPicNum[u4InstID], &_u4DumpRegPicNum[u4InstID]))
    {
        memset(bitstream_name, 0, 200);
        strncpy (bitstream_name , _bFileStr1[u4InstID][1], (strlen(_bFileStr1[u4InstID][1]) - 21));
        printk("Stream %s, StartPic %d, EndPic %d\n", bitstream_name, _u4StartCompPicNum[u4InstID], _u4EndCompPicNum[u4InstID]);
        printk("_u4CodecVer 0x%x\n", _u4CodecVer[u4InstID]);
        prStatus->u4BistreamResult = DECODE_OK;
        prStatus->fgVP9CRCOpen = FALSE;

        switch(_u4CodecVer[u4InstID])
        {
            case VDEC_VP9:
                VDEC_Pattern_MemoryAllocate(u4InstID, VDEC_VP9);
                vp9_test(u4InstID, prStatus->u4DualCore, prStatus->u4UFOMode, _u4StartCompPicNum[u4InstID], _u4EndCompPicNum[u4InstID]);
                break;

            case VDEC_H265:
                // maybe extend H265 here?, if have time..
                break;

            default:
                printk("@@@ Unknown Codec specified, Please check!!!!\n");
                break;
        }
        prStatus->u4VP9FileScanned++;
    }

    VP9_ReleaseWorkingBuffer(u4InstID);
    return;
}

void vp9_test(int u4InstID, bool fgDualCore, bool fgUFOMode, int frame_start , int frame_end )
{
    UINT32 u4FrameWidth, u4FrameHeight;
    UINT32 u4SBWidth, u4SBHeight;
    UINT32 u4FrameNum;
    UINT32 u4Ret;
    UINT32 i;

    UINT32 PIC_SIZE_Y, PIC_SIZE_C;

    struct file *fd;
    struct file *fd_bitstream;
    struct file *filp_info;
    char *ptr_base;
    char *ptr_base_test;
    char * file_name[200] = {0};

    UINT32 u4RiscVal1, u4RiscVal2, u4RiscVal3, u4RiscVal4, u4RiscVal5, u4RiscVal6, u4RiscVal7;
    UINT32 u4PatOffset;
    UINT32 u4Skip;
    UINT32 u4CoreId = 0;
    UINT32 u4UFOtemp;
    UINT32 maxMC = 0;
    UINT32 maxVLD = 0;
    const int buff_risc_size = 0x40000;

    int i4tmp = 0;
    unsigned char ucRiscTmp[100];
    unsigned char ucRiscType[100];
    unsigned char ucRiscAddr[100];
    unsigned char *pucRiscBuff;

    //int file_num, file_len, read_len;
    int repeat_count = 0;
    int i4RiscTmp;

    UINT32 u4IsFail;
    UINT32* ptr_temp;

    UINT32 physAlloc = 0x90100000;
    UINT32 u4FileSize;
    UINT32 BITSTREAM_OFFSET;
    UINT32 BitstreamOffsetBase;     // for shift bits
    UINT32 u4BitstreamOffset;
    UINT32 FRAME_BUFFER_OFFSET = 0x9166d000;
    UINT32 BITSTREAM_BUFF_SHIFT = 0x4600000;    //0x9600000; 0x4600000;  0x1E00000

    UINT32 u4CurrentYAddr, u4CurrentCAddr;
    UINT32 u4Align;
    UINT32 u4FifoEnough = 1;

    // for UFO MODE
    UINT32 UFO_Y_LEN_ADDR, UFO_C_LEN_ADDR;
    UINT32 UFO_LEN_SIZE_Y, UFO_LEN_SIZE_C;
    UINT32 PIC_SIZE_REF, PIC_SIZE_BS, PIC_SIZE_Y_BS;
    UINT32 PIC_UFO_WIDTH, PIC_UFO_HEIGHT, PIC_W_H;

    VDEC_INFO_VP9_FB_INFO_T* prFBInfo;
    VDEC_INFO_VP9_STATUS_INFO_T* prStatus;

    prFBInfo = _VP9_GetFBInfo(u4InstID);
    prStatus = _VP9_GetStatusInfo(u4InstID);

    BOOL fgOpen = FALSE;

    debug_mode = 0;

    // Load Pic Width
    sprintf(file_name, "%swidth_max.txt", bitstream_name );

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = ucRiscTmp;
    _tFBufFileInfo[u4InstID].u4TargetSz = 100;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    memset ( ucRiscTmp , 0 , 100 );

    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);
    if (fgOpen == FALSE)
    {
       sprintf(file_name, "%smv_pat/pic_width.txt", bitstream_name );
       memset ( ucRiscTmp , 0 , 100 );
       fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

       if(fgOpen == FALSE)
       {
           UTIL_Printf("[Error] Open fail %s @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
           UTIL_Printf("\n@@ VP9 Decode NG. Fail to find Pic width  [%s] \n", bitstream_name);
           return;
       }
    }
    HalFlushInvalidateDCache();

    sscanf (ucRiscTmp, "%i ", &(prFBInfo->u4Width));
    printf("W --> %d\n", prFBInfo->u4Width);

    // Load Pic Height;
     sprintf(file_name, "%sheight_max.txt", bitstream_name );

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = ucRiscTmp;
    _tFBufFileInfo[u4InstID].u4TargetSz = 100;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    memset ( ucRiscTmp , 0 , 100 );
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

    if (fgOpen == FALSE)
    {
       sprintf(file_name, "%smv_pat/pic_height.txt", bitstream_name );
       memset ( ucRiscTmp , 0 , 100 );
       fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

       if (fgOpen == FALSE)
       {
           UTIL_Printf("[Error] Open fail %s @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
           UTIL_Printf("\n@@ VP9 Decode NG. Fail to find Pic height  [%s] \n", bitstream_name);
           return;
       }
    }
    HalFlushInvalidateDCache();

    sscanf (ucRiscTmp, "%i ", &(prFBInfo->u4Height));
    printf("H --> %d\n", prFBInfo->u4Height);

    if (prStatus->u4UFOMode)
    {
        printk("\n==================== UFO_MODE ====================\n\n");
    }

    //printk("\n%s\nFrame:%d-%d  Width:%d Height:%d ErrorRate:%d Debug:%d\n",  bitstream_name, frame_start, frame_end, width, height, error_rate ,debug_mode );

    u4FrameWidth = ((prFBInfo->u4Width + 63) >> 6) << 6; //64 align
    u4FrameHeight = ((prFBInfo->u4Height + 63) >> 6) << 6; //64 align

    u4SBWidth = ((prFBInfo->u4Width + 63) >> 6); // # of SB
    u4SBHeight = ((prFBInfo->u4Height + 63) >> 6); // # of SB

    PIC_SIZE_Y = (u4SBWidth * u4SBHeight) << (6 + 6);
    PIC_SIZE_C = PIC_SIZE_Y >> 1;

    #if 0
    PIC_SIZE_Y = u4FrameWidth*u4FrameHeight;
    PIC_SIZE_C = (( PIC_SIZE_Y + ( PIC_SIZE_Y >> 1 ) + 511 )>> 9) << 9; //512 align
    #endif

    // move to emulation thread
    #if 0
    VDEC_Pattern_MemoryAllocate(u4InstID, VDEC_VP9);
    #endif
    BITSTREAM_OFFSET = g2VFIFO[u4InstID];
    BitstreamOffsetBase = BITSTREAM_OFFSET;
    physAlloc = BITSTREAM_OFFSET + BITSTREAM_BUFF_SIZE;       //FIFO for bitstream 100MB


    //BITSTR_LOAD();
    #if VP9_RING_VFIFO_SUPPORT

    _u4LoadBitstreamCnt[u4InstID] = 0;
    ptr_base = g2VFIFO[u4InstID];
    memset(ptr_base, 0, BITSTREAM_BUFF_SIZE);

    if (debug_mode>0)
        printk("BITSTREAM_OFFSET ioremap ptr_base = 0x%08.0X\n",ptr_base);

    if ( error_rate > 0 && debug_mode !=0 )
    {
        sprintf( file_name, "%s_EB_0x%08X_%d", bitstream_name, 0, error_rate );
    }
    else
    {
        sprintf(file_name, "%svld_pat/bitstream.bin", bitstream_name  );
    }

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = ptr_base;
    _tFBufFileInfo[u4InstID].u4TargetSz = BITSTREAM_BUFF_SIZE;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

    if (fgOpen == FALSE)
    {
       UTIL_Printf("[Error] Open fail %s @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
       UTIL_Printf("\n@@ VP9 Decode NG. Fail to find bitstream.bin  [%s] \n", bitstream_name);
       return;
    }


    HalFlushInvalidateDCache();
    if(_tFBufFileInfo[u4InstID].u4FileLength > BITSTREAM_BUFF_SIZE)
     {
        UTIL_Printf("=====>The Vfifo size is not enough!. \n");
        UTIL_Printf("=====>The file's size is 0x%x, video fifo size:0x%x\n", _tFBufFileInfo[u4InstID].u4FileLength, BITSTREAM_BUFF_SIZE);
        u4FifoEnough = 0;
    }

    _u4LoadBitstreamCnt[u4InstID]++;

    #else

    ptr_base = g2VFIFO[u4InstID];
    memset(ptr_base, 0, BITSTREAM_BUFF_SIZE);
    if (debug_mode>0)
        printk("BITSTREAM_OFFSET ioremap ptr_base = 0x%08.0X\n",ptr_base);

    if ( error_rate > 0 && debug_mode !=0 )
    {
        sprintf( file_name, "%s_EB_0x%08X_%d", bitstream_name, 0, error_rate );
    }
    else
    {
        sprintf(file_name, "%svld_pat/bitstream.bin", bitstream_name  );
    }

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tFBufFileInfo[u4InstID].pucTargetAddr = ptr_base;
    _tFBufFileInfo[u4InstID].u4TargetSz = BITSTREAM_BUFF_SIZE;
    _tFBufFileInfo[u4InstID].u4FileLength = 0;
    _tFBufFileInfo[u4InstID].u4FileOffset = 0;
    fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);

    if (fgOpen == FALSE)
    {
       UTIL_Printf("[Error] Open fail %s @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
       UTIL_Printf("\n@@ VP9 Decode NG. Fail to find bitstream.bin  [%s] \n", bitstream_name);
       return;
    }

    HalFlushInvalidateDCache();
    #endif

#if 0
    if ( debug_mode == 0 )
    {
        //Set_error_bitstream( BITSTREAM_OFFSET, BITSTREAM_BUFF_SIZE , u4FileSize , 0);
    }

    if (debug_mode>0)
    {
        printk("\n\n==== Bitstream %s, file size = %d ====\n\n", file_name ,u4FileSize);
    }

    if ( debug_mode == 3 )
    {
        physAlloc = FRAME_BUFFER_OFFSET;
    }
#endif

    //VP9_Check_VFIFO_DATA(ptr_base);

    //PREPARE_MC_BUF();
    //VDEC_MC_BUF_CFG(u4InstID, u4CoreId);
    VP9_FB_Config(u4InstID);
    //prStatus->u4DualCore = FALSE;

    //Loop over frames
    for ( u4FrameNum = frame_start; u4FrameNum <= frame_end; u4FrameNum++ )
    {
        printk("\n\n======== VP9 test frame %d ========\n\n" , u4FrameNum );

        prStatus->u4DualCore = FALSE;

        //verify frame_number exist
        if (prStatus->u4UFOMode)
        {
            sprintf(file_name, "%sufo_pat/ufo_%d_bits_Y.out", bitstream_name, u4FrameNum);
        }
        else
        {
            sprintf(file_name, "%spp_pat/frame_%d_Y.dat", bitstream_name, u4FrameNum);
        }

        _tFBufFileInfo[u4InstID].fgGetFileInfo = FALSE;
        _tFBufFileInfo[u4InstID].pucTargetAddr = ucRiscTmp;
        _tFBufFileInfo[u4InstID].u4TargetSz = 0xF;
        _tFBufFileInfo[u4InstID].u4FileLength = 0;
        _tFBufFileInfo[u4InstID].u4FileOffset = 0;
        fgOpen = fgOpenFile(u4InstID, file_name,"r+b", &_tFBufFileInfo[u4InstID]);
        if (fgOpen == FALSE)
        {
            UTIL_Printf("%s  Golden Missing  @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
            break; // no frame stop test
        }
        HalFlushInvalidateDCache();

        // Read Current Frame Settings
        pucRiscBuff = g2RiscBuffer[u4InstID];
        u4PatOffset = 0;
        sprintf(file_name, "%ssa_risc_setting%d.pat", bitstream_name, u4FrameNum);

        _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
        _tInFileInfo[u4InstID].pucTargetAddr = pucRiscBuff;
        _tInFileInfo[u4InstID].u4TargetSz = buff_risc_size;
        _tInFileInfo[u4InstID].u4FileLength = 0;
        _tFBufFileInfo[u4InstID].u4FileOffset = 0;

        memset( pucRiscBuff , 0 ,buff_risc_size );

        fgOpen = fgOpenFile(u4InstID, file_name,"r+", &_tInFileInfo[u4InstID]);
        if (fgOpen == FALSE)
        {
            UTIL_Printf("[Error] Open Risc Setting fail %s @ %d!!!!!!!!!!!!!\n",file_name,__LINE__);
            prStatus->u4BistreamResult = DECODE_MISSING_DATA;
            break; // no frame stop test
        }

        printk("Risc_Setting Real Size = 0x%x\n", _tInFileInfo[u4InstID].u4RealGetBytes);

        while ((_tInFileInfo[u4InstID].u4RealGetBytes - u4PatOffset) > 5 )
        {
            u4RiscVal1 = 0;
            u4RiscVal2 = 0;
            u4RiscVal3 = 0;
            u4RiscVal4 = 0;
            u4RiscVal5 = 0;
            u4RiscVal6 = 0;
            u4RiscVal7 = 0;
            i4RiscTmp = -1;
            u4CoreId = 0;
            u4Skip = 0;
            u4Ret = 0;

            if((*(pucRiscBuff + u4PatOffset) == 0x0D) && (*(pucRiscBuff + u4PatOffset + 1) == 0x0A))
            {
                u4PatOffset += 2;
                continue;
            }

            sscanf(pucRiscBuff + u4PatOffset, "%s %i\n", ucRiscType, &u4CoreId);

            // skip empty line and comment
            if((strncmp(ucRiscType,"//",2) != 0) && (ucRiscType[0] != '\n'))
            {
                // hw function.
                if(strncmp(ucRiscType, "RISC",4) != 0)
                {
                    if(strcmp(ucRiscType, "SOFT_RESET") == 0)
                    {
                        VP9_SOFT_RESET(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType, "INIT_BARREL_SHIFTER")== 0)
                    {
                        sscanf(pucRiscBuff + u4PatOffset, "%s %s %i %i\n", ucRiscType, ucRiscAddr, &u4RiscVal1, &u4CoreId);
                        u4BitstreamOffset = u4RiscVal1;
                        while(u4BitstreamOffset > BITSTREAM_BUFF_SIZE)
                        {
                            u4BitstreamOffset -= BITSTREAM_BUFF_SIZE;
                        }
                        VP9_INIT_BARREL_SHIFTER(u4InstID, BitstreamOffsetBase + u4BitstreamOffset, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_VLD_SHIFT_BIT")== 0)
                    {
                        sscanf(pucRiscBuff + u4PatOffset, "%s %i %i\n", ucRiscType, &u4RiscVal1, &u4CoreId);
                        VP9_VLD_SHIFT_BIT(u4InstID, u4RiscVal1, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_INIT_BOOL")== 0)
                    {
                        VP9_INIT_BOOL(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_COEF_PROBS")== 0)
                    {
                        VP9_UPDATE_COEF_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_MBSKIP_PROBS")== 0)
                    {
                        VP9_UPDATE_MBSKIP_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_INTER_MODE_PROBS")== 0)
                    {
                        VP9_UPDATE_INTER_MODE_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_SWITCHABLE_INTERP_PROBS")== 0)
                    {
                        VP9_UPDATE_SWITCHABLE_INTERP_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_TX_PROBS")== 0)
                    {
                        VP9_UPDATE_TX_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_INTRA_INTER_PROBS")== 0)
                    {
                        VP9_UPDATE_INTRA_INTER_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_SINGLE_REF_PROBS")== 0)
                    {
                        VP9_UPDATE_SINGLE_REF_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_Y_MODE_PROBS")== 0)
                    {
                        VP9_UPDATE_Y_MODE_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_PARTITION_PROBS")== 0)
                    {
                        VP9_UPDATE_PARTITION_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_MVD_INT_PROBS")== 0)
                    {
                        VP9_UPDATE_MVD_INT_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_MVD_FP_PROBS")== 0)
                    {
                        VP9_UPDATE_MVD_FP_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_MVD_HP_PROBS")== 0)
                    {
                        VP9_UPDATE_MVD_HP_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_COMP_REF_PROBS")== 0)
                    {
                        VP9_UPDATE_COMP_REF_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_UPDATE_COMP_INTER_PROBS")== 0)
                    {
                        VP9_UPDATE_COMP_INTER_PROBS(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VP9_READ_LITERAL")== 0)
                    {
                        sscanf(pucRiscBuff + u4PatOffset, "%s %i %i %i\n", ucRiscType, &u4RiscVal1, &u4RiscVal2, &u4CoreId);
                        VP9_READ_LITERAL(u4InstID, u4RiscVal1, u4RiscVal2, u4CoreId);
                    }
                    else if(strcmp( ucRiscType, "VDEC_MCORE_INIT") == 0)
                    {
                        VDEC_MCORE_INIT(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"VDEC_TRIG")== 0)
                    {
                        //VP9_RegDump (VP9_VLD_REG_OFFSET0, 0, 0, u4FrameNum, 0);     //Input Window
                        if(u4Skip == 1)
                            break;
                        #if VP9_PRE_REGISTER_DUMP
                        //RISCWrite_PP(54, 0, 0); // for debug bypass pp
                        VP9_RegDump (VLD_TOP_REG_OFFSET0, 0, 128, u4FrameNum, 0);   //VLD_TOP
                        VP9_RegDump (VLD_REG_OFFSET0, 33, 255,  u4FrameNum, 0);     //VLD
                        VP9_RegDump (VDEC_BS2_OFFSET0, 33, 255,  u4FrameNum, 0);     //VLD
                        VP9_RegDump (MC_REG_OFFSET0, 0, 864, u4FrameNum, 0);        //MC
                        VP9_RegDump (VP9_VLD_REG_OFFSET0, 0, 0, u4FrameNum, 0);     //Input Window
                        VP9_RegDump (VP9_VLD_REG_OFFSET0, 41, 124, u4FrameNum, 0);  //VP9 VLD
                        VP9_RegDump (HEVC_MISC_REG_OFFSET0, 0, 128, u4FrameNum, 0); //VLD_TOP
                        VP9_RegDump (HEVC_PP_REG_OFFSET0, 0, 896, u4FrameNum, 0);   //PP
                        #endif

                        if(fgUFOMode)
                        {
                            // clear output buffer
                            printk("// clear memory for UFO decode !!!\n");
                            memset(u4CurrentYAddr, 0, prFBInfo->u4DramPicY_Y_LENSize);
                            memset(u4CurrentCAddr, 0, prFBInfo->u4DramPicC_C_LENSize);
                        }
                        #if VP9_CRC_ENABLE
                        RISCWrite_MCore_TOP(192, 0x1, u4CoreId);
                        #endif
                        #if VP9_PERFORMANCE_LOG
                        RISCWrite_VLD_TOP(21, 0x1, u4CoreId);
                        #endif
                        u4Skip = VDEC_TRIG(u4InstID, u4CoreId);
                    }
                    else if(strcmp(ucRiscType,"TILE_INFO_LOAD")== 0)
                    {
                        VP9_TILE_INFO_LOAD(u4InstID, u4FrameNum, u4CoreId, PHYSICAL(BitstreamOffsetBase));
                    }
                    else if(strcmp(ucRiscType,"VDEC_UFO_CONFIG")== 0)
                    {
                        // be sure after MC config, we can get correct FB ID
                        if(fgUFOMode)
                        {
                            VP9_UFO_CONFIG(u4InstID, u4CoreId);
                        }
                    }
                    else if(strcmp(ucRiscType,"SEG_ID_RESET")== 0)
                    {
                        printk("SEG_ID_RESET\n");
                        if(u4CoreId != LAE_ID)
                        {
                            memset(g2SegIdWrapper[u4InstID][0], 0, PATTERN_SEG_ID_SZ);
                        }
                        else
                        {
                            memset(g2SegIdWrapper[u4InstID][1], 0, PATTERN_SEG_ID_SZ);
                        }
                    }
                    else if(strcmp(ucRiscType,"SKIP_FRAME") == 0)
                    {
                        printk("SKIP FRAME\n");
                        u4Skip = 1;
                        break;
                    }
                    else
                    {
                        printk("[Error] Unknown HW Function !!\n");
                        printk("[Error] Unknown HW Function !!\n");
                        printk("[Error] Unknown HW Function !!\n");
                        printk("[Error] Unknown HW Function !!\n");
                        printk("[Error] Unknown HW Function !!\n");
                        printk("[Error] Unknown HW Function !!\n");
                    }
                }
                else // risc read write.
                {
                    memset(ucRiscType, 0, 100);
                    sscanf(pucRiscBuff + u4PatOffset, "%s %i %s %i\n", ucRiscType, &u4RiscVal1, ucRiscAddr, &u4CoreId );
                    i4tmp = simple_strtol(ucRiscAddr, NULL, 10);  //for signed overflow
                    u4RiscVal2 = (i4tmp < 0)?  0x100000000 + i4tmp : i4tmp ;

                    if(debug_mode > 0)
                    {
                         //printk("%s (%u ,%s, %u);\n", ucRiscType, u4RiscVal1, ucRiscAddr, u4CoreId);
                    }

                    if(strcmp(ucRiscType, "RISCWrite_MV") == 0)
                    {
                        memset(ucRiscType, 0, 100);
                        sscanf(pucRiscBuff + u4PatOffset, "%s %i %s %i\n", ucRiscType, &u4RiscVal1, ucRiscAddr, &u4CoreId );

                        // only 1 MV buffer is needed.
                        if(strcmp(ucRiscAddr, "MV_BUF_ADDR_0") == 0)
                        {
                            RISCWrite_MV(u4RiscVal1, PHYSICAL(prFBInfo->VP9_MV_Addr[0]) >> 4, u4CoreId);
                        }
                        else if(strcmp(ucRiscAddr, "MV_BUF_ADDR_1") == 0)
                        {
                            RISCWrite_MV(u4RiscVal1, PHYSICAL(prFBInfo->VP9_MV_Addr[1]) >> 4, u4CoreId);
                        }
                        else
                        {
                            RISCWrite_MV(u4RiscVal1, u4RiscVal2, u4CoreId);
                        }
                    }
                    else if(strcmp(ucRiscType, "RISCWrite_PP") == 0)
                    {
                        RISCWrite_PP(u4RiscVal1, u4RiscVal2, u4CoreId);
                    }
                    else if(strcmp(ucRiscType, "RISCWrite_MC") == 0)
                    {
                        if(IS_VP9_MC_Y(u4RiscVal1) || IS_VP9_REF_Y(u4RiscVal1) || IS_VP9_MC_C(u4RiscVal1) ||
                           IS_VP9_REF_C(u4RiscVal1) )
                        {
                            memset(ucRiscType, 0, 100);
                            sscanf(pucRiscBuff + u4PatOffset, "%s %i %s %i %i\n",
                                   ucRiscType, &u4RiscVal1, ucRiscAddr, &u4RiscVal2, &u4CoreId );
                        }
                        else if(IS_VP9_REF_WIDTH_HEIGHT(u4RiscVal1))
                        {
                            memset(ucRiscType, 0, 100);
                            sscanf(pucRiscBuff + u4PatOffset, "%s %i %i %i %i\n",
                                   ucRiscType, &u4RiscVal1, &u4RiscVal2, &u4RiscVal3, &u4CoreId );
                        }
                        else if(IS_VP9_REF_WIDTH_HEIGHT(u4RiscVal1) || IS_VP9_REF_SCALING_STEP(u4RiscVal1)||
                                IS_VP9_TTL_MI_ROW_COL(u4RiscVal1))
                        {
                            memset(ucRiscType, 0, 100);
                            sscanf(pucRiscBuff + u4PatOffset, "%s %i %i %i %i\n",
                                   ucRiscType, &u4RiscVal1, &u4RiscVal2, &u4RiscVal3, &u4CoreId );
                        }
                        else if(IS_VP9_SCALING_EN(u4RiscVal1))
                        {
                            memset(ucRiscType, 0, 100);
                            sscanf(pucRiscBuff + u4PatOffset, "%s %i %i %i %i %i\n",
                                   ucRiscType, &u4RiscVal1, &u4RiscVal2, &u4RiscVal3, &u4RiscVal4,&u4CoreId );
                        }
                        else if(IS_VP9_MAX_BLK_SIZE_RRF(u4RiscVal1))
                        {
                            memset(ucRiscType, 0, 100);
                            sscanf(pucRiscBuff + u4PatOffset, "%s %i %i %i %i %i %i %i %i\n",
                                   ucRiscType, &u4RiscVal1, &u4RiscVal2, &u4RiscVal3, &u4RiscVal4, &u4RiscVal5,
                                   &u4RiscVal6, &u4RiscVal7, &u4CoreId);

                        }

                        // [vpx]
                        // buffer maintain take care
                        if(IS_VP9_MC_Y(u4RiscVal1) ) // OUT_Y
                        {
                            RISCWrite_MC(u4RiscVal1, PHYSICAL(prFBInfo->VP9_FBM_YAddr[u4RiscVal2]) >> 9 ,u4CoreId);
                            u4CurrentYAddr = prFBInfo->VP9_FBM_YAddr[u4RiscVal2];
                            prFBInfo->u4CurrentFBId = u4RiscVal2;
                            //RISCWrite_MC(u4RiscVal1, u4RiscVal2 * 9 ,u4CoreId);
                        }
                        else if(IS_VP9_REF_Y(u4RiscVal1)) // Set REF_Y, Last, Golden, ARF
                        {
                           RISCWrite_MC(u4RiscVal1, PHYSICAL(prFBInfo->VP9_FBM_YAddr[u4RiscVal2]) ,u4CoreId);
                           //RISCWrite_MC(u4RiscVal1, u4RiscVal2, u4CoreId);
                        }
                        else if(IS_VP9_MC_C(u4RiscVal1) ) // OUT_C
                        {
                            RISCWrite_MC(u4RiscVal1, PHYSICAL(prFBInfo->VP9_FBM_CAddr[u4RiscVal2]) >> 8 ,u4CoreId);
                            u4CurrentCAddr = prFBInfo->VP9_FBM_CAddr[u4RiscVal2];
                            //RISCWrite_MC(u4RiscVal1, u4RiscVal2 * 8 ,u4CoreId);
                        }
                        else if(IS_VP9_REF_C(u4RiscVal1)) // Set REF_C, Last, Golden, ARF
                        {
                            RISCWrite_MC(u4RiscVal1, PHYSICAL(prFBInfo->VP9_FBM_CAddr[u4RiscVal2]) ,u4CoreId);
                            //RISCWrite_MC(u4RiscVal1, u4RiscVal2, u4CoreId);
                        }
                        else if(IS_VP9_REF_WIDTH_HEIGHT(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, (u4RiscVal2 << 16) + u4RiscVal3 ,u4CoreId);
                            }
                        }
                        else if(IS_VP9_REF_SCALING_STEP(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, (u4RiscVal2 << 16) + u4RiscVal3 ,u4CoreId);
                            }
                        }
                        else if(IS_VP9_SCALING_EN(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, (u4RiscVal2 << 8) +( u4RiscVal3 << 4) + (u4RiscVal4), u4CoreId);
                            }
                        }
                        else if(IS_VP9_REF_SCALING_FACT(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, u4RiscVal2 ,u4CoreId);
                            }
                        }
                        else if(IS_VP9_MAX_BLK_SIZE_RRF(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, (u4RiscVal2 << 24) + (u4RiscVal3 << 20) +
                                             (u4RiscVal4 << 16) + (u4RiscVal5 << 8) + (u4RiscVal6 << 4) + u4RiscVal7,
                                             u4CoreId);
                            }
                        }
                        else if(IS_VP9_TTL_MI_ROW_COL(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, (u4RiscVal2 << 16) + u4RiscVal3 ,u4CoreId);
                            }
                        }
                        else if(IS_VP9_REF_DRAM_PITCH(u4RiscVal1))
                        {
                            // only set when P/B
                            if(VP9_IS_KEY_FRAME(u4CoreId) == 0)
                            {
                                RISCWrite_MC(u4RiscVal1, u4RiscVal2 ,u4CoreId);
                            }
                        }
                        else
                        {
                            RISCWrite_MC(u4RiscVal1,  u4RiscVal2, u4CoreId);
                        }
                    }
                    else if(strcmp(ucRiscType, "RISCWrite_VP9_VLD") == 0)
                    {
                        if(strcmp(ucRiscAddr, "BITSTREAM_OFFSET") == 0)
                        {
                            // [vpx]
                            sscanf(pucRiscBuff + u4PatOffset, "%s %i %s %i %i\n", ucRiscType, &u4RiscVal1, ucRiscAddr, &u4RiscVal2, &u4CoreId);
                            while(u4RiscVal2 > BITSTREAM_BUFF_SIZE)
                            {
                                u4RiscVal2 -= BITSTREAM_BUFF_SIZE;
                            }
                            RISCWrite_VP9_VLD(u4RiscVal1, PHYSICAL(BitstreamOffsetBase + u4RiscVal2), u4CoreId);
                        }
                        else
                        {
                            RISCWrite_VP9_VLD(u4RiscVal1, u4RiscVal2, u4CoreId);
                        }
                    }
                    else if(strcmp(ucRiscType, "RISCWrite_VLD") == 0)
                    {
                        RISCWrite_VLD(u4RiscVal1, u4RiscVal2, u4CoreId);
                    }
                    else if(strcmp(ucRiscType, "RISCWrite_VLD_TOP") == 0)
                    {
                        if(strcmp(ucRiscAddr, "SEGMENT_ID_ADDR") == 0)
                        {
                             RISCWrite_VLD_TOP(u4RiscVal1, PHYSICAL(g2SegIdWrapper[u4InstID][0]), u4CoreId);
                        }
                        else if(strcmp(ucRiscAddr, "SEGMENT_ID_ADDR_1") == 0)
                        {
                             RISCWrite_VLD_TOP(u4RiscVal1, PHYSICAL(g2SegIdWrapper[u4InstID][1]), u4CoreId);
                        }
                        else
                        {
                            RISCWrite_VLD_TOP(u4RiscVal1, u4RiscVal2, u4CoreId);
                        }
                    }
                    else if(strcmp(ucRiscType, "RISCWrite_BS2") == 0)
                    {
                        RISCWrite_BS2(u4RiscVal1, u4RiscVal2, u4CoreId);
                    }
                    else if(strcmp(ucRiscType, "RISCRead_VP9_VLD") == 0)
                    {
                        RISCRead_VP9_VLD(u4RiscVal1, &u4Ret, u4RiscVal2);
                    }
                    else
                    {
                        printk("[Error] Unknown RISC Command !!\n");
                    }
                }
            }
            #if 0
            else
            {
                if(debug_mode > 0)
                {
                    printk("%s \n", risc_type);
                }
            }
            #endif

            // polling decode done & buffer compare
            if(strcmp(ucRiscType,"VDEC_TRIG")== 0 && u4Skip == 0)
            {
                if( !prStatus->u4DualCore)
                {
                    prStatus->u4DecodeTimeOut = VP9_Wait_Decode_Done(jiffies, u4CoreId);
                }
                else
                {
                    prStatus->u4DecodeTimeOut = VP9_Wait_MCore_Decode_Done(jiffies);
                }
                #if VP9_RING_VFIFO_SUPPORT
                if(!u4FifoEnough && !prStatus->u4DecodeTimeOut)
                    VP9_RingFifo(u4InstID, u4CoreId);
                #endif
                //if (debug_mode > 0 || _u4DecodeTimeOut)
                if (prStatus->u4DecodeTimeOut)
                {
                    prStatus->u4BistreamResult = DECODE_TIMEOUT;

                    VP9_RegDump (VLD_TOP_REG_OFFSET0, 0, 128, u4FrameNum, 1);   //VLD_TOP
                    VP9_RegDump (VLD_REG_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                    VP9_RegDump (VDEC_BS2_OFFSET0, 33, 255,  u4FrameNum, 0);     //VLD
                    VP9_RegDump (MC_REG_OFFSET0, 0, 864, u4FrameNum, 1);        //MC
                    VP9_RegDump (VP9_VLD_REG_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                    VP9_RegDump (VP9_VLD_REG_OFFSET0, 41, 124, u4FrameNum, 1);  //VP9 VLD
                    VP9_RegDump (HEVC_MISC_REG_OFFSET0, 0, 128, u4FrameNum, 1); //VLD_TOP
                    VP9_RegDump (HEVC_PP_REG_OFFSET0, 0, 896, u4FrameNum, 1);   //PP
                    VP9_RegDump (HEVC_MV_REG_OFFSET0, 0, 254, u4FrameNum, 1);   //MV

                    if(prStatus->u4DualCore)
                    {
                        VP9_RegDump (VLD_TOP_REG_OFFSET1, 0, 128, u4FrameNum, 1);   //VLD_TOP
                        VP9_RegDump (VLD_REG_OFFSET1, 33, 255,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (VDEC_BS2_OFFSET1, 33, 255,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (MC_REG_OFFSET1, 0, 864, u4FrameNum, 1);        //MC
                        VP9_RegDump (VP9_VLD_REG_OFFSET1, 0, 0, u4FrameNum, 1);     //Input Window
                        VP9_RegDump (VP9_VLD_REG_OFFSET1, 41, 124, u4FrameNum, 1);  //VP9 VLD
                        VP9_RegDump (HEVC_MISC_REG_OFFSET1, 0, 128, u4FrameNum, 1); //VLD_TOP
                        VP9_RegDump (HEVC_PP_REG_OFFSET1, 0, 896, u4FrameNum, 1);   //PP
                        VP9_RegDump (HEVC_MV_REG_OFFSET1, 0, 254, u4FrameNum, 1);   //MV

                        VP9_RegDump (LAE_VLDTOP_OFFSET0, 0, 128,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (LAE_VLD_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (LAE_BS2_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (LAE_VP9_VLD_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                        VP9_RegDump (LAE_VP9_VLD_OFFSET0, 41, 124, u4FrameNum, 1);  //VP9 VLD
                        VP9_RegDump (LAE_MV_OFFSET0, 0, 254, u4FrameNum, 1);         //MV
                        VP9_RegDump (MVDEC_TOP_OFFSET0, 0, 127, u4FrameNum, 1);   //MCORE
                    }
                    // RRF calculate
                    VP9_RRF_Calculate_Dram_Size(u4InstID, &PIC_SIZE_Y, &prFBInfo->u4UFO_LEN_SIZE_Y,
                                                &prFBInfo->u4UFO_LEN_SIZE_C,  u4CoreId);
                    // care ufo
                    VP9DumpMem( u4InstID, u4CurrentYAddr, PIC_SIZE_Y , u4FrameNum, 1, 0 );
                    VP9DumpMem( u4InstID, u4CurrentCAddr, PIC_SIZE_Y  >> 1, u4FrameNum, 2, 0 );
                }

                u4IsFail = 1;

                #if SIM_LOG
                u4IsFail = 0;
                #else
                if ( u4IsFail )
                {
                    // RRF calculate
                    #if VP9_CRC_ENABLE

                    VP9_RRF_Calculate_Dram_Size(u4InstID, &PIC_SIZE_Y, &prFBInfo->u4UFO_LEN_SIZE_Y,
                                                &prFBInfo->u4UFO_LEN_SIZE_C,  u4CoreId);

                    VP9_COMPARE_COUNT_TBL(u4InstID, u4FrameNum, u4CoreId);

                    u4IsFail = VP9_CRC_Check(u4InstID, u4FrameNum, u4CoreId);

                    #else

                    VP9_RRF_Calculate_Dram_Size(u4InstID, &PIC_SIZE_Y, &prFBInfo->u4UFO_LEN_SIZE_Y,
                                                &prFBInfo->u4UFO_LEN_SIZE_C,  u4CoreId);

                    u4IsFail = VP9GoldenComparison(u4InstID, u4FrameNum, PIC_SIZE_Y, u4CurrentYAddr, u4CurrentCAddr, 1,
                                                   prFBInfo->VP9_FBM_YLEN_Addr[prFBInfo->u4CurrentFBId],
                                                   prFBInfo->VP9_FBM_CLEN_Addr[prFBInfo->u4CurrentFBId],
                                                   prFBInfo->u4UFO_LEN_SIZE_Y, prFBInfo->u4UFO_LEN_SIZE_C);

                    VP9_COMPARE_COUNT_TBL(u4InstID, u4FrameNum, u4CoreId);
                    #endif
                }
                #endif
                if(u4IsFail)
                {
                    prStatus->u4BistreamResult = DECODE_MISMATCH;
                    printk("@@ Golden Compare Failed  @%u!!! \n", u4FrameNum);
                   if(prStatus->u4DecodeTimeOut == 0)
                   {
                        VP9_RegDump (VLD_TOP_REG_OFFSET0, 0, 128, u4FrameNum, 1);   //VLD_TOP
                        VP9_RegDump (VLD_REG_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (VDEC_BS2_OFFSET0, 33, 255,  u4FrameNum, 0);     //VLD
                        VP9_RegDump (MC_REG_OFFSET0, 0, 864, u4FrameNum, 1);        //MC
                        VP9_RegDump (VP9_VLD_REG_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                        VP9_RegDump (VP9_VLD_REG_OFFSET0, 41, 124, u4FrameNum, 1);  //VP9 VLD
                        VP9_RegDump (HEVC_MISC_REG_OFFSET0, 0, 128, u4FrameNum, 1); //VLD_TOP
                        VP9_RegDump (HEVC_PP_REG_OFFSET0, 0, 896, u4FrameNum, 1);   //PP
                        VP9_RegDump (HEVC_MV_REG_OFFSET0, 0, 254, u4FrameNum, 1);   //MV

                        if(prStatus->u4DualCore)
                        {
                            VP9_RegDump (VLD_TOP_REG_OFFSET1, 0, 128, u4FrameNum, 1);   //VLD_TOP
                            VP9_RegDump (VLD_REG_OFFSET1, 33, 255,  u4FrameNum, 1);     //VLD
                            VP9_RegDump (VDEC_BS2_OFFSET1, 33, 255,  u4FrameNum, 1);     //VLD
                            VP9_RegDump (MC_REG_OFFSET1, 0, 864, u4FrameNum, 1);        //MC
                            VP9_RegDump (VP9_VLD_REG_OFFSET1, 0, 0, u4FrameNum, 1);     //Input Window
                            VP9_RegDump (VP9_VLD_REG_OFFSET1, 41, 124, u4FrameNum, 1);  //VP9 VLD
                            VP9_RegDump (HEVC_MISC_REG_OFFSET1, 0, 128, u4FrameNum, 1); //VLD_TOP
                            VP9_RegDump (HEVC_PP_REG_OFFSET1, 0, 896, u4FrameNum, 1);   //PP
                            VP9_RegDump (HEVC_MV_REG_OFFSET1, 0, 254, u4FrameNum, 1);   //MV

                            VP9_RegDump (LAE_VLD_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                            VP9_RegDump (LAE_BS2_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                            VP9_RegDump (LAE_VP9_VLD_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                            VP9_RegDump (LAE_VP9_VLD_OFFSET0, 41, 124, u4FrameNum, 1);  //VP9 VLD
                            VP9_RegDump (LAE_MV_OFFSET0, 0, 254, u4FrameNum, 1);         //MV

                            VP9_RegDump (MVDEC_TOP_OFFSET0, 0, 127, u4FrameNum, 1);   //MCORE
                        }
                   }
                }
                else
                {
                    #if VP9_PERFORMANCE_LOG
                    VP9_Performance_Log(u4InstID, u4FrameNum, u4CoreId);
                    VP9_MC_Prefetch_Log(u4InstID, u4FrameNum, u4CoreId);
                    #endif
                    #if 0
                        VP9_RegDump (VLD_TOP_REG_OFFSET0, 0, 128, u4FrameNum, 1);   //VLD_TOP
                        VP9_RegDump (VLD_REG_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                        VP9_RegDump (MC_REG_OFFSET0, 0, 864, u4FrameNum, 1);        //MC
                        VP9_RegDump (VP9_VLD_REG_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                        VP9_RegDump (VP9_VLD_REG_OFFSET0, 41, 105, u4FrameNum, 1);  //VP9 VLD
                        VP9_RegDump (HEVC_MISC_REG_OFFSET0, 0, 128, u4FrameNum, 1); //VLD_TOP
                        VP9_RegDump (HEVC_PP_REG_OFFSET0, 0, 896, u4FrameNum, 1);   //PP
                    #endif
                    printk("@@%u Pass \n", u4FrameNum);
                }
                // Reset Mcore
                if(prStatus->u4DualCore)
                {
                    printk("Reset Dual Core!\n",__FUNCTION__);
                    RISCWrite_MCore_TOP(24, 0, u4CoreId);
                    RISCWrite_MCore_TOP(7, 0x1, u4CoreId);
                    RISCWrite_MCore_TOP(7, 0x0, u4CoreId);
                }
            }
            else  if(strcmp(ucRiscType,"VDEC_TRIG")== 0 && u4Skip == 2) // LAE Decode error
            {
                printk("[VP9] LAE Decode Error....!!!!!!\n");

                prStatus->u4BistreamResult = DECODE_LAE_TIMEOUT;

                VP9_RegDump (VLD_TOP_REG_OFFSET0, 0, 128, u4FrameNum, 1);   //VLD_TOP
                VP9_RegDump (VLD_REG_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                VP9_RegDump (VDEC_BS2_OFFSET0, 33, 255,  u4FrameNum, 0);     //VLD
                VP9_RegDump (MC_REG_OFFSET0, 0, 864, u4FrameNum, 1);        //MC
                VP9_RegDump (VP9_VLD_REG_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                VP9_RegDump (VP9_VLD_REG_OFFSET0, 41, 124, u4FrameNum, 1);  //VP9 VLD
                VP9_RegDump (HEVC_MISC_REG_OFFSET0, 0, 128, u4FrameNum, 1); //VLD_TOP
                VP9_RegDump (HEVC_PP_REG_OFFSET0, 0, 896, u4FrameNum, 1);   //PP
                VP9_RegDump (HEVC_MV_REG_OFFSET0, 0, 254, u4FrameNum, 1);   //MV

                // redundant condition
                if(prStatus->u4DualCore)
                {
                    VP9_RegDump (VLD_TOP_REG_OFFSET1, 0, 128, u4FrameNum, 1);   //VLD_TOP
                    VP9_RegDump (VLD_REG_OFFSET1, 33, 255,  u4FrameNum, 1);     //VLD
                    VP9_RegDump (VDEC_BS2_OFFSET1, 33, 255,  u4FrameNum, 1);     //VLD
                    VP9_RegDump (MC_REG_OFFSET1, 0, 864, u4FrameNum, 1);        //MC
                    VP9_RegDump (VP9_VLD_REG_OFFSET1, 0, 0, u4FrameNum, 1);     //Input Window
                    VP9_RegDump (VP9_VLD_REG_OFFSET1, 41, 124, u4FrameNum, 1);  //VP9 VLD
                    VP9_RegDump (HEVC_MISC_REG_OFFSET1, 0, 128, u4FrameNum, 1); //VLD_TOP
                    VP9_RegDump (HEVC_PP_REG_OFFSET1, 0, 896, u4FrameNum, 1);   //PP
                    VP9_RegDump (HEVC_MV_REG_OFFSET1, 0, 254, u4FrameNum, 1);   //MV

                    VP9_RegDump (LAE_VLDTOP_OFFSET0, 0, 128,  u4FrameNum, 1);     //LAE_VLDTOP
                    VP9_RegDump (LAE_VLD_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                    VP9_RegDump (LAE_BS2_OFFSET0, 33, 255,  u4FrameNum, 1);     //VLD
                    VP9_RegDump (LAE_VP9_VLD_OFFSET0, 0, 0, u4FrameNum, 1);     //Input Window
                    VP9_RegDump (LAE_VP9_VLD_OFFSET0, 41, 124, u4FrameNum, 1);  //VP9 VLD
                    VP9_RegDump (LAE_MV_OFFSET0, 0, 254, u4FrameNum, 1);         //MV

                    VP9_RegDump (MVDEC_TOP_OFFSET0, 0, 127, u4FrameNum, 1);   //MCORE
                 }

                // Reset Mcore
                if(prStatus->u4DualCore)
                {
                    printk("[LAE] Reset Dual Core!\n",__FUNCTION__);
                    RISCWrite_MCore_TOP(24, 0, u4CoreId);
                    RISCWrite_MCore_TOP(7, 0x1, u4CoreId);
                    RISCWrite_MCore_TOP(7, 0x0, u4CoreId);
                }
            }

            //buffer shift line
            while( (*(pucRiscBuff + u4PatOffset) != 0x0D) && (*(pucRiscBuff + u4PatOffset + 1) != 0x0A) )
            {
                u4PatOffset ++;
            }
            // skip 2 carriage return chars
            u4PatOffset += 2;
        }

        if(prStatus->u4BistreamResult != DECODE_OK)
        {
            break;
        }
        //vfree(buff_risc);
    }

    if(prStatus->u4BistreamResult == DECODE_TIMEOUT)
    {
        printk("\n@@ VP9 Decode NG, Decode Timeout @%d!!  [%s] \n",  u4FrameNum, bitstream_name);
    }
    else if(prStatus->u4BistreamResult == DECODE_MISMATCH)
    {
        printk("\n@@ VP9 Decode NG, Golden Mismatch @%d!!  [%s] \n",  u4FrameNum, bitstream_name);
    }
    else if(prStatus->u4BistreamResult == DECODE_MISSING_DATA)
    {
        printk("\n@@ VP9 Decode NG, Missing RISC_SETTING files!!  [%s] \n", bitstream_name);
    }
    else if(prStatus->u4BistreamResult == DECODE_LAE_TIMEOUT)
    {
        printk("\n@@ VP9 Decode NG, LAE Decode Timeout @%d!!  [%s] \n", u4FrameNum, bitstream_name);
    }
    else
    {
        printk("\n@@ VP9 Decode Completed!!  [%s] \n", bitstream_name);
    }

    return;
}

#endif

BOOL fgGoldenCmp(UINT32 u4DecBuf,UINT32 u4GoldenBuf,UINT32 u4Size)
{
    UINT32 i;
    UINT32 *pSrc,*pDes;
    pSrc = (UINT32 *)u4DecBuf;
    pDes = (UINT32 *)u4GoldenBuf;
    //width 64 align,height 32 align
    for(i = 0; i < u4Size/4; i++)
    {
        if(*pSrc != *pDes)
        {
            printk("Decbuf 0x%x [0x%x] != GoldBuf 0x%x [0x%x] offset %d\n",u4DecBuf,*pSrc,u4GoldenBuf,*pDes,i);
            return 1;
        }
        pSrc ++;
        pDes ++;
    }

    return 0;
}

