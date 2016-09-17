#ifndef _EPD_DRV_H_
#define _EPD_DRV_H_
#include "lcm_drv.h"


typedef enum {
	EPD_POLARITY_RISING = 0,
	EPD_POLARITY_FALLING = 1
} HDMI_POLARITY;

typedef struct {
	unsigned int PLL_CLOCK;

	unsigned int width;
	unsigned int height;

	/* polarity parameters */
	LCM_POLARITY clk_pol;
	LCM_POLARITY de_pol;
	LCM_POLARITY vsync_pol;
	LCM_POLARITY hsync_pol;

	LCM_DPI_FORMAT format;
	LCM_COLOR_ORDER rgb_order;

	unsigned int i2x_en;
	unsigned int i2x_edge;
	unsigned int embsync;
	unsigned int bit_swap;
	unsigned int pannel_frq;
	/* timing parameters */
	unsigned int hsync_pulse_width;
	unsigned int hsync_back_porch;
	unsigned int hsync_front_porch;
	unsigned int vsync_pulse_width;
	unsigned int vsync_back_porch;
	unsigned int vsync_front_porch;
} LCM_EPD_PARAMS;

typedef struct {
	void (*init) ();
	void (*get_params) (LCM_EPD_PARAMS *params);
	void (*gen_pattern_frame) (unsigned long pattern);
	void (*get_screen_size) (unsigned int *pWidth, unsigned int *pHeight);
	void (*power_on) (void);
	void (*power_off) (void);

} EPD_DRIVER;


/* --------------------------------------------------------------------------- */
/* HDMI Driver Functions */
/* --------------------------------------------------------------------------- */

const EPD_DRIVER *EPD_GetDriver(void);

#endif
