#ifndef SECMOD_H
#define SECMOD_H

#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>

struct sec_ops {
    int (*sec_get_rid)(unsigned int *rid);
};

struct sec_mod {
    dev_t                 id;
    int                   init;
    spinlock_t            lock;
    const struct sec_ops *ops;
};

#define NUM_SBC_PUBK_HASH           8
#define NUM_CRYPTO_SEED          16
#define NUM_RID 4


#ifdef CONFIG_OF
/*device information data*/
struct masp_tag {
	u32 size;
	u32 tag;
	unsigned int rom_info_sbc_attr;
	unsigned int rom_info_sdl_attr;
	unsigned int hw_sbcen;
	unsigned int lock_state;
	unsigned int rid[NUM_RID];
	/*rom_info.m_SEC_KEY.crypto_seed */
	unsigned char crypto_seed[NUM_CRYPTO_SEED];
	unsigned int sbc_pubk_hash[NUM_SBC_PUBK_HASH];
};
#endif

#endif /* end of SECMOD_H */
