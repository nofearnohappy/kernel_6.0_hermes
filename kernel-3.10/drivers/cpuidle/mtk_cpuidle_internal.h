#ifndef _MTK_CPUIDLE_INTERNAL_H_
#define _MTK_CPUIDLE_INTERNAL_H_

enum idle_lock_spm_id{
    IDLE_SPM_LOCK_VCORE_DVFS= 0,
};

enum {
    IDLE_TYPE_DP = 0,
    IDLE_TYPE_SO = 1,
    IDLE_TYPE_SL = 2,
    IDLE_TYPE_RG = 3,
    NR_TYPES     = 4,
};

enum {
    BY_CPU       = 0,
    BY_CLK       = 1,
    BY_TMR       = 2,
    BY_OTH       = 3,
    BY_VTG       = 4,
    NR_REASONS   = 5
};

enum {
	CG_INFRA   = 0,
	CG_PERI    = 1,
	CG_DISP0   = 2,
	CG_DISP1   = 3,
	CG_IMAGE   = 4,
	CG_MFG     = 5,
	CG_AUDIO   = 6,
	CG_VDEC0   = 7,
	CG_VDEC1   = 8,
	CG_VENC    = 9,
	NR_GRPS    = 10,
};

extern unsigned int     dpidle_condition_mask[NR_GRPS];
extern unsigned int     dpidle_block_mask[NR_GRPS];
extern unsigned int     dpidle_time_critera;
extern unsigned int     dpidle_block_time_critera;//default 30sec
extern bool             dpidle_by_pass_cg;

extern unsigned int     soidle_condition_mask[NR_GRPS];
extern unsigned int     soidle_block_mask[NR_GRPS];
extern unsigned int     soidle_time_critera;
extern unsigned int     soidle_block_time_critera;//default 30sec
extern bool             soidle_by_pass_cg;

extern bool spm_get_sodi_en(void);

extern unsigned int slidle_condition_mask[NR_GRPS];
extern unsigned int slidle_block_mask[NR_GRPS];

extern const char *reason_name[NR_REASONS];

int __init mtk_cpuidle_internal_init(void);
bool cg_check_idle_can_enter(
	unsigned int *condition_mask, unsigned int *block_mask);

void idle_cnt_inc(int idle_type, int cpu);
unsigned long idle_cnt_get(int idle_type, int cpu);

void idle_block_cnt_inc(int idle_type, int reason);
unsigned long idle_block_cnt_get(int idle_type, int reason);
void idle_block_cnt_clr(int idle_type);

int idle_switch_get(int idle_type);
const char *cg_grp_get_name(int id);
#endif
