#ifndef _EXTERNAL_FACTORY_H_
#define _EXTERNAL_FACTORY_H_

#include "hdmi_drv.h"

typedef int DPI_I32;

typedef short int DPI_I16;
typedef unsigned int DPI_U32;
typedef bool DPI_BOOL;
typedef void *pDPI;

typedef enum {
	STEP1_CHIP_INIT,
	STEP2_JUDGE_CALLBACK,
	STEP3_START_DPI_AND_CONFIG,
	STEP4_DPI_STOP_AND_POWER_OFF,
	STEP_FACTORY_MAX_NUM
} HDMI_FACTORY_TEST;

typedef struct {
	DPI_I32 hdmi_width;	/* DPI read buffer width */
	DPI_I32 hdmi_height;	/* DPI read buffer height */
	DPI_I32 bg_width;	/* DPI read buffer width */
	DPI_I32 bg_height;	/* DPI read buffer height */
	HDMI_VIDEO_RESOLUTION output_video_resolution;
	DPI_I32 scaling_factor;
} DPI_PARAM_CONTEXT;

int hdmi_factory_mode_test(HDMI_FACTORY_TEST test_step, void *info);
#endif
