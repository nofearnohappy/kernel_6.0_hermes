#ifndef _VDEC_HW_H265_H_
#define _VDEC_HW_H265_H_
//#include "typedef.h"
//#include "vdec_info_common.h"


enum u4RegBaseSeqID
{
    HEVC_COM_VLD,
    HEVC_VLD_TOP,
    HEVC_MC,
    HEVC_MV,
    HEVC_PP,
    HEVC_MISC,
    HEVC_BS2,
    HEVC_VLD,
    HEVC_GCON,
    HEVC_MCORE,
    HEVC_UFO_ENC
};


// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************

extern void vWriteHEVCGconReg(UINT32 dAddr, UINT32 dVal);
extern UINT32 u4ReadHEVCGconReg(UINT32 dAddr);
extern void vVDecWriteHEVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCVLD(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCMC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCMC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCMV(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCPP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCMISC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCMISC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCBS2(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCBS2(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteHEVCMCORE_UFO_ENC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadHEVCMCORE_UFO_ENC(UINT32 u4VDecID, UINT32 u4Addr);

extern void vVDecHEVCSetCoreState(UCHAR u4VDecID, UINT32 u4CoreID);
extern void vVDecHEVCResetHW(UINT32 u4VDecID, UINT32 u4VDecType);
extern UINT32 u4VDecHEVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID);
extern UINT32 u4VDecHEVCVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 dShiftBit);
extern UINT32 u4VDECHEVCInitSearchStartCode(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBit);
extern BOOL fgInitH265BarrelShift(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H265_BS_INIT_PRM_T *prH265BSInitPrm);
extern void vInitHEVCFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H265_INIT_PRM_T *prH265VDecInitPrm);
extern UINT32 u4VDecReadH265VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa);
#endif

