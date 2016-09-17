#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>

int lz4k_compress(const unsigned char *src, size_t src_len,
		  unsigned char *dst, size_t *dst_len, void *wrkmem);
int lz4k_decompress_safe(const unsigned char *src, size_t src_len,
			 unsigned char *dst, size_t *dst_len);

#ifdef CONFIG_ZRAM
/* Set ZRAM hooks */
extern void zram_set_hooks(void *compress_func, void *decompress_func, const char *name);
#endif
static int __init lz4k_init(void)
{
#ifdef CONFIG_ZRAM
	zram_set_hooks(&lz4k_compress, &lz4k_decompress_safe, "LZ4K");
#endif
	return 0;
}

static void __exit lz4k_exit(void)
{
	printk(KERN_INFO "Bye LZ4K!\n");
}
module_init(lz4k_init);
module_exit(lz4k_exit);
