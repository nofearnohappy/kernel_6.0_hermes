#ifndef _DW9761BAF_H
#define _DW9761BAF_H

#include <linux/ioctl.h>

#define DW9761BAF_MAGIC 'A'


typedef struct {
unsigned int u4CurrentPosition;
unsigned int u4MacroPosition;
unsigned int u4InfPosition;
bool          bIsMotorMoving;
bool          bIsMotorOpen;
bool          bIsSupportSR;
} stDW9761BAF_MotorInfo;


#ifdef LensdrvCM3
typedef struct {
	
	float Aperture;
	
	float FilterDensity;
	
	float FocalLength;
	
	float FocalDistance;
	
	u16 u4OIS_Mode;
	
	u16 Facing;
	
	float OpticalAxisAng[2];
	
	float Position[3];
	
	float FocusRange;
	
	u16 State;
	
	float InfoAvalibleMinFocusDistance;
	float InfoAvalibleApertures;
	float InfoAvalibleFilterDensity;
	u16 InfoAvalibleOptStabilization;
	float InfoAvalibleFocalLength;
	float InfoAvalibleHypeDistance;
}stDW9761BAF_MotorMETAInfo;
#endif

#define DW9761BAFIOC_G_MOTORINFO _IOR(DW9761BAF_MAGIC,0,stDW9761BAF_MotorInfo)

#define DW9761BAFIOC_T_MOVETO _IOW(DW9761BAF_MAGIC,1,unsigned int)

#define DW9761BAFIOC_T_SETINFPOS _IOW(DW9761BAF_MAGIC,2,unsigned int)

#define DW9761BAFIOC_T_SETMACROPOS _IOW(DW9761BAF_MAGIC,3,unsigned int)
#ifdef LensdrvCM3
#define DW9761BAFIOC_G_MOTORMETAINFO _IOR(DW9761BAF_MAGIC,4,stDW9761BAF_MotorMETAInfo)
#endif

#else
#endif
