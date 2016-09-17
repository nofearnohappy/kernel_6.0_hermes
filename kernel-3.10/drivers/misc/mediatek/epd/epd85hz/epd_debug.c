/*****************************************************************************/
/*****************************************************************************/
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>

#include <mach/mt_gpio.h>
#include <mach/mt_typedefs.h>

#include <cust_gpio_usage.h>

#include <asm/atomic.h>
#include <asm/io.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~the static variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~the gloable variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int test_pattern = 0;

typedef union _EPD_CTRL_ {
	struct {
		unsigned char BIT0:1;	/* BIT0 //X */
		unsigned char BIT1:1;	/* BIT1 //X */
		unsigned char BIT2:1;	/* BIT2 //B0    //No output, chagne to B4 */
		unsigned char BIT3:1;	/* BIT3 //B1 */
		unsigned char LD:1;	/* B2 */
		unsigned char XDIO:1;	/* B3 */
		unsigned char GCLK:1;	/* B4 */
		unsigned char YDIO:1;	/* B5 */
	} Bits;
	unsigned char Reg_val;
} EPD_CTRL;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~the definition~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define SCREEN_WIDTH  960
#define SCREEN_HEIGHT 540

#define DPI_WIDTH     264
#define DPI_HEIGHT    545

#define SCREEN_CTRL 96
#define EINK_WIDTH   960
/* one pixel take 2bit voltage setting, */
/* one packet is 8bit data bus, IMAGEWIDTH>>2, get how many packet need */
/* << 2, one packet used ARGB 4byte to store */
#define TARGET_BUFFER_SIZE (((EINK_WIDTH+SCREEN_CTRL)>>2)<<2)

#define TOTAL_FRAME 40

#define CLK_XDIO_TR_ON	0
#define CLK_XDIO_TR_OFF	4

#define CLK_DE_TR_ON	0
#define CLK_DE_TR_OFF	((EINK_WIDTH/4) + 4)
#define CLK_LD_TR_ON	(CLK_DE_TR_OFF + 4)
#define CLK_LD_TR_OFF	(CLK_LD_TR_ON + 5)
#define CLK_GCLK_TR_ON	(CLK_LD_TR_ON - 2)
#define CLK_GCLK_TR_OFF	((EINK_WIDTH+SCREEN_CTRL)/4-1)

#define CHESS_BOARD_WIDTH 80
#define BLACK_VOLTAGE 0x55
#define WHITE_VOLTAGE 0xAA

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~extern declare~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* --------------------------------------------------------------------------- */
/* Information Dump Routines */
/* --------------------------------------------------------------------------- */

static void StoreDataHeader(unsigned char *pData, unsigned int DataSize, unsigned int Line)
{
	unsigned int ii = 0;
	unsigned char R, G, B;
	unsigned char *dptr;

	unsigned char A;
	unsigned char tmp = 0;

	unsigned char start_byte = 0x80;


	EPD_CTRL epd_ctrl_instance;


	/* start header ctl line 0~4 */
	/* 0 frame start header */
	unsigned char ydio_start_byte = 0;
	unsigned char ydio_end_byte = 0;
	unsigned char gclk_start_byte = 0;
	unsigned char gclk_end_byte = 0;
	unsigned char xdio_start_byte = 0;
	unsigned char xdio_end_byte = 0;

	switch (Line) {
	case 0:
		ydio_start_byte = 120;
		ydio_end_byte = 120;
		gclk_start_byte = 120;
		gclk_end_byte = 120;
		break;
		/* 1 frame start header */
	case 1:
		ydio_start_byte = 30;
		ydio_end_byte = 120;
		gclk_start_byte = 0;
		gclk_end_byte = 75;
		break;
		/* 2~4 frame start header */
	case 2:
	case 3:
	case 4:
		ydio_start_byte = 120;
		ydio_end_byte = 120;
		gclk_start_byte = 0;
		gclk_end_byte = 75;
		break;
	}

	epd_ctrl_instance.Reg_val = 0x00;

	for (ii = 0; ii < DataSize; ii = ii + 4) {

		if ((ii / 4) == ydio_start_byte) {
			epd_ctrl_instance.Bits.YDIO = 1;
			/* EPD_CTRL.YDIO= 1; */
		}

		if ((ii / 4) == ydio_end_byte) {
			epd_ctrl_instance.Bits.YDIO = 0;
			/* EPD_CTRL.YDIO = 0; */
		}

		if ((ii / 4) == gclk_start_byte) {
			epd_ctrl_instance.Bits.GCLK = 1;
			/* EPD_CTRL.GCLK = 1; */
		}

		if ((ii / 4) == gclk_end_byte) {
			epd_ctrl_instance.Bits.GCLK = 0;
			/* EPD_CTRL.GLCK = 0; */
		}

		dptr = pData + ii;

#if 1
		B = tmp;
		G = ((epd_ctrl_instance.Reg_val & 0xF0) >> 4) | ((tmp & 0x0F) << 4);
		R = (epd_ctrl_instance.Reg_val & 0xF0) | ((tmp & 0xF0) >> 4);
		A = 0xff;
#else

		B = 0;
		G = 0;
		R = 0;
		A = 0xff;

#endif



		*(dptr + 0) = A;
		*(dptr + 1) = R;
		*(dptr + 2) = G;
		*(dptr + 3) = B;

		/*
		   B=0;
		   G=(tmp&0xf0);
		   R=(tmp&0xf)<<3;
		   A=0;

		   *(dptr+0)=epd_ctrl_instance.Reg_val;
		   *(dptr+1)=G;
		   *(dptr+2)=R;
		   *(dptr+3)=A;
		 */
	}

	return;
}

static void StoreTestData(unsigned char *pData, unsigned int DataSize, unsigned int Line, unsigned int frame)
{
	unsigned int ii = 0;
	unsigned char R, G, B;
	unsigned char *dptr;
	unsigned int file_length = 0;
	unsigned char A;
	unsigned char tmp = 0;

	unsigned char start_byte = 0x80;

	EPD_CTRL epd_ctrl_instance;
	unsigned int clk_num;

	epd_ctrl_instance.Reg_val = 0x00;
	for (ii = 0; ii < DataSize; ii = ii + 4) {

		/* -----------------------------------------------------*/
		/* Line Data 120Clk, */
		/* +16 Extra ctrl clk,   0~135 */
		/* CLK0~1, XDIO pulse */
		/* CLK2~121, data */
		/* CLK122~130 delay */
		/* CLK131~132  LD pulse */
		/* CLK133 */
		/* CLK130~135 GCLK */
		/* -----------------------------------------------------// */

		/* -----------------------------------------------------// */
		/* Line Data 120Clk, ctrl clk inculded */
		/* CLK0~4, XDIO pulse */
		/* CLK5~100, data */
		/* CLK101~103 delay */
		/* CLK104~109  LD pulse */
		/* CLK133 */
		/* CLK102~119 GCLK */
		/* -----------------------------------------------------// */

		clk_num = ii / 4;

		if (Line == 0) {
			epd_ctrl_instance.Bits.GCLK = 1;
		} else {
			if (clk_num == CLK_GCLK_TR_ON)
				epd_ctrl_instance.Bits.GCLK = 1;

			if (clk_num == CLK_GCLK_TR_OFF)
				epd_ctrl_instance.Bits.GCLK = 0;
		}

		if (clk_num == CLK_XDIO_TR_ON)
			epd_ctrl_instance.Bits.XDIO = 1;

		if (clk_num == CLK_XDIO_TR_OFF)
			epd_ctrl_instance.Bits.XDIO = 0;

		if (clk_num == CLK_LD_TR_ON)
			epd_ctrl_instance.Bits.LD = 1;

		if (clk_num == CLK_LD_TR_OFF)
			epd_ctrl_instance.Bits.LD = 0;


		if (clk_num >= CLK_DE_TR_ON && clk_num <= CLK_DE_TR_OFF) {
			/* tmp = start_byte + frame; */

			if (frame <= 19)
				tmp = 0x55;
			else
				tmp = 0xAA;
		} else
			tmp = 0x00;
		/* tmp=(Line&0xff); */


		/* tmp = start_byte + frame; */
		/* epd_ctrl_instance.Reg_val = 0x00; */

		dptr = pData + ii;

#if 1
		B = tmp;
		G = ((epd_ctrl_instance.Reg_val & 0xF0) >> 4) | ((tmp & 0x0F) << 4);
		R = (epd_ctrl_instance.Reg_val & 0xF0) | ((tmp & 0xF0) >> 4);
		A = 0xFF;
#else
		B = 0;
		G = 0;
		R = 0;
		A = 0xff;

#endif

		*(dptr + 0) = A;
		*(dptr + 1) = R;
		*(dptr + 2) = G;
		*(dptr + 3) = B;


		/*
		   B=epd_ctrl_instance.Reg_val;
		   G=(tmp&0xf0);
		   R=(tmp&0xf)<<3;
		   A=0;

		   *(dptr+0)=epd_ctrl_instance.Reg_val;
		   *(dptr+1)=G;
		   *(dptr+2)=R;
		   *(dptr+3)=A;
		 */



		/*
		   for(ii=0;ii<DataSize;ii=ii+4){       //ARGB, each used one byte

		   //0,1,2,3,4,5,....
		   //0,120,240, 0 .....



		   #if 0
		   tmp=((ii>>2)&0xff)+((Line%3)*120);

		   if(((Line%3)==2)||((frame%2)==1)){
		   tmp=0xff;
		   }
		   #else
		   tmp=((ii>>2)&0x1)?0x80:0x00;

		   #endif


		   dptr=pData+ii;

		   B=0;
		   G=(tmp&0xf0);
		   R=(tmp&0xf)<<3;
		   A=0;

		   *(dptr+0)=B;
		   *(dptr+1)=G;
		   *(dptr+2)=R;
		   *(dptr+3)=A;
		 */

	}

	return;
}

static void StoreChessBoard(unsigned char *pData, unsigned int DataSize, unsigned int Line, unsigned int invertflg)
{
	unsigned int ii = 0;
	unsigned char R, G, B;
	unsigned char *dptr;
	unsigned int file_length = 0;
	unsigned char A;
	unsigned char tmp = 0;

	unsigned int blackflg = 0;

	unsigned char start_byte = 0x80;

	EPD_CTRL epd_ctrl_instance;
	unsigned int clk_num;

	epd_ctrl_instance.Reg_val = 0x00;
	for (ii = 0; ii < DataSize; ii = ii + 4) {

		if ((Line / CHESS_BOARD_WIDTH) & 0x01) {
			blackflg = invertflg ?
				((ii / CHESS_BOARD_WIDTH) & 0x01) : (!((ii / CHESS_BOARD_WIDTH) & 0x01));
		} else {
			blackflg = (!invertflg) ?
				((ii / CHESS_BOARD_WIDTH) & 0x01) : (!((ii / CHESS_BOARD_WIDTH) & 0x01));
		}

		tmp = blackflg ? BLACK_VOLTAGE : WHITE_VOLTAGE;
		clk_num = ii / 4;

		if (Line == 0)
			epd_ctrl_instance.Bits.GCLK = 1;
		else {
			if (clk_num == CLK_GCLK_TR_ON)
				epd_ctrl_instance.Bits.GCLK = 1;

			if (clk_num == CLK_GCLK_TR_OFF)
				epd_ctrl_instance.Bits.GCLK = 0;

		}

		if (clk_num == CLK_XDIO_TR_ON)
			epd_ctrl_instance.Bits.XDIO = 1;

		if (clk_num == CLK_XDIO_TR_OFF)
			epd_ctrl_instance.Bits.XDIO = 0;

		if (clk_num == CLK_LD_TR_ON)
			epd_ctrl_instance.Bits.LD = 1;

		if (clk_num == CLK_LD_TR_OFF)
			epd_ctrl_instance.Bits.LD = 0;

		if (clk_num >= CLK_DE_TR_ON && clk_num <= CLK_DE_TR_OFF)
			;
		else
			tmp = 0x00;


		dptr = pData + ii;

#if 1
		B = tmp;
		G = ((epd_ctrl_instance.Reg_val & 0xF0) >> 4) | ((tmp & 0x0F) << 4);
		R = (epd_ctrl_instance.Reg_val & 0xF0) | ((tmp & 0xF0) >> 4);
		A = 0xFF;
#else
		B = 0;
		G = 0;
		R = 0;
		A = 0xff;

#endif

		*(dptr + 0) = A;
		*(dptr + 1) = R;
		*(dptr + 2) = G;
		*(dptr + 3) = B;

	}

	return;
}


void epd_gen_pattern_frame(unsigned long pattern_buffer)
{
	pr_debug("gen_pattern_frame in+ %ld", pattern_buffer);
	unsigned char *buff_ptr = (unsigned char *)pattern_buffer;

	unsigned int total_size = 0;
	unsigned int ii = 0, jj = 0;

	unsigned char data_Line_buf[TARGET_BUFFER_SIZE];
	memset(data_Line_buf, 0, TARGET_BUFFER_SIZE);

	for (ii = 0; ii < TOTAL_FRAME; ii++) {
		StoreDataHeader(data_Line_buf, TARGET_BUFFER_SIZE, 0);
		memcpy(buff_ptr, data_Line_buf, TARGET_BUFFER_SIZE);
		buff_ptr = buff_ptr + TARGET_BUFFER_SIZE;
		total_size = total_size + TARGET_BUFFER_SIZE;

		StoreDataHeader(data_Line_buf, TARGET_BUFFER_SIZE, 1);
		memcpy(buff_ptr, data_Line_buf, TARGET_BUFFER_SIZE);
		buff_ptr = buff_ptr + TARGET_BUFFER_SIZE;
		total_size = total_size + TARGET_BUFFER_SIZE;

		StoreDataHeader(data_Line_buf, TARGET_BUFFER_SIZE, 2);
		memcpy(buff_ptr, data_Line_buf, TARGET_BUFFER_SIZE);
		buff_ptr = buff_ptr + TARGET_BUFFER_SIZE;
		total_size = total_size + TARGET_BUFFER_SIZE;

		StoreDataHeader(data_Line_buf, TARGET_BUFFER_SIZE, 3);
		memcpy(buff_ptr, data_Line_buf, TARGET_BUFFER_SIZE);
		buff_ptr = buff_ptr + TARGET_BUFFER_SIZE;
		total_size = total_size + TARGET_BUFFER_SIZE;

		StoreDataHeader(data_Line_buf, TARGET_BUFFER_SIZE, 4);
		memcpy(buff_ptr, data_Line_buf, TARGET_BUFFER_SIZE);
		buff_ptr = buff_ptr + TARGET_BUFFER_SIZE;
		total_size = total_size + TARGET_BUFFER_SIZE;

		for (jj = 0; jj < (DPI_HEIGHT - 5); jj++) {

			memset(data_Line_buf, 0, TARGET_BUFFER_SIZE);

			if (test_pattern == 0)
				StoreTestData(data_Line_buf, TARGET_BUFFER_SIZE, jj, ii);
			else if (test_pattern == 1) {
				if (ii <= 19)
					StoreChessBoard(data_Line_buf, TARGET_BUFFER_SIZE, jj, 0);
				else
					StoreChessBoard(data_Line_buf, TARGET_BUFFER_SIZE, jj, 1);
			} else if (test_pattern == 2)
				StoreChessBoard(data_Line_buf, TARGET_BUFFER_SIZE, jj, 1);

			memcpy(buff_ptr, data_Line_buf, TARGET_BUFFER_SIZE);
			buff_ptr = buff_ptr + TARGET_BUFFER_SIZE;
			total_size = total_size + TARGET_BUFFER_SIZE;

		}

		pr_debug("Gen_Frame Total byte size : %d\n", total_size);
	}

	pr_debug("the %d th frame size is : %d\n", ii, total_size);

	return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static const char STR_HELP[] = "\n" "USAGE\n"
			"	echo [ACTION]... > epd\n"
			"\n"
			"ACTION\n"
			"        EPD-GPIO:[H|L]\n"
			"             debug gpio\n" "\n";

/* TODO: this is a temp debug solution */
static void process_dbg_opt(const char *opt)
{
	if (0 == strncmp(opt, "EPD-GPIO:", 9)) {
		if (0 == strncmp(opt + 9, "H", 1)) {
			pr_debug("[EPD][Debug] - GPIO pull High\n");
			mt_set_gpio_mode(GPIO145 | 0x80000000, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO145 | 0x80000000, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO145 | 0x80000000, GPIO_OUT_ONE);

			mt_set_gpio_mode(GPIO135 | 0x80000000, GPIO_MODE_01);
			mt_set_gpio_dir(GPIO135 | 0x80000000, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO135 | 0x80000000, GPIO_OUT_ONE);

			mt_set_gpio_mode(GPIO136 | 0x80000000, GPIO_MODE_01);
			mt_set_gpio_dir(GPIO136 | 0x80000000, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO136 | 0x80000000, GPIO_OUT_ONE);
		} else if (0 == strncmp(opt + 9, "L", 1)) {
			pr_debug("[EPD][Debug] - GPIO pull Low\n");
			mt_set_gpio_mode(GPIO145 | 0x80000000, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO145 | 0x80000000, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO145 | 0x80000000, GPIO_OUT_ZERO);

			mt_set_gpio_mode(GPIO135 | 0x80000000, GPIO_MODE_01);
			mt_set_gpio_dir(GPIO135 | 0x80000000, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO135 | 0x80000000, GPIO_OUT_ZERO);

			mt_set_gpio_mode(GPIO136 | 0x80000000, GPIO_MODE_01);
			mt_set_gpio_dir(GPIO136 | 0x80000000, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO136 | 0x80000000, GPIO_OUT_ZERO);
		}
	} else
		goto Error;

	return;

 Error:
	pr_err("[EPD] parse command error!\n\n%s", STR_HELP);
}



static void process_dbg_cmd(char *cmd)
{
	char *tok;

	pr_debug("[epd] %s\n", cmd);

	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}

/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

struct dentry *epd_dbgfs = NULL;


static ssize_t debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}


static char debug_buffer[2048];

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	int n = 0;

	n += scnprintf(debug_buffer + n, debug_bufmax - n, STR_HELP);
	debug_buffer[n++] = 0;

	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}


static ssize_t debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;

	process_dbg_cmd(debug_buffer);

	return ret;
}


static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};


void EPD_DBG_Init(void)
{
	pr_debug("EPD DEBUG\n");
	epd_dbgfs = debugfs_create_file("epd", S_IFREG | S_IRUGO, NULL, (void *)0, &debug_fops);
}


void EPD_DBG_Deinit(void)
{
	debugfs_remove(epd_dbgfs);
}

/* #endif */
