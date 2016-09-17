#include <asm/uaccess.h>
#include <linux/cpu.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mach/mt_typedefs.h>

#include "mtk_cpuidle_internal.h"

#define IDLE_TAG     "[Power/swap]"
#define idle_err(fmt, args...)		pr_err(IDLE_TAG fmt, ##args)
#define idle_warn(fmt, args...)		pr_warn(IDLE_TAG fmt, ##args)
#define idle_info(fmt, args...)		pr_notice(IDLE_TAG fmt, ##args)
#define idle_dbg(fmt, args...)		pr_info(IDLE_TAG fmt, ##args)
/* #define idle_ver(fmt, args...)  pr_debug(IDLE_TAG fmt, ##args) */
#define idle_ver(fmt, args...)		pr_info(IDLE_TAG fmt, ##args)	/* pr_debug show nothing */

/*Idle handler on/off*/
static int idle_switch[NR_TYPES] = {
	1,  /* dpidle switch */
	1,  /* soidle switch */
	1,  /* slidle switch */
	1,  /* rgidle switch */
};

static const char *idle_name[NR_TYPES] = {
    "dpidle",
    "soidle",
    "slidle",
    "rgidle",
};

const char *reason_name[NR_REASONS] = {
    "by_cpu",
    "by_clk",
    "by_tmr",
    "by_oth",
    "by_vtg",
};

#define INVALID_GRP_ID(grp) (grp < 0 || grp >= NR_GRPS)

unsigned int dpidle_condition_mask[NR_GRPS] = {
    0x0000008A, //INFRA:
//    0x37FA1FFD, //PERI0:
//  TODO: check uart1 CG on D-1 EVB ??
    0x37F21FFD, //PERI0:
    0x000FFFFF, //DISP0:
    0x0000003F, //DISP1:
    0x00000FE1, //IMAGE:
    0x00000001, //MFG:
    0x00000000, //AUDIO:
    0x00000001, //VDEC0:
    0x00000001, //VDEC1:
    0x00001111, //VENC:
};

unsigned int soidle_condition_mask[NR_GRPS] = {
    0x00000088, //INFRA:
    0x37FA0FFC, //PERI0:
    0x000033FC, //DISP0:
    0x00000030, //DISP1:
    0x00000FE1, //IMAGE:
    0x00000001, //MFG:
    0x00000000, //AUDIO:
    0x00000001, //VDEC0:
    0x00000001, //VDEC1:
    0x00001111, //VENC:
};

unsigned int slidle_condition_mask[NR_GRPS] = {
    0x00000000, //INFRA:
    0x07C01000, //PERI0:
    0x00000000, //DISP0:
    0x00000000, //DISP1:
    0x00000000, //IMAGE:
    0x00000000, //MFG:
    0x00000000, //AUDIO:
    0x00000000, //VDEC0:
    0x00000000, //VDEC1:
    0x00000000, //VENC:
};

char cg_group_name[][NR_GRPS] = {
	"INFRA",
	"PERI",
	"DISP0",
	"DISP1",
	"IMAGE",
	"MFG",
	"AUDIO",
	"VDEC0",
	"VDEC1",
	"VENC",
};

static unsigned long    dpidle_cnt[NR_CPUS] = {0};
static unsigned long    dpidle_block_cnt[NR_REASONS] = {0};
unsigned int            dpidle_block_mask[NR_GRPS] = {0x0};
unsigned int            dpidle_time_critera = 26000;
unsigned int            dpidle_block_time_critera = 30000;//default 30sec
bool                    dpidle_by_pass_cg = false;

static unsigned long    soidle_cnt[NR_CPUS] = {0};
static unsigned long    soidle_block_cnt[NR_REASONS] = {{0}};
unsigned int     		soidle_block_mask[NR_GRPS] = {0x0};
unsigned int     		soidle_time_critera = 26000; 
unsigned int     		soidle_block_time_critera = 30000;//default 30sec
bool                    soidle_by_pass_cg = false;

/* Slow Idle */
unsigned int slidle_block_mask[NR_GRPS] = {0x0};
static unsigned long slidle_block_cnt[NR_REASONS] = {0};
static unsigned long slidle_cnt[NR_CPUS] = {0};

static unsigned long    rgidle_cnt[NR_CPUS] = {0};

void idle_cnt_inc(int idle_type, int cpu)
{
    switch (idle_type) {
    case IDLE_TYPE_DP:
        dpidle_cnt[cpu]++;
        break;
    case IDLE_TYPE_SO:
        soidle_cnt[cpu]++;
        break;
    case IDLE_TYPE_SL:
        slidle_cnt[cpu]++;
        break;
    case IDLE_TYPE_RG:
        rgidle_cnt[cpu]++;
        break;
    default:
        break;
    }
}
EXPORT_SYMBOL(idle_cnt_inc);

unsigned long idle_cnt_get(int idle_type, int cpu)
{
    unsigned long   ret = 0;

    switch (idle_type) {
    case IDLE_TYPE_DP:
        ret = dpidle_cnt[cpu];
        break;
    case IDLE_TYPE_SO:
        ret = soidle_cnt[cpu];
        break;
    case IDLE_TYPE_SL:
        ret = slidle_cnt[cpu];
        break;
    case IDLE_TYPE_RG:
        ret = rgidle_cnt[cpu];
        break;
    default:
        break;
    }

    return ret;
}
EXPORT_SYMBOL(idle_cnt_get);

void idle_block_cnt_inc(int idle_type, int reason)
{
    switch (idle_type) {
    case IDLE_TYPE_DP:
        dpidle_block_cnt[reason]++;
        break;
    case IDLE_TYPE_SO:
        soidle_block_cnt[reason]++;
        break;
    case IDLE_TYPE_SL:
        slidle_block_cnt[reason]++;
        break;
    case IDLE_TYPE_RG:
        /* TODO */
        break;
    default:
        break;
    }
}
EXPORT_SYMBOL(idle_block_cnt_inc);

unsigned long idle_block_cnt_get(int idle_type, int reason)
{
    unsigned long   ret = 0;

    switch (idle_type) {
    case IDLE_TYPE_DP:
        ret = dpidle_block_cnt[reason];
        break;
    case IDLE_TYPE_SO:
        ret = soidle_block_cnt[reason];
        break;
    case IDLE_TYPE_SL:
        ret = slidle_block_cnt[reason];
        break;
    case IDLE_TYPE_RG:
        /* TODO */
        break;
    default:
        break;
    }

    return ret;
}
EXPORT_SYMBOL(idle_block_cnt_get);

void idle_block_cnt_clr(int idle_type)
{
    switch (idle_type) {
    case IDLE_TYPE_DP:
        memset(dpidle_block_cnt, 0, sizeof(dpidle_block_cnt));
        break;
    case IDLE_TYPE_SO:
        memset(soidle_block_cnt, 0, sizeof(soidle_block_cnt));
        break;
    case IDLE_TYPE_SL:
        memset(slidle_block_cnt, 0, sizeof(slidle_block_cnt));
        break;
    case IDLE_TYPE_RG:
        /* TODO */
        break;
    default:
        break;
    }
}
EXPORT_SYMBOL(idle_block_cnt_clr);

int idle_switch_get(int idle_type)
{
    return (idle_type >= 0 && idle_type < NR_TYPES) ?
                idle_switch[idle_type] :
                0;
}
EXPORT_SYMBOL(idle_switch_get);

static DEFINE_MUTEX(dpidle_locked);

static void enable_dpidle_by_mask(int grp, unsigned int mask)
{
    mutex_lock(&dpidle_locked);
    dpidle_condition_mask[grp] &= ~mask;
    mutex_unlock(&dpidle_locked);
}

static void disable_dpidle_by_mask(int grp, unsigned int mask)
{
    mutex_lock(&dpidle_locked);
    dpidle_condition_mask[grp] |= mask;
    mutex_unlock(&dpidle_locked);
}

void enable_dpidle_by_bit(int id)
{
    int grp = id / 32;
    unsigned int mask = 1U << (id % 32);
    BUG_ON(INVALID_GRP_ID(grp));
    enable_dpidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_dpidle_by_bit);

void disable_dpidle_by_bit(int id)
{
    int grp = id / 32;
    unsigned int mask = 1U << (id % 32);
    BUG_ON(INVALID_GRP_ID(grp));
    disable_dpidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_dpidle_by_bit);

/************************************************
 * SODI part
 ************************************************/
static DEFINE_MUTEX(soidle_locked);

static void enable_soidle_by_mask(int grp, unsigned int mask)
{
    mutex_lock(&soidle_locked);
    soidle_condition_mask[grp] &= ~mask;
    mutex_unlock(&soidle_locked);
}

static void disable_soidle_by_mask(int grp, unsigned int mask)
{
    mutex_lock(&soidle_locked);
    soidle_condition_mask[grp] |= mask;
    mutex_unlock(&soidle_locked);
}

void enable_soidle_by_bit(int id)
{
    int grp = id / 32;
    unsigned int mask = 1U << (id % 32);
    BUG_ON(INVALID_GRP_ID(grp));
    enable_soidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_soidle_by_bit);

void disable_soidle_by_bit(int id)
{
    int grp = id / 32;
    unsigned int mask = 1U << (id % 32);
    BUG_ON(INVALID_GRP_ID(grp));
    disable_soidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_soidle_by_bit);

/************************************************
 * slow idle part
 ************************************************/
static DEFINE_MUTEX(slidle_locked);

static void enable_slidle_by_mask(int grp, unsigned int mask)
{
    mutex_lock(&slidle_locked);
    slidle_condition_mask[grp] &= ~mask;
    mutex_unlock(&slidle_locked);
}

static void disable_slidle_by_mask(int grp, unsigned int mask)
{
    mutex_lock(&slidle_locked);
    slidle_condition_mask[grp] |= mask;
    mutex_unlock(&slidle_locked);
}

void enable_slidle_by_bit(int id)
{
    int grp = id / 32;
    unsigned int mask = 1U << (id % 32);
    BUG_ON(INVALID_GRP_ID(grp));
    enable_slidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(enable_slidle_by_bit);

void disable_slidle_by_bit(int id)
{
    int grp = id / 32;
    unsigned int mask = 1U << (id % 32);
    BUG_ON(INVALID_GRP_ID(grp));
    disable_slidle_by_mask(grp, mask);
}
EXPORT_SYMBOL(disable_slidle_by_bit);

/***************************/
/* debugfs                 */
/***************************/
static char dbg_buf[2048] = {0};
static char cmd_buf[512] = {0};

/* idle_state */
static int _idle_state_open(struct seq_file *s, void *data)
{
    return 0;
}

static int idle_state_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, _idle_state_open, inode->i_private);
}

static ssize_t idle_state_read(struct file *filp, 
                                 char __user *userbuf, 
                                 size_t count, 
                                 loff_t *f_pos)
{
    int len = 0;
    char *p = dbg_buf;
    int i;

    p += sprintf(p, "********** idle state dump **********\n");

    for (i = 0; i < nr_cpu_ids; i++) {
        p += sprintf(p, "soidle_cnt[%d]=%lu, dpidle_cnt[%d]=%lu, "
                "slidle_cnt[%d]=%lu, rgidle_cnt[%d]=%lu\n",
                i, soidle_cnt[i], i, dpidle_cnt[i],
                i, slidle_cnt[i], i, rgidle_cnt[i]);
    }

    p += sprintf(p, "\n********** variables dump **********\n");
    for (i = 0; i < NR_TYPES; i++) {
        p += sprintf(p, "%s_switch=%d, ", idle_name[i], idle_switch[i]);
    }
    p += sprintf(p, "\n");

    p += sprintf(p, "\n********** idle command help **********\n");
    p += sprintf(p, "status help:   cat /sys/kernel/debug/cpuidle/idle_state\n");
    p += sprintf(p, "switch on/off: echo switch mask > /sys/kernel/debug/cpuidle/idle_state\n");

    p += sprintf(p, "soidle help:   cat /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "dpidle help:   cat /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "slidle help:   cat /sys/kernel/debug/cpuidle/slidle_state\n");
    p += sprintf(p, "rgidle help:   cat /sys/kernel/debug/cpuidle/rgidle_state\n");

    len = p - dbg_buf;

    return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t idle_state_write(struct file *filp, 
                                  const char __user *userbuf, 
                                  size_t count, 
                                  loff_t *f_pos)
{
    char cmd[32];
    int idx;
    int param;

    count = min(count, sizeof(cmd_buf) - 1);

    if (copy_from_user(cmd_buf, userbuf, count)) {
        return -EFAULT;
    }
    cmd_buf[count] = '\0';

    if (sscanf(cmd_buf, "%s %x", cmd, &param) == 2) {
        if (!strcmp(cmd, "switch")) {
            for (idx = 0; idx < NR_TYPES; idx++) {
                idle_switch[idx] = (param & (1U << idx)) ? 1 : 0;
            }
        }
        return count;
    }

    return -EINVAL;
}

static const struct file_operations idle_state_fops = {
    .open = idle_state_open,
    .read = idle_state_read,
    .write = idle_state_write,
    .llseek = seq_lseek,
    .release = single_release,
};
/* dpidle_state */
static int _dpidle_state_open(struct seq_file *s, void *data)
{
    return 0;
}

static int dpidle_state_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, _dpidle_state_open, inode->i_private);
}

static ssize_t dpidle_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
    int len = 0;
    char *p = dbg_buf;
    int i;
    ssize_t retval = 0;

    p += sprintf(p, "*********** deep idle state ************\n");
    p += sprintf(p, "dpidle_time_critera=%u\n", dpidle_time_critera);

    for (i = 0; i < NR_REASONS; i++) {
        p += sprintf(p, "[%d]dpidle_block_cnt[%s]=%lu\n", i, reason_name[i],
                dpidle_block_cnt[i]);
    }

    p += sprintf(p, "\n");

    for (i = 0; i < NR_GRPS; i++) {
        p += sprintf(p, "[%02d]dpidle_condition_mask[%-8s]=0x%08x\t\t"
                "dpidle_block_mask[%-8s]=0x%08x\n", i,
				cg_grp_get_name(i), dpidle_condition_mask[i],
				cg_grp_get_name(i), dpidle_block_mask[i]);
    }

    p += sprintf(p, "dpidle_bypass_cg=%u\n", dpidle_by_pass_cg);

    p += sprintf(p, "\n*********** dpidle command help  ************\n");
    p += sprintf(p, "dpidle help:   cat /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "switch on/off: echo [dpidle] 1/0 > /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "cpupdn on/off: echo cpupdn 1/0 > /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "en_dp_by_bit:  echo enable id > /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "dis_dp_by_bit: echo disable id > /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/dpidle_state\n");
    p += sprintf(p, "bypass cg:     echo bypass 1/0 > /sys/kernel/debug/cpuidle/dpidle_state\n");

    len = p - dbg_buf;

    return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t dpidle_state_write(struct file *filp, 
                                  const char __user *userbuf, 
                                  size_t count, 
                                  loff_t *f_pos)
{
    char cmd[32];
    int param;

    count = min(count, sizeof(cmd_buf) - 1);

    if (copy_from_user(cmd_buf, userbuf, count)) {
        return -EFAULT;
    }
    cmd_buf[count] = '\0';

    if (sscanf(cmd_buf, "%s %d", cmd, &param) == 2) {
        if (!strcmp(cmd, "dpidle")) {
            idle_switch[IDLE_TYPE_DP] = param;
        } else if (!strcmp(cmd, "enable")) {
            enable_dpidle_by_bit(param);
        } else if (!strcmp(cmd, "disable")) {
            disable_dpidle_by_bit(param);
        } else if (!strcmp(cmd, "time")) {
            dpidle_time_critera = param;
        }else if (!strcmp(cmd, "bypass")) {
            dpidle_by_pass_cg = param;
            printk(KERN_WARNING"bypass = %d\n", dpidle_by_pass_cg);
        }
        return count;
    } else if (sscanf(cmd_buf, "%d", &param) == 1) {
        idle_switch[IDLE_TYPE_DP] = param;
        return count;
    }

    return -EINVAL;
}

static const struct file_operations dpidle_state_fops = {
    .open = dpidle_state_open,
    .read = dpidle_state_read,
    .write = dpidle_state_write,
    .llseek = seq_lseek,
    .release = single_release,
};

/* soidle_state */
static int _soidle_state_open(struct seq_file *s, void *data)
{
    return 0;
}

static int soidle_state_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, _soidle_state_open, inode->i_private);
}

static ssize_t soidle_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
    int len = 0;
    char *p = dbg_buf;
    int i;
    ssize_t retval = 0;

    p += sprintf(p, "*********** deep idle state ************\n");
    p += sprintf(p, "soidle_time_critera=%u\n", soidle_time_critera);

    for (i = 0; i < NR_REASONS; i++) {
        p += sprintf(p, "[%d]soidle_block_cnt[%s]=%lu\n", i, reason_name[i],
                soidle_block_cnt[i]);
    }

    p += sprintf(p, "\n");

    for (i = 0; i < NR_GRPS; i++) {
        p += sprintf(p, "[%02d]soidle_condition_mask[%-8s]=0x%08x\t\t"
                "soidle_block_mask[%-8s]=0x%08x\n", i,
				cg_grp_get_name(i), soidle_condition_mask[i],
				cg_grp_get_name(i), soidle_block_mask[i]);
    }

    p += sprintf(p, "soidle_bypass_cg=%u\n", soidle_by_pass_cg);

    p += sprintf(p, "\n*********** soidle command help  ************\n");
    p += sprintf(p, "soidle help:   cat /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "switch on/off: echo [soidle] 1/0 > /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "cpupdn on/off: echo cpupdn 1/0 > /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "en_dp_by_bit:  echo enable id > /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "dis_dp_by_bit: echo disable id > /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "modify tm_cri: echo time value(dec) > /sys/kernel/debug/cpuidle/soidle_state\n");
    p += sprintf(p, "bypass cg:     echo bypass 1/0 > /sys/kernel/debug/cpuidle/soidle_state\n");

    len = p - dbg_buf;

    return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t soidle_state_write(struct file *filp, 
                                  const char __user *userbuf, 
                                  size_t count, 
                                  loff_t *f_pos)
{
    char cmd[32];
    int param;

    count = min(count, sizeof(cmd_buf) - 1);

    if (copy_from_user(cmd_buf, userbuf, count)) {
        return -EFAULT;
    }
    cmd_buf[count] = '\0';

    if (sscanf(cmd_buf, "%s %d", cmd, &param) == 2) {
        if (!strcmp(cmd, "soidle")) {
            idle_switch[IDLE_TYPE_DP] = param;
        } else if (!strcmp(cmd, "enable")) {
            enable_soidle_by_bit(param);
        } else if (!strcmp(cmd, "disable")) {
            disable_soidle_by_bit(param);
        } else if (!strcmp(cmd, "time")) {
            soidle_time_critera = param;
        }else if (!strcmp(cmd, "bypass")) {
            soidle_by_pass_cg = param;
            printk(KERN_WARNING"bypass = %d\n", soidle_by_pass_cg);
        }
        return count;
    } else if (sscanf(cmd_buf, "%d", &param) == 1) {
        idle_switch[IDLE_TYPE_DP] = param;
        return count;
    }

    return -EINVAL;
}

static const struct file_operations soidle_state_fops = {
    .open = soidle_state_open,
    .read = soidle_state_read,
    .write = soidle_state_write,
    .llseek = seq_lseek,
    .release = single_release,
};

/* slidle_state */
static int _slidle_state_open(struct seq_file *s, void *data)
{
    return 0;
}

static int slidle_state_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, _slidle_state_open, inode->i_private);
}

static ssize_t slidle_state_read(struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
    int len = 0;
    char *p = dbg_buf;
    int i;

    p += sprintf(p, "*********** slow idle state ************\n");
    for (i = 0; i < NR_REASONS; i++) {
        p += sprintf(p, "[%d]slidle_block_cnt[%s]=%lu\n",
                i, reason_name[i], slidle_block_cnt[i]);
    }

    p += sprintf(p, "\n");

    for (i = 0; i < NR_GRPS; i++) {
		p += sprintf(p, "[%02d]slidle_condition_mask[%-8s]=0x%08x\t\t"
				"slidle_block_mask[%-8s]=0x%08x\n", i,
				cg_grp_get_name(i), slidle_condition_mask[i],
				cg_grp_get_name(i), slidle_block_mask[i]);
    }

    p += sprintf(p, "\n********** slidle command help **********\n");
    p += sprintf(p, "slidle help:   cat /sys/kernel/debug/cpuidle/slidle_state\n");
    p += sprintf(p, "switch on/off: echo [slidle] 1/0 > /sys/kernel/debug/cpuidle/slidle_state\n");

    len = p - dbg_buf;

    return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t slidle_state_write(struct file *filp, const char __user *userbuf,
                                  size_t count, loff_t *f_pos)
{
    char cmd[32];
    int param;

    count = min(count, sizeof(cmd_buf) - 1);

    if (copy_from_user(cmd_buf, userbuf, count)) {
        return -EFAULT;
    }
    cmd_buf[count] = '\0';

    if (sscanf(userbuf, "%s %d", cmd, &param) == 2) {
        if (!strcmp(cmd, "slidle")) {
            idle_switch[IDLE_TYPE_SL] = param;
        } else if (!strcmp(cmd, "enable")) {
            enable_slidle_by_bit(param);
        } else if (!strcmp(cmd, "disable")) {
            disable_slidle_by_bit(param);
        }
        return count;
    } else if (sscanf(userbuf, "%d", &param) == 1) {
        idle_switch[IDLE_TYPE_SL] = param;
        return count;
    }

    return -EINVAL;
}

static const struct file_operations slidle_state_fops = {
    .open = slidle_state_open,
    .read = slidle_state_read,
    .write = slidle_state_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static struct dentry *root_entry;

static int mtk_cpuidle_debugfs_init(void)
{
    /* TODO: check if debugfs_create_file() failed */
    /* Initialize debugfs */
    root_entry = debugfs_create_dir("cpuidle", NULL);
    if (!root_entry) {
        printk(KERN_WARNING"Can not create debugfs `dpidle_state`\n");
        return 1;
    }

    debugfs_create_file("idle_state", 0644, root_entry, NULL, &idle_state_fops);
    debugfs_create_file("dpidle_state", 0644, root_entry, NULL, &dpidle_state_fops);
    debugfs_create_file("soidle_state", 0644, root_entry, NULL, &soidle_state_fops);
    debugfs_create_file("slidle_state", 0644, root_entry, NULL,
			&slidle_state_fops);

    return 0;
}

#define idle_readl(addr)    DRV_Reg32(addr)

static void __iomem *infrasys_base;
static void __iomem *perisys_base;
static void __iomem *audiosys_base;
static void __iomem *mfgsys_base;
static void __iomem *mmsys_base;
static void __iomem *imgsys_base;
static void __iomem *vdecsys_base;
static void __iomem *vencsys_base;
static void __iomem *sleepsys_base;

#define INFRA_REG(ofs)      (infrasys_base + ofs)
#define PREI_REG(ofs)       (perisys_base + ofs)
#define AUDIO_REG(ofs)      (audiosys_base + ofs)
#define MFG_REG(ofs)        (mfgsys_base + ofs)
#define MM_REG(ofs)         (mmsys_base + ofs)
#define IMG_REG(ofs)        (imgsys_base + ofs)
#define VDEC_REG(ofs)       (vdecsys_base + ofs)
#define VENC_REG(ofs)       (vencsys_base + ofs)
#define SPM_REG(ofs)        (sleepsys_base + ofs)

#ifdef SPM_PWR_STATUS
#undef SPM_PWR_STATUS
#endif

#ifdef SPM_PWR_STATUS_2ND
#undef SPM_PWR_STATUS_2ND
#endif

#define INFRA_PDN_STA       INFRA_REG(0x0048)
#define PERI_PDN0_STA       PREI_REG(0x0018)
#define PERI_PDN1_STA       PREI_REG(0x001C)
#define AUDIO_TOP_CON0      AUDIO_REG(0x0000)
#define MFG_CG_CON      MFG_REG(0)
#define DISP_CG_CON0        MM_REG(0x100)
#define DISP_CG_CON1        MM_REG(0x110)
#define IMG_CG_CON      IMG_REG(0x0000)
#define VDEC_CKEN_SET       VDEC_REG(0x0000)
#define LARB_CKEN_SET       VDEC_REG(0x0008)
#define VENC_CG_CON     VENC_REG(0x0)

#define SPM_PWR_STATUS      SPM_REG(0x060c)
#define SPM_PWR_STATUS_2ND  SPM_REG(0x0610)

#define DIS_PWR_STA_MASK        BIT(3)
#define MFG_PWR_STA_MASK        BIT(4)
#define ISP_PWR_STA_MASK        BIT(5)
#define VDE_PWR_STA_MASK        BIT(7)
#define VEN2_PWR_STA_MASK       BIT(20)
#define VEN_PWR_STA_MASK        BIT(21)
#define MFG_2D_PWR_STA_MASK     BIT(22)
#define MFG_ASYNC_PWR_STA_MASK      BIT(23)
#define AUDIO_PWR_STA_MASK      BIT(24)
#define USB_PWR_STA_MASK        BIT(25)

enum subsys_id {
	SYS_VDE,
	SYS_MFG,
	SYS_VEN,
	SYS_ISP,
	SYS_DIS,
	SYS_VEN2,
	SYS_AUDIO,
	SYS_MFG_2D,
	SYS_MFG_ASYNC,
	SYS_USB,
	NR_SYSS__,
};

static int sys_is_on(enum subsys_id id)
{
	u32 pwr_sta_mask[] = {
		VDE_PWR_STA_MASK,
		MFG_PWR_STA_MASK,
		VEN_PWR_STA_MASK,
		ISP_PWR_STA_MASK,
		DIS_PWR_STA_MASK,
		VEN2_PWR_STA_MASK,
		AUDIO_PWR_STA_MASK,
		MFG_2D_PWR_STA_MASK,
		MFG_ASYNC_PWR_STA_MASK,
		USB_PWR_STA_MASK,
	};

	u32 mask = pwr_sta_mask[id];
	u32 sta = idle_readl(SPM_PWR_STATUS);
	u32 sta_s = idle_readl(SPM_PWR_STATUS_2ND);

	return (sta & mask) && (sta_s & mask);
}

static void get_all_clock_state(u32 clks[NR_GRPS])
{
	int i;

	for (i = 0; i < NR_GRPS; i++)
		clks[i] = 0;

	clks[CG_INFRA] = ~idle_readl(INFRA_PDN_STA);       /* INFRA */

	clks[CG_PERI] = ~idle_readl(PERI_PDN0_STA);       /* PERI */

	if (sys_is_on(SYS_DIS)) {
		clks[CG_DISP0] = ~idle_readl(DISP_CG_CON0);    /* DISP0 */
		clks[CG_DISP1] = ~idle_readl(DISP_CG_CON1);    /* DISP1 */
	}

	if (sys_is_on(SYS_ISP))
		clks[CG_IMAGE] = ~idle_readl(IMG_CG_CON);  /* IMAGE */

	if (sys_is_on(SYS_MFG))
		clks[CG_MFG] = ~idle_readl(MFG_CG_CON);  /* MFG */

	clks[CG_AUDIO] = ~idle_readl(AUDIO_TOP_CON0);  /* AUDIO */

	if (sys_is_on(SYS_VDE)) {
		clks[CG_VDEC0] = idle_readl(VDEC_CKEN_SET);    /* VDEC0 */
		clks[CG_VDEC1] = idle_readl(LARB_CKEN_SET);    /* VDEC1 */
	}

	if (sys_is_on(SYS_VEN))
		clks[CG_VENC] = idle_readl(VENC_CG_CON); /* VENC_JPEG */
}

bool cg_check_idle_can_enter(
	unsigned int *condition_mask, unsigned int *block_mask)
{
	int i;
	u32 clks[NR_GRPS];
	u32 r = 0;

	get_all_clock_state(clks);

	for (i = 0; i < NR_GRPS; i++) {
		block_mask[i] = condition_mask[i] & clks[i];
		r |= block_mask[i];
	}

	return r == 0;
}

static int __init get_base_from_node(
	const char *cmp, void __iomem **pbase, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		idle_err("node '%s' not found!\n", cmp);
		return -1;
	}

	*pbase = of_iomap(node, idx);

	return 0;
}

static void __init iomap_init(void)
{
	get_base_from_node("mediatek,INFRACFG_AO", &infrasys_base, 0);
	get_base_from_node("mediatek,PERICFG", &perisys_base, 0);
	get_base_from_node("mediatek,AUDIO", &audiosys_base, 0);
	get_base_from_node("mediatek,G3D_CONFIG", &mfgsys_base, 0);
	get_base_from_node("mediatek,MMSYS_CONFIG", &mmsys_base, 0);
	get_base_from_node("mediatek,IMGSYS", &imgsys_base, 0);
	get_base_from_node("mediatek,VDEC_GCON", &vdecsys_base, 0);
	get_base_from_node("mediatek,VENC_GCON", &vencsys_base, 0);
	get_base_from_node("mediatek,SLEEP", &sleepsys_base, 0);
}

const char *cg_grp_get_name(int id)
{
	/* TODO: BUG_ON while id is invalid */

	return cg_group_name[id];
}

int __init mtk_cpuidle_internal_init(void)
{
	iomap_init();

	return mtk_cpuidle_debugfs_init();
}

