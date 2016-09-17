#ifndef _VDEC_HW_H264_H_
#define _VDEC_HW_H264_H_
//#include "typedef.h"
//#include "vdec_info_common.h"



// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************
extern void vWriteAVCGconReg(UINT32 dAddr, UINT32 dVal);
extern UINT32 u4ReadAVCGconReg(UINT32 dAddr);
extern void vVDecWriteAVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCCOMVLD(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCVLD(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCMC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCMC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCMV(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCPP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCMISC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCMISC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCVLDTOP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCUFO(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCUFO(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCCRC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCCRC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCFG(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCFG(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCBS2(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCBS2(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCMCORE_TOP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecAVCSetCoreState(UCHAR u4VDecID, UINT32 u4CoreID);
extern void vVDecAVCResetHW(UINT32 u4VDecID, UINT32 u4VDecType);
extern UINT32 u4VDecAVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID);
extern UINT32 u4VDecAVCVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 dShiftBit);
extern BOOL fgInitH264BarrelShift1(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm);
extern BOOL fgInitH264BarrelShift2(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm);
extern void vInitFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm);
extern UINT32 u4VDecReadH264VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa);
extern void vVDecAVCSetVLDVFIFO(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFifoSa, UINT32 u4VFifoEa);
extern void vVDecAVCPowerDownHW(UINT32 u4VDecID);


#endif

