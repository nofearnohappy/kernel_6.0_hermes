#ifndef _VDEC_VERIFY_VPARSER_VP9_H_
#define _VDEC_VERIFY_VPARSER_VP9_H_

//#include "x_typedef.h"
#include "../include/vdec_info_vp9.h"

#define VP9_CRC_COMPARE 0
#define VP9_UFO_MODE 0
#define VP9_BW_FAKE_ENGINE 0
#define VP9_DDR4 0
void vVerInitVP9(UINT32 u4InstID);
void vVerVParserVP9(UINT32 u4InstID, BOOL fgInquiry);
void vVerDecodeVP9(UINT32 u4InstID);
void vVerVP9DecEnd(UINT32 u4InstID);
BOOL fgVP9IntraOnly(VP9_COMMON_T * prCommon);
void vVP9DumpMem(UCHAR* buf, UINT32 size ,UINT32 frame_num ,UINT32 u4Type);

#endif // _PR_EMU_H_

