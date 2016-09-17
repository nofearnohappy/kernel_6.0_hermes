/*****************************************************************************/
/*****************************************************************************/
#include <mach/mt_typedefs.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/printk.h>

#include "epd_drv.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~the static variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
struct i2c_client *I2C_Client;

static struct mutex epd_lock;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define SCREEN_WIDTH  960
#define SCREEN_HEIGHT 540

#define DPI_WIDTH     264
#define DPI_HEIGHT    545

#define PMIC_PWRUP GPIO_A
#define PMIC_PWRCOM GPIO_B

#define PMIC_SLAVE_ADDR 0xA8
#define PMIC_REG_IMST_VALUE 0x00
#define PMIC_REG_FUNC_ADJUST 0x01
#define PMIC_REG_VCOM_SETTING 0x02

#define DEFAULT_VCOM_SETTING 0x74
#define DEFAULT_VCOM_VOLTAGE 2500	/* default -2500 mv */
#define VCOM_STEP 22

#define VCOM_SUPPORT_MIN 604
#define VCOM_SUPPORT_MAX 5002

#define MAX_I2C_READ_NUM 8
#define MAX_I2C_WRITE_NUM 7
#define EPD_I2C_CHANNEL 1
/* ~~~~~~~~~~~~~~~~~~~~~~~the gloable variable~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static int32_t epd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_board_info i2c_epd __initdata = {
	.type = "FP_EPD",
	.addr = 0x54,
	.irq = 8,
};

struct i2c_device_id gEPDI2cIdTable[] = {
	{
	 "FP_EPD", 0}
};

struct i2c_driver epd_i2c_driver = {
	.probe = epd_i2c_probe,
	.driver = {.name = "FP_EPD",},
	.id_table = gEPDI2cIdTable,
};

int epd_mutex_init(struct mutex *m)
{
	mutex_init(m);
	return 0;
}

int epd_sw_mutex_lock(struct mutex *m)
{
	mutex_lock(m);
	return 0;
}

int epd_sw_mutex_unlock(struct mutex *m)
{
	mutex_unlock(m);
	return 0;
}

static int registerI2cDevice(char const *DeviceName, char const *DriverName)
{
	int32_t retVal;

	pr_debug("registerI2cDevice in +\n");
	retVal = strnlen(DeviceName, I2C_NAME_SIZE);
	if (retVal >= I2C_NAME_SIZE) {
		pr_debug("I2c device name too long!\n");
		return -1;
	}

	i2c_register_board_info(EPD_I2C_CHANNEL, &i2c_epd, 1);

	memcpy(gEPDI2cIdTable[0].name, DeviceName, retVal);
	gEPDI2cIdTable[0].name[retVal] = 0;
	gEPDI2cIdTable[0].driver_data = 0;

	retVal = i2c_add_driver(&epd_i2c_driver);
	if (retVal != 0)
		pr_debug("I2C driver add failed, retVal=%d\n", retVal);

	epd_mutex_init(&epd_lock);

	return retVal == 0 ? 0 : -1;
}

static int32_t epd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	pr_debug("%s, client=%p\n", __func__, (void *)client);

	client->timing = 100;
/* i2c_bus_adapter = to_i2c_adapter(client->dev.parent); */
	/*
	 * On some boards the configuration switches
	 *  are connected via an I2C controlled GPIO expander.
	 * At this point in the initialization, we're not
	 *  ready to to I2C yet, so don't try to read any config
	 *  switches here.  Instead, wait until gpio_expander_init().
	 */

	I2C_Client = client;

	return 0;
}

/****************************Platform I2C Read/Write*****************************/
uint8_t epd_i2c_read_len_bytes(struct i2c_client *client, uint8_t offset, uint8_t *buf, uint8_t len)
{
	int ret = 0;
	int read_len = 0;
	uint8_t regAddress = offset;

	while (len > 0) {
		pr_debug("epd_i2c_read_len_bytes, len: %d\n", read_len);
		read_len = (len > MAX_I2C_READ_NUM ? MAX_I2C_READ_NUM : len);

		ret = i2c_master_send(client, (const char *)&regAddress, sizeof(uint8_t));
		if (ret < 0) {
			pr_err("[Error]epd i2c sends command error!\n");
			return 0;
		} else {
			ret = i2c_master_recv(client, (char *)buf, read_len);
			if (ret < 0)
				pr_err("[Error]epd i2c recv data error!\n");

			regAddress += read_len;
			buf += read_len;
			len -= read_len;
		}
	}

	return ret;
}

uint8_t epd_i2c_write_len_bytes(struct i2c_client *client, uint8_t offset, uint8_t *buf, uint8_t len)
{
	int i = 0;
	int ret = 0;
	int write_len = 0;
	char write_data[8];
	uint8_t regAddress = offset;

	while (len > 0) {
		pr_debug("mhl_i2c_write_len_bytes, len: %d\n", len);
		write_len = (len > MAX_I2C_WRITE_NUM ? MAX_I2C_WRITE_NUM : len);
		write_data[0] = regAddress;

		for (i = 0; i < write_len; i++)
			write_data[i + 1] = *(buf + i);

		ret = i2c_master_send(client, write_data, write_len + 1);
		if (ret < 0) {
			pr_err("[Error]epd i2c write command/data error!\n");
			return 0;
		}

		regAddress += write_len;
		len -= write_len;
		buf += write_len;
	}

	return ret;
}

uint8_t I2C_Read_Block(uint8_t deviceID, uint8_t offset, uint8_t *buf, uint8_t len)
{
	uint8_t slave_addr = deviceID;
	uint8_t accessI2cAddr = 0;
	u32 client_main_addr;

	pr_debug("epd enter %s (0x%02x, 0x%02x, 0x%02x)\n", __func__, deviceID, offset, len);

	epd_sw_mutex_lock(&epd_lock);
	accessI2cAddr = slave_addr >> 1;

	/*backup default client address */
	client_main_addr = I2C_Client->addr;
	I2C_Client->addr = accessI2cAddr;
	/* I2C_Client->addr = (accessI2cAddr & I2C_MASK_FLAG)|I2C_WR_FLAG; */
	I2C_Client->timing = 100;

	memset(buf, 0xff, len);
	epd_i2c_read_len_bytes(I2C_Client, offset, buf, len);

	/* restore default client address */
	I2C_Client->addr = client_main_addr;

	epd_sw_mutex_unlock(&epd_lock);
	return len;
}

void I2C_Write_Block(uint8_t deviceID, uint8_t offset, uint8_t *buf, uint16_t len)
{
	uint8_t tmp[2] = { 0 };
	uint8_t accessI2cAddr;
	uint8_t slave_addr = deviceID;
	uint32_t client_main_addr;

	pr_debug("epd enter %s (0x%02x, 0x%02x, 0x%02x)\n", __func__, deviceID, offset, len);

	epd_sw_mutex_lock(&epd_lock);
	accessI2cAddr = slave_addr >> 1;

	/* backup addr */
	client_main_addr = I2C_Client->addr;
	I2C_Client->addr = accessI2cAddr;
	I2C_Client->timing = 100;
	epd_i2c_write_len_bytes(I2C_Client, offset, buf, len);

	/* restore default client address */
	I2C_Client->addr = client_main_addr;
	epd_sw_mutex_unlock(&epd_lock);

	return;
}

void init(void)
{
	pr_debug("epd driver init in +");
	registerI2cDevice("FP_EPD", "fp9928adrv");
}

void get_params(LCM_EPD_PARAMS *params)
{
	/*pannel size */
	params->width = DPI_WIDTH;
	params->height = DPI_HEIGHT;

	/*DPI signal polarity setting... */
	params->clk_pol = EPD_POLARITY_FALLING;
	params->de_pol = EPD_POLARITY_RISING;
	params->hsync_pol = EPD_POLARITY_FALLING;
	params->vsync_pol = EPD_POLARITY_FALLING;

	/*DPI setting... */
	params->format = LCM_DPI_FORMAT_RGB888;
	params->rgb_order = LCM_COLOR_ORDER_RGB;
	params->i2x_en = true;
	params->i2x_edge = 2;
	params->embsync = false;
	params->pannel_frq = 85;

	/*timing parameters */
	params->hsync_back_porch = 2;
	params->hsync_front_porch = 450;
	params->hsync_pulse_width = 2;

	params->vsync_back_porch = 1;
	params->vsync_front_porch = 15;
	params->vsync_pulse_width = 1;

	/* DPI output clock, and E-ink input clock */
	params->PLL_CLOCK = 34989;	/* dpi_width*dpi_height*3*85 */
}

void get_screen_size(unsigned int *pWidth, unsigned int *pHeight)
{
	*pWidth = SCREEN_WIDTH;
	*pHeight = SCREEN_HEIGHT;
}

void power_on(void)
{				/*
				   //need to add the actions
				   unsigned int v_dif  = 0;
				   unsigned int VCOM   = DEFAULT_VCOM_VOLTAGE;
				   unsigned char plus_flg = 0;
				   unsigned char OutVcom  = 0;
				 */
	/*GPIO setting */
/*
    GPIO_SETTING(PMIC_PWRUP,ON);
    GPIO_SETTING(PMIC_PWRCOM,ON);

    msleep(15);//delay 15 ms

    VCOM = READ_VCOM_From_Flash();
    if(VCOM <= VCOM_SUPPORT_MIN)
    {
	VCOM = VCOM_SUPPORT_MIN;
    }

    if(VCOM >= VCOM_SUPPORT_MAX)
    {
	VCOM = VCOM_SUPPORT_MAX;
    }

    plus_flg = (VCOM>=DEFAULT_VCOM_VOLTAGE) ? 1 : 0;
    v_dif = plus_flg ? (VCOM - DEFAULT_VCOM_VOLTAGE) : (DEFAULT_VCOM_VOLTAGE - VCOM);
    v_dif = v_dif/VCOM_STEP;
    OutVcom = plus_flg ? (DEFAULT_VCOM_VOLTAGE + v_dif) : (DEFAULT_VCOM_VOLTAGE - v_dif);

    I2C_Write_Block(PMIC_SLAVE_ADDR, PMIC_REG_VCOM_SETTING, &OutVcom, 1);
    I2C_Setting_PMIC(PMIC_SLAVE_ADDR, PMIC_REG_VCOM_SETTING, OutVcom);

    msleep(15);//delay 15 ms
*/
	return;
}

void power_off(void)
{				/*
				   GPIO_SETTING(PMIC_PWRUP, OFF);
				   GPIO_SETTING(PMIC_PWRCOM, OFF);

				   msleep(15);//delay 15 ms
				 */
	return;
}

void epd_test_l(bool enable)
{

}

const EPD_DRIVER *EPD_GetDriver(void)
{
	static const EPD_DRIVER EPD_DRV = {
		.init = init,
		.get_params = get_params,
		.gen_pattern_frame = NULL,
		.get_screen_size = get_screen_size,
		.power_on = power_on,
		.power_off = power_off
	};

	return &EPD_DRV;
}

/* #endif */
