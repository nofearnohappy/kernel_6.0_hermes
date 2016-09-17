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
#ifndef _VDEC_INFO_VP9_H_
#define _VDEC_INFO_VP9_H_

#include "drv_config.h"

#if(CONFIG_DRV_VERIFY_SUPPORT)
#include "vdec_usage.h"
#include "vdec_info_common.h"
#else
//#include "x_stl_lib.h"
//#include "x_assert.h"
//#include "u_pbinf.h"
#endif

// vp9 internal used.
#define VP9_CONFIG_CHIP_VER_MT5890 8590 // Oryx
#define VP9_CONFIG_CHIP_VER_MT5891 8591 // WuKong 10 bits
#define VP9_CONFIG_CHIP_VER_CURR VP9_CONFIG_CHIP_VER_MT5890




#define PIC_Y_BUFFER      0
#define PIC_C_BUFFER      1
#define LAE_BUFFER        2
#define ERR_BUFFER        3
#define TILE_BUFFER       4
#define COUNT_TBL_BUFFER  5
#define UFO_Y_LEN		  6
#define UFO_C_LEN		  7
#define SEGID_0_BUFFER    8
#define SEGID_1_BUFFER    9



#define BITSTREAM_LENGTH  256
#define VP9_LAE_ID        8
#define CORE_0_ID 0
#define CORE_1_ID 1

#define VP9_PRINT_ERROR(x, ...)  printk("[VP9][ERROR]!! "x" \n", ##__VA_ARGS__)
#define VP9_PRINT_INFO(x, ...)   printk("[VP9][INFO] --> "x" \n", ##__VA_ARGS__)

#define VP9_PRINT_VERBOSE(x, ...) \
    do {                           \
        if(_u4VP9LogOption == 1 || _u4VP9LogOption == 3)     \
             printk("[VP9][VERBOSE] "x" \n", ##__VA_ARGS__);  \
    }while(0)

#define CHECK_ERROR(x)     \
{                            \
    if(x != VP9_ERR_NONE)    \
    {                        \
        printk("[VP9][ERROR] errno: 0x%x\n", x); \
        VP9_PRINT_INFO("@@ VP9 Decode Failed [%s]",_bFileStr1[0][1]); \
        _u4VerBitCount[u4InstID] = 0xffffffff;  \
        return;                                   \
    }                                              \
}


// 0/1 ---> disable/enable verbose and sim log
extern UINT32 _u4VP9LogOption;

#define VP9_IVF_FILE_HEADER_SZ      0x20
#define VP9_IVF_FRAME_HEADER_SZ     0x0C

// Enable MCore flow or not, if enabled, decoder will check resolution to
// determine Mcore flow or Score flow to decode each frame.
// turn it on if IC support Mcore, otherwise, please do NOT set turn this on.
#if ((VP9_CONFIG_CHIP_VER_CURR == VP9_CONFIG_CHIP_VER_MT5890) || (VP9_CONFIG_CHIP_VER_CURR == VP9_CONFIG_CHIP_VER_MT5891))
#define VP9_ENABLE_MCORE     0
#else
#define VP9_ENABLE_MCORE     0
#endif

#define CORE0_FLOW 1    // determine core 0 or core 1 flow

#define VP9_V_FIFO_SZ        V_FIFO_SZ
#define VP9_SEG_ID_SZ       (0x12000)
//#define VP9_DPB_SZ           0x6900000
#define VP9_DPB_SZ           0xB400000              // for 4k
#define VP9_GOLD_Y_SZ        0x1FE000               //(1920*1088)
//#define VP9_GOLD_Y_SZ        0xB00000
#define VP9_GOLD_C_SZ        (VP9_GOLD_Y_SZ >> 1)
#define VP9_LAE_BUFFER_SZ   ((64 * 4 * 16)* 8 * 64)
#define VP9_ERR_BUFFER_SZ   (288 << 4)
#define VP9_CRC_BUFFER_SZ    0x80000                // 512k
#define VP9_TILE_BUFFER_SIZE 0x10000                //64k, actually (64 x 16 x 4) is enough
#define VP9_COUNT_TBL_SZ     0x4000//at least (256 * 128 * 4)         //16384 bytes
#define VP9_PROB_TBL_SZ     0xF00// at least (2560)

#define MAX_PROB 255
#define REFS_PER_FRAME 3

#define REF_FRAMES_LOG2 3
#define REF_FRAMES (1 << REF_FRAMES_LOG2)
#define MAX_FRAMES (8+2)
#define MAX_VP9_MV_BUF (1 + 1)   // for Core 0/1 & LAE

// 1 scratch frame for the new frame, 3 for scaled references on the encoder
// TODO(jkoleszar): These 3 extra references could probably come from the
// normal reference pool.
#define FRAME_BUFFERS (REF_FRAMES + 4)
#define FRAME_CONTEXTS_LOG2 2
#define FRAME_CONTEXTS (1 << FRAME_CONTEXTS_LOG2)


#define VP9_SYNC_CODE_0 0x49
#define VP9_SYNC_CODE_1 0x83
#define VP9_SYNC_CODE_2 0x42
#define VP9_FRAME_MARKER 0x2
#define CONFIG_VP9_HIGH 1
#define CONFIG_HIGH_QUANT 1

#define IGNORE_RESERVED(x) \
  if (u4VDEC_HAL_VP9_Read_Bit_Raw(x)) \
  {                                   \
      VP9_PRINT_ERROR("Reserved bit must be unset"); \
      return VP9_FAIL; \
  }


typedef enum {
    VP9_ERR_NONE = 0,
    VP9_ERR_STREAM_CORRUPT = 0x80000000,
    VP9_ERR_STREAM_UNSUP,
    VP9_ERR_STREAM_EOF,
    VP9_ERR_MEM_ERROR,
    VP9_ERR_UNKNOW_ERROR
}VP9_ERROR_TYPE;

typedef enum {
    VP9_OK = 0,
    VP9_FAIL,
    VP9_SKIP_FRAME
}VP9_RETURN_TYPE;

// Bitstream profiles indicated by 2-3 bits in the uncompressed header.
// 00: Profile 0.  8-bit 4:2:0 only.
// 10: Profile 1.  8-bit 4:4:4, 4:2:2, and 4:4:0.
// 01: Profile 2.  10-bit and 12-bit color only, with 4:2:0 sampling.
// 110: Profile 3. 10-bit and 12-bit color only, with 4:2:2/4:4:4/4:4:0
//                 sampling.
// 111: Undefined profile.
typedef enum BITSTREAM_PROFILE {
  PROFILE_0,
  PROFILE_1,
  PROFILE_2,
//#if (VP9_CONFIG_CHIP_VER_CURR > VP9_CONFIG_CHIP_VER_MT5890)
  PROFILE_3,
//#endif
  MAX_PROFILES
} BITSTREAM_PROFILE;

typedef enum BIT_DEPTH {
  BITS_8,
  BITS_10,
  BITS_12
} BIT_DEPTH;

typedef enum PARTITION_TYPE {
  PARTITION_NONE,
  PARTITION_HORZ,
  PARTITION_VERT,
  PARTITION_SPLIT,
  PARTITION_TYPES,
  PARTITION_INVALID = PARTITION_TYPES
} PARTITION_TYPE;

#define PARTITION_PLOFFSET   4  // number of probability models per block size
#define PARTITION_CONTEXTS (4 * PARTITION_PLOFFSET)

// block transform size
typedef enum {
  TX_4X4 = 0,                      // 4x4 transform
  TX_8X8 = 1,                      // 8x8 transform
  TX_16X16 = 2,                    // 16x16 transform
  TX_32X32 = 3,                    // 32x32 transform
  TX_SIZES
} TX_SIZE;

// frame transform mode
typedef enum {
  ONLY_4X4            = 0,        // only 4x4 transform used
  ALLOW_8X8           = 1,        // allow block transform size up to 8x8
  ALLOW_16X16         = 2,        // allow block transform size up to 16x16
  ALLOW_32X32         = 3,        // allow block transform size up to 32x32
  TX_MODE_SELECT      = 4,        // transform specified for each block
  TX_MODES            = 5,
} TX_MODE;

typedef enum {
  DCT_DCT   = 0,                      // DCT  in both horizontal and vertical
  ADST_DCT  = 1,                      // ADST in vertical, DCT in horizontal
  DCT_ADST  = 2,                      // DCT  in vertical, ADST in horizontal
  ADST_ADST = 3,                      // ADST in both directions
  TX_TYPES = 4
} TX_TYPE;

typedef enum {
    UNKNOWN    = 0,
    BT_601     = 1,  // YUV
    BT_709     = 2,  // YUV
    SMPTE_170  = 3,  // YUV
    SMPTE_240  = 4,  // YUV
    RESERVED_1 = 5,
    RESERVED_2 = 6,
    SRGB       = 7   // RGB
} COLOR_SPACE;


typedef enum {
    KEY_FRAME = 0,
    INTER_FRAME = 1,
    FRAME_TYPES,
} FRAME_TYPE;

typedef enum {
  DC_PRED,         // Average of above and left pixels
  V_PRED,          // Vertical
  H_PRED,          // Horizontal
  D45_PRED,        // Directional 45  deg = round(arctan(1/1) * 180/pi)
  D135_PRED,       // Directional 135 deg = 180 - 45
  D117_PRED,       // Directional 117 deg = 180 - 63
  D153_PRED,       // Directional 153 deg = 180 - 27
  D207_PRED,       // Directional 207 deg = 180 + 27
  D63_PRED,        // Directional 63  deg = round(arctan(2/1) * 180/pi)
  TM_PRED,         // True-motion
  NEARESTMV,
  NEARMV,
  ZEROMV,
  NEWMV,
  MB_MODE_COUNT
} PREDICTION_MODE;

typedef enum {
    NO_FRAME = -1,
    INTRA_FRAME = 0,
    LAST_FRAME = 1,
    GOLDEN_FRAME = 2,
    ALTREF_FRAME = 3,
    MAX_REF_FRAMES = 4
} MV_REFERENCE_FRAME;

typedef enum {
  PLANE_TYPE_Y  = 0,
  PLANE_TYPE_UV = 1,
  PLANE_TYPES
} PLANE_TYPE;

typedef enum {
  EIGHTTAP = 0,
  EIGHTTAP_SMOOTH = 1,
  EIGHTTAP_SHARP = 2,
  BILINEAR = 3,
  SWITCHABLE = 4  /* should be the last one */
} INTERP_FILTER;


/* Symbols for coding which components are zero jointly */
#define MV_JOINTS     4
typedef enum {
  MV_JOINT_ZERO = 0,             /* Zero vector */
  MV_JOINT_HNZVZ = 1,            /* Vert zero, hor nonzero */
  MV_JOINT_HZVNZ = 2,            /* Hor zero, vert nonzero */
  MV_JOINT_HNZVNZ = 3,           /* Both components nonzero */
} MV_JOINT_TYPE;

/* Symbols for coding magnitude class of nonzero components */
#define MV_CLASSES     11
typedef enum {
  MV_CLASS_0 = 0,      /* (0, 2]     integer pel */
  MV_CLASS_1 = 1,      /* (2, 4]     integer pel */
  MV_CLASS_2 = 2,      /* (4, 8]     integer pel */
  MV_CLASS_3 = 3,      /* (8, 16]    integer pel */
  MV_CLASS_4 = 4,      /* (16, 32]   integer pel */
  MV_CLASS_5 = 5,      /* (32, 64]   integer pel */
  MV_CLASS_6 = 6,      /* (64, 128]  integer pel */
  MV_CLASS_7 = 7,      /* (128, 256] integer pel */
  MV_CLASS_8 = 8,      /* (256, 512] integer pel */
  MV_CLASS_9 = 9,      /* (512, 1024] integer pel */
  MV_CLASS_10 = 10,    /* (1024,2048] integer pel */
} MV_CLASS_TYPE;

#define DIFF_UPDATE_PROB 252

// Coefficient token alphabet
#define ZERO_TOKEN      0   // 0     Extra Bits 0+0
#define ONE_TOKEN       1   // 1     Extra Bits 0+1
#define TWO_TOKEN       2   // 2     Extra Bits 0+1
#define THREE_TOKEN     3   // 3     Extra Bits 0+1
#define FOUR_TOKEN      4   // 4     Extra Bits 0+1
#define CATEGORY1_TOKEN 5   // 5-6   Extra Bits 1+1
#define CATEGORY2_TOKEN 6   // 7-10  Extra Bits 2+1
#define CATEGORY3_TOKEN 7   // 11-18 Extra Bits 3+1
#define CATEGORY4_TOKEN 8   // 19-34 Extra Bits 4+1
#define CATEGORY5_TOKEN 9   // 35-66 Extra Bits 5+1
#define CATEGORY6_TOKEN 10  // 67+   Extra Bits 14+1
#define EOB_TOKEN       11  // EOB   Extra Bits 0+0

#define ENTROPY_TOKENS 12

#define ENTROPY_NODES 11


#define CLASS0_BITS    1  /* bits at integer precision for class 0 */
#define CLASS0_SIZE    (1 << CLASS0_BITS)
#define MV_OFFSET_BITS (MV_CLASSES + CLASS0_BITS - 2)
#define MV_FP_SIZE 4

#define MV_MAX_BITS    (MV_CLASSES + CLASS0_BITS + 2)
#define MV_MAX         ((1 << MV_MAX_BITS) - 1)
#define MV_VALS        ((MV_MAX << 1) + 1)

#define MV_IN_USE_BITS 14
#define MV_UPP   ((1 << MV_IN_USE_BITS) - 1)
#define MV_LOW   (-(1 << MV_IN_USE_BITS))

/* Coefficients are predicted via a 3-dimensional probability table. */

#define REF_TYPES 2  // intra=0, inter=1

/* Middle dimension reflects the coefficient position within the transform. */
#define COEF_BANDS 6

/* Inside dimension is measure of nearby complexity, that reflects the energy
   of nearby coefficients are nonzero.  For the first coefficient (DC, unless
   block type is 0), we look at the (already encoded) blocks above and to the
   left of the current block.  The context index is then the number (0,1,or 2)
   of these blocks having nonzero coefficients.
   After decoding a coefficient, the measure is determined by the size of the
   most recently decoded coefficient.
   Note that the intuitive meaning of this measure changes as coefficients
   are decoded, e.g., prior to the first token, a zero means that my neighbors
   are empty while, after the first token, because of the use of end-of-block,
   a zero means we just decoded a zero and hence guarantees that a non-zero
   coefficient will appear later in this block.  However, this shift
   in meaning is perfectly OK because our context depends also on the
   coefficient band (and since zigzag positions 0, 1, and 2 are in
   distinct bands). */

#define COEFF_CONTEXTS 6
#define UNCONSTRAINED_NODES         3

#define BLOCK_SIZE_GROUPS 4
#define SKIP_CONTEXTS 3
#define INTER_MODE_CONTEXTS 7

/* Segment Feature Masks */
#define MAX_MV_REF_CANDIDATES 2


#define MINQ 0
#define MAXQ 255
#define QINDEX_RANGE (MAXQ - MINQ + 1)
#define QINDEX_BITS 8

#define MAX_LOOP_FILTER 63
#define MAX_SHARPNESS 7
#define RRF_NEW_FORMULA 1

#define SIMD_WIDTH 16

#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      2

#define INTRA_INTER_CONTEXTS 4
#define COMP_INTER_CONTEXTS 5
#define REF_CONTEXTS 5
#define INTRA_MODES (TM_PRED + 1)

#define INTER_MODES (1 + NEWMV - NEARESTMV)

#define TX_SIZE_CONTEXTS 2
#define SWITCHABLE_FILTERS 3   // number of switchable filters
#define SWITCHABLE_FILTER_CONTEXTS (SWITCHABLE_FILTERS + 1)


#define TREE_SIZE(leaf_count) (2 * (leaf_count) - 2)
#define INTER_OFFSET(mode) ((mode) - NEARESTMV)

typedef UCHAR vp9_prob;
typedef INT8  vp9_tree_index;

typedef vp9_prob vp9_coeff_probs_model[REF_TYPES][COEF_BANDS]
                                      [COEFF_CONTEXTS][UNCONSTRAINED_NODES];

typedef UINT32 vp9_coeff_count_model[REF_TYPES][COEF_BANDS]
                                          [COEFF_CONTEXTS]
                                          [UNCONSTRAINED_NODES + 1];


typedef struct {
  vp9_prob sign;
  vp9_prob classes[MV_CLASSES - 1];
  vp9_prob class0[CLASS0_SIZE - 1];
  vp9_prob bits[MV_OFFSET_BITS];
  vp9_prob class0_fp[CLASS0_SIZE][MV_FP_SIZE - 1];
  vp9_prob fp[MV_FP_SIZE - 1];
  vp9_prob class0_hp;
  vp9_prob hp;
} nmv_component;

typedef struct {
  vp9_prob joints[MV_JOINTS - 1];
  nmv_component comps[2];
} nmv_context;


typedef struct {
  unsigned int sign[2];
  unsigned int classes[MV_CLASSES];
  unsigned int class0[CLASS0_SIZE];
  unsigned int bits[MV_OFFSET_BITS][2];
  unsigned int class0_fp[CLASS0_SIZE][MV_FP_SIZE];
  unsigned int fp[MV_FP_SIZE];
  unsigned int class0_hp[2];
  unsigned int hp[2];
} nmv_component_counts;

typedef struct {
  unsigned int joints[MV_JOINTS];
  nmv_component_counts comps[2];
} nmv_context_counts;

struct tx_probs {
  vp9_prob p32x32[TX_SIZE_CONTEXTS][TX_SIZES - 1];
  vp9_prob p16x16[TX_SIZE_CONTEXTS][TX_SIZES - 2];
  vp9_prob p8x8[TX_SIZE_CONTEXTS][TX_SIZES - 3];
};

struct tx_counts {
  unsigned int p32x32[TX_SIZE_CONTEXTS][TX_SIZES];
  unsigned int p16x16[TX_SIZE_CONTEXTS][TX_SIZES - 1];
  unsigned int p8x8[TX_SIZE_CONTEXTS][TX_SIZES - 2];
};

typedef struct frame_contexts {
  vp9_prob y_mode_prob[BLOCK_SIZE_GROUPS][INTRA_MODES - 1];
  vp9_prob uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];
  vp9_prob partition_prob[PARTITION_CONTEXTS][PARTITION_TYPES - 1];
  vp9_coeff_probs_model coef_probs[TX_SIZES][PLANE_TYPES];
  vp9_prob switchable_interp_prob[SWITCHABLE_FILTER_CONTEXTS]
                                 [SWITCHABLE_FILTERS - 1];
  vp9_prob inter_mode_probs[INTER_MODE_CONTEXTS][INTER_MODES - 1];
  vp9_prob intra_inter_prob[INTRA_INTER_CONTEXTS];
  vp9_prob comp_inter_prob[COMP_INTER_CONTEXTS];
  vp9_prob single_ref_prob[REF_CONTEXTS][2];
  vp9_prob comp_ref_prob[REF_CONTEXTS];
  struct tx_probs tx_probs;
  vp9_prob skip_probs[SKIP_CONTEXTS];
  nmv_context nmvc;
} FRAME_CONTEXT;

typedef struct {
  UINT32 y_mode[BLOCK_SIZE_GROUPS][INTRA_MODES];
  UINT32 uv_mode[INTRA_MODES][INTRA_MODES];
  UINT32 partition[PARTITION_CONTEXTS][PARTITION_TYPES];
  vp9_coeff_count_model coef[TX_SIZES][PLANE_TYPES];
  UINT32 eob_branch[TX_SIZES][PLANE_TYPES][REF_TYPES]
                         [COEF_BANDS][COEFF_CONTEXTS];
  UINT32 switchable_interp[SWITCHABLE_FILTER_CONTEXTS]
                                [SWITCHABLE_FILTERS];
  UINT32 inter_mode[INTER_MODE_CONTEXTS][INTER_MODES];
  UINT32 intra_inter[INTRA_INTER_CONTEXTS][2];
  UINT32 comp_inter[COMP_INTER_CONTEXTS][2];
  UINT32 single_ref[REF_CONTEXTS][2][2];
  UINT32 comp_ref[REF_CONTEXTS][2];
  struct tx_counts tx;
  UINT32 skip[SKIP_CONTEXTS][2];
  nmv_context_counts mv;
} FRAME_COUNTS;

#define MI_SIZE_LOG2 3
#define MI_BLOCK_SIZE_LOG2 (6 - MI_SIZE_LOG2)  // 64 = 2^6

#define MI_SIZE (1 << MI_SIZE_LOG2)  // pixels per mi-unit
#define MI_BLOCK_SIZE (1 << MI_BLOCK_SIZE_LOG2)  // mi-units per max block

#define MI_MASK (MI_BLOCK_SIZE - 1)


#define SEGMENT_DELTADATA   0
#define SEGMENT_ABSDATA     1

#define MAX_SEGMENTS     8
#define SEG_TREE_PROBS   (MAX_SEGMENTS-1)

#define PREDICTION_PROBS 3

#define REF_SCALE_SHIFT 14
#define REF_NO_SCALE (1 << REF_SCALE_SHIFT)
#define REF_INVALID_SCALE -1
#define REF_INVALID_STEP -1

// Segment level features.
typedef enum {
  SEG_LVL_ALT_Q = 0,               // Use alternate Quantizer ....
  SEG_LVL_ALT_LF = 1,              // Use alternate loop filter value...
  SEG_LVL_REF_FRAME = 2,           // Optional Segment reference frame
  SEG_LVL_SKIP = 3,                // Optional Segment (0,0) + skip mode
  SEG_LVL_MAX = 4                  // Number of features supported
} SEG_LVL_FEATURES;


typedef struct  {
  UINT8 enabled;
  UINT8 update_map;
  UINT8 update_data;
  UINT8 abs_delta;
  UINT8 temporal_update;
  UINT32 u4SegCtr;                 //  VP9 segment abs_delta (seg->abs_delta),  VP9_segment enable 0 ~ 7 (feature_enabled)
  UINT32 u4SegFeature_0_3;         //  VP9 segment loopfilter feature data 0 (seg 0) ~ data 3,(seg 3)
  UINT32 u4SegFeature_4_7;         //  VP9 segment loopfilter feature data 4 (seg 4) ~ data 7,(seg 7)

  vp9_prob tree_probs[SEG_TREE_PROBS];
  vp9_prob pred_probs[PREDICTION_PROBS];

  INT16 feature_data[MAX_SEGMENTS][SEG_LVL_MAX];
  UINT32 feature_mask[MAX_SEGMENTS];
}SEGMENTATION;

typedef struct {
  INT32 i4X_scale_fp;   // horizontal fixed point scale factor
  INT32 i4Y_scale_fp;   // vertical fixed point scale factor
  INT32 i4X_step_q4;
  INT32 i4Y_step_q4;
  UINT32 u4Ref_Scaling_EN;
#if 0
  int (*scale_value_x)(int val, const struct scale_factors *sf);
  int (*scale_value_y)(int val, const struct scale_factors *sf);
  convolve_fn_t predict[2][2][2];  // horiz, vert, avg
#endif
}VP9_Scale_Factors_T;

typedef struct _VP9_SUPER_FRAME_INFO_T_
{
    BOOL   fgInSuperFrame;
    UINT32 u4SuperFrmSizes[8];
    UINT32 u4SuperFrmCount;
    UINT32 u4SuperFrmIndex;
}VP9_SUPER_FRAME_INFO_T;

typedef struct _VP9_DRAM_BUF_T_
{
    UINT32 u4BufVAddr;
    UINT32 u4BufSize;
}VP9_DRAM_BUF_T;

typedef struct _VP9_INPUT_CTX_T_
{
    //const char *filename;
    //off_t length;
    //struct FileTypeDetectionBuffer detect;
    //enum VideoFileType file_type;
    UINT32 u4VaFifoStart;                 ///< Video Fifo memory start address
    UINT32 u4VaFifoEnd;                 ///< Video Fifo memory end address
    UINT32 u4FileOffset;
    UINT32 u4FileLength;
    UINT32 u4BitstreamLoadingCnt;
    CHAR   ucBitStreamName[256];
    UINT32 u4VaFrameStart;
    UINT32 u4VaFrameEnd;
    UINT32 u4FrameSize;
    UINT32 u4UnCompressSize;
    VP9_SUPER_FRAME_INFO_T rSuperFrame;

    UINT32 u4Width;
    UINT32 u4Height;
    //UINT8 use_i420;
    //UINT8 only_i420;
    UINT32 u4Fourcc;
}VP9_INPUT_CTX_T;

typedef struct _VP9_FB_INFO_T_
{
    VP9_DRAM_BUF_T rBufY;
    VP9_DRAM_BUF_T rBufC;
    VP9_DRAM_BUF_T rUFO_LEN_Y;
    VP9_DRAM_BUF_T rUFO_LEN_C;
    UCHAR    u1InUse;
    UINT32   u4YWidth;
    UINT32   u4YHeight;
    UINT32   u4YCropWidth;
    UINT32   u4YCropHeight;
    UINT32   u4YStride;

    UINT32   u4CWidth;
    UINT32   u4CHeight;
    UINT32   u4CCropWidth;
    UINT32   u4CCropHeight;
    UINT32   u4CStride;
    // UFO

    UINT32   u4DramPicY_Y_LENSize;
    UINT32   u4DramPicC_C_LENSize;
}VP9_FB_INFO_T;

typedef enum {
  SINGLE_REFERENCE      = 0,
  COMPOUND_REFERENCE    = 1,
  REFERENCE_MODE_SELECT = 2,
  REFERENCE_MODES       = 3,
} REFERENCE_MODE;

typedef struct _VP9_REF_CNT_BUF_T_
{
    VP9_FB_INFO_T rBuf;
  //UINT32   u4Idx;
    UINT32   u4RefCount;
}VP9_REF_CNT_BUF_T;

typedef struct _VP9_REF_BUF_T_
{
    VP9_FB_INFO_T* prBuf;
    VP9_Scale_Factors_T rScaleFactors;
    UINT32   u4Idx;//index of FRAME_BUFS[MAX_FRAMES]
}VP9_REF_BUF_T;


typedef struct macroblockd {
//	  struct macroblockd_plane plane[MAX_MB_PLANE];

  int mode_info_stride;

  // A NULL indicates that the 8x8 is not part of the image
//	  MODE_INFO **mi_8x8;
//	  MODE_INFO **prev_mi_8x8;
//	  MODE_INFO *mi_stream;

  int up_available;
  int left_available;

  /* Distance of MB away from frame edges */
  int mb_to_left_edge;
  int mb_to_right_edge;
  int mb_to_top_edge;
  int mb_to_bottom_edge;

  /* pointers to reference frames */
  VP9_REF_BUF_T *block_refs[2];

  /* pointer to current frame */
//	  const YV12_BUFFER_CONFIG *cur_buf;

  /* mc buffer */
//	  DECLARE_ALIGNED(16, uint8_t, mc_buf[80 * 2 * 80 * 2]);

  int lossless;
  /* Inverse transform function pointers. */
//	  void (*itxm_add)(const int16_t *input, uint8_t *dest, int stride, int eob);

//	  const InterpKernel *interp_kernel;

  int corrupted;

  /* Y,U,V,(A) */
//	  ENTROPY_CONTEXT *above_context[MAX_MB_PLANE];
//	  ENTROPY_CONTEXT left_context[MAX_MB_PLANE][16];

//	  PARTITION_CONTEXT *above_seg_context;
//	  PARTITION_CONTEXT left_seg_context[8];
} MACROBLOCKD;

typedef struct {
  INT32 filter_level;

  INT32 sharpness_level;
  INT32 last_sharpness_level;

  UINT8 mode_ref_delta_enabled;
  UINT8 mode_ref_delta_update;

  // 0 = Intra, Last, GF, ARF
  INT8 ref_deltas[MAX_REF_LF_DELTAS];
  INT8 last_ref_deltas[MAX_REF_LF_DELTAS];

  // 0 = ZERO_MV, MV
  INT8 mode_deltas[MAX_MODE_LF_DELTAS];
  INT8 last_mode_deltas[MAX_MODE_LF_DELTAS];
}VP9_LOOP_FILTER_INFO_T;

typedef struct _VP9_UNCOMPRESSED_HEADER_T_
{
    //uncompressed header
    UINT32 u4Profile;
    FRAME_TYPE u4FrameType;
    UINT32 u4ShowFrame;
    UINT32 u4ShowExisting;
    UINT32 u4Width;
    UINT32 u4Height;
    UINT32 u4ColorSpace;
    UINT32 u4BitDepth;
    UINT32 u4SubSampling_X;
    UINT32 u4SubSampling_Y;
    UINT32 u4ErrResilenceMode;
    UINT32 u4RefreshFrameFlags;
    // Flag signaling that the frame is encoded using only INTRA modes.
    UINT32 u4IntraOnly;
    // Flag signaling that the frame context should be reset to default values.
    // 0 or 1 implies don't reset, 2 reset just the context specified in the
    // frame header, 3 reset all contexts.
    UINT32 u4ResetFrameContext;
    UINT32 u4FrameParallelDecodingMode;
    UINT32 u4RefreshFrameContext;
    UINT32 u4FrameContextIdx;
    UINT32 u4AllowHighPrecisionMv;
    INTERP_FILTER eInterpFilterType;
    UINT32 u4BaseQIdx;
    UINT32 u4Y_DC_DELTA_Q;
    UINT32 u4C_DC_DELTA_Q;
    UINT32 u4C_AC_DELTA_Q;
    UINT32 u4FirstPartitionSize;
    SEGMENTATION seg;
    INT32 u4MbCols;
    UINT32 u4MiCols;
    INT32 u4Log2TileCols;
    INT32 u4Log2TileRows;
    UINT32 u4Lossless;
    BOOL fgUse_Prev_MI;
    VP9_LOOP_FILTER_INFO_T rLoopFilter;
}VP9_UNCOMPRESSED_HEADER_T;

typedef struct _VP9_TILE_INFO_T_ {
  INT32 i4MiRowStart, i4MiRowEnd;
  INT32 i4MiColStart, i4MiColEnd;
} VP9_TILE_INFO_T;

typedef struct TileBuffer {
    UCHAR *data;
    UINT32 size;
    INT32 col;  // only used with multi-threaded decoding
} TileBuffer;



typedef struct _VP9_COMMON_T_
{
    UINT32 au4Y_Dequant[QINDEX_RANGE][8];
    UINT32 au4UV_Dequant[QINDEX_RANGE][8];
    UINT32 au4DeQuant[MAX_SEGMENTS][4];

    VP9_INPUT_CTX_T rInputCtx;
    //VP9_FRAME_CTX_T rFrameCtx;
    //VP9_FB_INFO_T rFrameInfo;
    //BOOL fgFramePoolInited;

    VP9_REF_CNT_BUF_T* pCurrentFB;
    VP9_REF_CNT_BUF_T FRAME_BUFS[FRAME_BUFFERS];//should not be changed except reallocate
    INT32 REF_FRAME_MAP[REF_FRAMES]; /* maps fb_idx to reference slot */

    // Each frame can reference REFS_PER_FRAME buffers
    VP9_REF_BUF_T FRAME_REFS[REFS_PER_FRAME];
    UINT32 REF_FRAME_SIGN_BIAS[MAX_REF_FRAMES];
    MV_REFERENCE_FRAME COMP_FIXED_REF;
    MV_REFERENCE_FRAME COMP_VAR_REF[2];

    UINT32 u4DisplayWidth;
    UINT32 u4DiaplayHeight;
    UINT32 u4LastWidth;
    UINT32 u4LastHeight;
    UINT32 u4LastShowFrame;
    VP9_FB_INFO_T* prFrameToShow;
    UINT32 u4InstID;
    UINT32 u4FrameNum;
    UINT32 u4StartNum;
    UINT32 u4EndNum;
    UINT32 u4TotalDecoded;
    UINT32 u4NewFbIdx;
    UINT32 u4DecodeResult;
    UINT32 u4Use_High;

    // MBs, mb_rows/cols is in 16-pixel units; mi_rows/cols is in
    // MODE_INFO (8-pixel) units.
    UINT32 u4MBs;
    UINT32 u4MB_rows, u4MI_rows;
    UINT32 u4MB_cols, u4MI_cols;
    UINT32 u4MI_stride;

    BOOL fgFrameBufferConfiged;
    BOOL fgMultiCoreEnable;
    BOOL fgPixelCompare;
    BOOL fgUFOModeEnable;
    BOOL fgCRCOpen;

    VP9_ERROR_TYPE eErrno;
    FRAME_TYPE u4LastFrameType;

    VP9_UNCOMPRESSED_HEADER_T rUnCompressedHeader;

    MACROBLOCKD rMBD;
    TX_MODE eTxMode;
    REFERENCE_MODE eRefMode;

    UINT32 u4KeyFrameDecoded;

    FRAME_CONTEXT fc;  /* this frame entropy */
    FRAME_CONTEXT frame_contexts[FRAME_CONTEXTS];
    FRAME_COUNTS counts;
    TileBuffer TILE_INFOS[4][64];

//Dram Buffer
    VP9_DRAM_BUF_T rDramSegId0;
    VP9_DRAM_BUF_T rDramSegId1;
    VP9_DRAM_BUF_T rDramDpb;
    VP9_DRAM_BUF_T rMVBuffer[MAX_VP9_MV_BUF];
    VP9_DRAM_BUF_T rTileBuffer;
    VP9_DRAM_BUF_T rCountTBLBuffer;
    VP9_DRAM_BUF_T rProbTBLBuffer;

    VP9_DRAM_BUF_T rDramCRCYBuf0;     // Mcore y0
    VP9_DRAM_BUF_T rDramCRCCBuf0;     // Mcore c0
    VP9_DRAM_BUF_T rDramCRCYBuf1;     // Mcore y1
    VP9_DRAM_BUF_T rDramCRCCBuf1;     // Mcore c1

    VP9_DRAM_BUF_T rDramCRCYBuf2;     // Score y0
    VP9_DRAM_BUF_T rDramCRCCBuf2;     // Score c0
    VP9_DRAM_BUF_T rDramGOLDENYbuf;
	VP9_DRAM_BUF_T rDramGOLDENCbuf;
//MCore Buffer
    VP9_DRAM_BUF_T rLAEBuffer;
    VP9_DRAM_BUF_T rErrorBuffer;

    UCHAR ucBitstreamName[256];
}VP9_COMMON_T;

#endif //#ifndef _VDEC_INFO_VP8H_

