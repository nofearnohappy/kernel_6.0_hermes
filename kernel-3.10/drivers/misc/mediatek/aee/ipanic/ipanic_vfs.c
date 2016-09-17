#include <linux/file.h>
#include <linux/fs.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/err.h>
#include "ipanic.h"


struct file *expdb_open(void)
{
	static struct file *filp_expdb;
	if (!filp_expdb)
		filp_expdb = filp_open(AEE_EXPDB_PATH, O_RDWR, 0);
	if (IS_ERR(filp_expdb)) {
		LOGD("filp_open(%s) for aee failed (%ld)\n", AEE_EXPDB_PATH, PTR_ERR(filp_expdb));
	}
	return filp_expdb;
}

ssize_t expdb_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	return kernel_write(filp, buf, len, off);
}

ssize_t expdb_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	return kernel_read(filp, off, buf, len);
}
  
