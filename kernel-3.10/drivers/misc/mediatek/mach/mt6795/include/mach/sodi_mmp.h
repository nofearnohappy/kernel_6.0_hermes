#ifndef __H_SODI_MMP__
#define __H_SODI_MMP__

#include <linux/mmprofile.h>

typedef struct {
    MMP_Event SODI;
    MMP_Event sodi_enable;
    MMP_Event self_refresh_cnt;
    MMP_Event sodi_status;    
}SODI_MMP_Events_t ;

SODI_MMP_Events_t * sodi_mmp_get_events(void);
void init_sodi_mmp_events(void);
void sodi_mmp_init(void);

#endif
