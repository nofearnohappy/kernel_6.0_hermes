/******************************************************************************
*
 *
 * Filename:
 * ---------
 *    mt_soc_pcm_common
 *
 * Project:
 * --------
 *     mt_soc_pcm_common function
 *
 *
 * Description:
 * ------------
 *   common function
 *
 * Author:
 * -------
 *   Chipeng Chang (MTK02308)
 *
 *---------------------------------------------------------------------------
---
 *

*******************************************************************************/

#include "mt_soc_pcm_common.h"

unsigned long audio_frame_to_bytes(struct snd_pcm_substream *substream, unsigned long count)
{
	unsigned long bytes = count;
	struct snd_pcm_runtime *runtime = substream->runtime;
	if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
	    runtime->format == SNDRV_PCM_FORMAT_U32_LE)
		bytes = bytes << 2;
	else
		bytes = bytes << 1;

	if (runtime->channels == 2)
		bytes = bytes << 1;

	/* printk("%s bytes = %d count = %d\n",__func__,bytes,count); */
	return bytes;
}


unsigned long audio_bytes_to_frame(struct snd_pcm_substream *substream, unsigned long bytes)
{
	unsigned long count = bytes;
	struct snd_pcm_runtime *runtime = substream->runtime;
	if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
	    runtime->format == SNDRV_PCM_FORMAT_U32_LE)
		count = count >> 2;
	else
		count = count >> 1;

	if (runtime->channels == 2)
		count = count >> 1;

	/* printk("%s bytes = %d count = %d\n",__func__,bytes,count); */
	return count;
}

unsigned long mtk_local_audio_copy_from_user(bool IsSRAM, kal_uint8 *dst, char *src, int len)
{
	if (IsSRAM) {
		/* PRINTK_AUDDRV("mtk_local_audio_copy_from_user SRAM = %d\n", len); */
#ifdef MEMCPY_SINGLE_MODE
		int loopcnt = (len >> 2);
		int remain = len - (loopcnt<<2);
		int i;
		for (i = 0; i < loopcnt; i++)
			copy_from_user(dst+i*4, src+i*4, 4);
		if (remain)
			copy_from_user(dst+loopcnt*4, src+loopcnt*4, remain);
#else	/* use burst mode */
		copy_from_user(dst, src, len);
#endif
	} else {
		/* PRINTK_AUDDRV("mtk_local_audio_copy_from_user DRAM = %d\n", len); */
		copy_from_user(dst, src, len);
	}
	return 0;
}

unsigned long mtk_local_audio_copy_to_user(bool IsSRAM, kal_uint8 *dst, char *src, int len)
{
	if (IsSRAM) {
		/* PRINTK_AUDDRV("mtk_local_audio_copy_to_user SRAM = %d\n", len); */
#ifdef MEMCPY_SINGLE_MODE
		int loopcnt = (len >> 2);
		int remain = len - (loopcnt<<2);
		int i;
		for (i = 0; i < loopcnt; i++)
			copy_to_user(dst+i*4, src+i*4, 4);
		if (remain)
			copy_to_user(dst+loopcnt*4, src+loopcnt*4, remain);
#else	/* use burst mode */
		copy_to_user(dst, src, len);
#endif
	} else {
		/* PRINTK_AUDDRV("mtk_local_audio_copy_to_user DRAM = %d\n", len); */
		copy_to_user(dst, src, len);
	}
	return 0;
}

