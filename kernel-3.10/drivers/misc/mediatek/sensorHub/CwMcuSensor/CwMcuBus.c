#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <mach/eint.h>
#include <cust_cwmcu.h>
#include <mach/mt_pm_ldo.h>
#include <linux/spi/spi.h>
#include <mach/mt_spi.h>
#include <mach/mt_sleep.h>
#include "CwMcuSensor.h"


extern struct CWMCU_data *sensor;
static DEFINE_MUTEX(cwmcu_bus_lock);


#ifdef CWMCU_I2C_INTERFACE
static u8 *CWI2CDMABuf_va = NULL;
static u64 CWI2CDMABuf_pa = NULL;
static struct i2c_board_info __initdata i2c_cw_boardinfo = { I2C_BOARD_INFO("CwMcuSensor", (0x3a))};

static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, s32 len)
{
    int ret;
    s32 retry = 0;
    u8 buffer[2];

    struct i2c_msg msg[2] =
    {
        {
            .addr = (client->addr & I2C_MASK_FLAG),
            .flags = 0,//(client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
            .buf = buffer,
            .len = 1,
            .timing = I2C_MASTER_CLOCK
        },
        {
            .addr = (client->addr & I2C_MASK_FLAG),
            .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
            .flags = I2C_M_RD,
            .buf = CWI2CDMABuf_pa,     
            .len = len,
            .timing = I2C_MASTER_CLOCK
        },
    };
    
    buffer[0] = addr; 
  
    if (rxbuf == NULL)
        return -1;

    //GTP_DEBUG("dma i2c read: 0x%04X, %d bytes(s)", addr, len);
    for (retry = 0; retry < 10; ++retry)
    {
        ret = i2c_transfer(client->adapter, &msg[0], 2);
        if (ret < 0)
        {
        	CW_ERROR("I2C DMA read error retry=%d", retry);
            continue;
        }
        memcpy(rxbuf, CWI2CDMABuf_va, len);
        return 0;
    }
    CW_ERROR("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d", addr, len, ret);
    return ret;
}


static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
    int ret;
    s32 retry = 0;
    u8 *wr_buf = CWI2CDMABuf_va;
    //CW_DEBUG("fwq3,.....%x",txbuf[0]);
    struct i2c_msg msg =
    {
        .addr = (client->addr & I2C_MASK_FLAG),
        .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
        .flags = 0,
        .buf = CWI2CDMABuf_pa,
        .len = 1+len,
        .timing = I2C_MASTER_CLOCK
    };
    
    wr_buf[0] = addr;

    if (txbuf == NULL)
        return -1;
    
    //GTP_DEBUG("dma i2c write: 0x%04X, %d bytes(s)", addr, len);
    memcpy(wr_buf+1, txbuf, len+1);
	//for(retry=0;retry<len+1;retry++)
	//{
	//	CW_DEBUG("fwq4,.....wr_buf[%d]=%x",retry,wr_buf[retry]);
	//}
    for (retry = 0; retry < 5; ++retry)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret < 0)
        {
        	CW_ERROR("I2C DMA write error retry=%d", retry);
            continue;
        }
        return 0;
    }
    CW_ERROR("Dma I2C Write Error: 0x%04X, %d byte(s), err-code: %d", addr, len, ret);
    return ret;
}

static s32 i2c_dma_read_serial(struct i2c_client *client, u8 *rxbuf, s32 len)
{
    int ret;
    s32 retry = 0;
    u8 buffer[2];

    struct i2c_msg msg[2] =
    {
        {
            .addr = (client->addr & I2C_MASK_FLAG),
            .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
            .flags = I2C_M_RD,
            .buf = CWI2CDMABuf_pa,     
            .len = len,
            .timing = I2C_MASTER_CLOCK
        },
    };
     
    if (rxbuf == NULL)
        return -1;

    //GTP_DEBUG("dma i2c read: 0x%04X, %d bytes(s)", addr, len);
    for (retry = 0; retry < 5; ++retry)
    {
        ret = i2c_transfer(client->adapter, &msg[0], 1);
        if (ret < 0)
        {
        	CW_ERROR("I2C DMA read error retry=%d", retry);
            continue;
        }
        memcpy(rxbuf, CWI2CDMABuf_va, len);
        return 0;
    }
    CW_ERROR("Dma I2C Read Error: %d byte(s), err-code: %d", len, ret);
    return ret;
}

static s32 i2c_dma_write_serial(struct i2c_client *client, u8 *txbuf, s32 len)
{
    int ret;
    s32 retry = 0;
    u8 *wr_buf = CWI2CDMABuf_va;
    
    struct i2c_msg msg =
    {
        .addr = (client->addr & I2C_MASK_FLAG),
        .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
        .flags = 0,
        .buf = CWI2CDMABuf_pa,
        .len = len,
        .timing = I2C_MASTER_CLOCK
    };
    
    if (txbuf == NULL)
        return -1;
    
    //GTP_DEBUG("dma i2c write: 0x%04X, %d bytes(s)", addr, len);
    memcpy(wr_buf, txbuf, len);
    for (retry = 0; retry < 10; ++retry)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret < 0)
        {
        	CW_ERROR("I2C DMA write error retry=%d", retry);
            continue;
        }
        return 0;
    }
    CW_ERROR("Dma I2C Write Error: %d byte(s), err-code: %d",len, ret);
    return ret;
}

static s32 i2c_read_bytes_dma(struct i2c_client *client, u8 addr, u8 *rxbuf, s32 len)
{
    s32 left = len;
    s32 read_len = 0;
    u8 *rd_buf = rxbuf;
    s32 ret = 0;    
    
    //GTP_DEBUG("Read bytes dma: 0x%04X, %d byte(s)", addr, len);
    while (left > 0)
    {
        if (left > GTP_DMA_MAX_TRANSACTION_LENGTH)
        {
            read_len = GTP_DMA_MAX_TRANSACTION_LENGTH;
        }
        else
        {
            read_len = left;
        }
        ret = i2c_dma_read(client, addr, rd_buf, read_len);
        if (ret < 0)
        {
            CW_ERROR("dma read failed");
            return -1;
        }
        
        left -= read_len;
        addr += read_len;
        rd_buf += read_len;
    }
    return 0;
}

static s32 i2c_write_bytes_dma(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{

    s32 ret = 0;
    s32 write_len = 0;
    s32 left = len;
    u8 *wr_buf = txbuf;
    //CW_DEBUG("fwq2...%x",txbuf[0]);
    //GTP_DEBUG("Write bytes dma: 0x%04X, %d byte(s)", addr, len);
    while (left > 0)
    {
        if (left > GTP_DMA_MAX_I2C_TRANSFER_SIZE)
        {
            write_len = GTP_DMA_MAX_I2C_TRANSFER_SIZE;
        }
        else
        {
            write_len = left;
        }
        ret = i2c_dma_write(client, addr, wr_buf, write_len);
        
        if (ret < 0)
        {
            CW_ERROR("dma i2c write failed!");
            return -1;
        }
        
        left -= write_len;
        addr += write_len;
        wr_buf += write_len;
    }
    return 0;
}

static s32 i2c_write_bytes_dma_serial(struct i2c_client *client,u8 *txbuf, s32 len)
{

    s32 ret = 0;
    s32 write_len = 0;
    s32 left = len;
    u8 *wr_buf = txbuf;
    
    //GTP_DEBUG("Write bytes dma: 0x%04X, %d byte(s)", addr, len);
    while (left > 0)
    {
        if (left > GTP_DMA_MAX_I2C_TRANSFER_SIZE)
        {
            write_len = GTP_DMA_MAX_I2C_TRANSFER_SIZE;
        }
        else
        {
            write_len = left;
        }
        ret = i2c_dma_write_serial(client, wr_buf, write_len);
        
        if (ret < 0)
        {
            CW_ERROR("dma i2c write failed!");
            return -1;
        }
        
        left -= write_len;
        wr_buf += write_len;
    }
    return 0;
}

static s32 i2c_read_bytes_dma_serial(struct i2c_client *client, u8 *rxbuf, s32 len)
{
    s32 left = len;
    s32 read_len = 0;
    u8 *rd_buf = rxbuf;
    s32 ret = 0;    
    
    //GTP_DEBUG("Read bytes dma: 0x%04X, %d byte(s)", addr, len);
    while (left > 0)
    {
        if (left > GTP_DMA_MAX_TRANSACTION_LENGTH)
        {
            read_len = GTP_DMA_MAX_TRANSACTION_LENGTH;
        }
        else
        {
            read_len = left;
        }
        ret = i2c_dma_read_serial(client, rd_buf, read_len);
        if (ret < 0)
        {
            CW_ERROR("dma read serial failed");
            return -1;
        }
        
        left -= read_len;
        
        rd_buf += read_len;
    }
    return 0;
}

void i2c_init(void *client)
{
	CWI2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, GTP_DMA_MAX_TRANSACTION_LENGTH, &CWI2CDMABuf_pa, GFP_KERNEL);
    if(!CWI2CDMABuf_va)
	{
    	CW_INFO("[sensorHUB] dma_alloc_coherent error");
	}

    sensor->client = (struct i2c_client *)client;
    i2c_set_clientdata(sensor->client, sensor);
}

static int CWMCU_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{    
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CW_ERROR("i2c_check_functionality error");
		return -EIO;
	}

    CWMCU_probe(client);
    pm_runtime_enable(&client->dev);  

    return 0;
}

static int CWMCU_i2c_remove(struct i2c_client *client)
{
	struct CWMCU_data *sensor = i2c_get_clientdata(client);
	kfree(sensor);
    
	return 0;
}

static int CWMCU_i2c_suspend(struct device *dev)
{
    CWMCU_suspend(&sensor->client->dev);

}

static int CWMCU_i2c_resume(struct device *dev)
{
    CWMCU_resume(&sensor->client->dev);
}

static const struct dev_pm_ops CWMCU_pm_ops = {
	.suspend = CWMCU_i2c_suspend,
	.resume = CWMCU_i2c_resume
};

static const struct i2c_device_id CWMCU_id[] = {
	{ CWMCU_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, CWMCU_id);

static struct i2c_driver CWMCU_i2c_driver = {
	.driver = {
		.name = CWMCU_NAME,
		.owner = THIS_MODULE,
		/*.pm = &CWMCU_pm_ops,*/
	},
	.probe    = CWMCU_i2c_probe,
	.remove   = CWMCU_i2c_remove,
	.id_table = CWMCU_id,
};

#elif defined(CWMCU_SPI_INTERFACE)
#define CWMCU_SPI_OLD_PROTOCOL

unsigned char* cwmcu_spi_src_buffer_all = NULL;
static struct mt_chip_conf spi_conf;

void spi_io_enable(int enable)
{
#if 1
    if (enable){        
        mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_SPI_CS);
        mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_SPI_SCK);
        mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_SPI_MISO);    
        mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_SPI_MOSI);
    }
    else{
        //set dir pull to save power
        mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_SPI_CS_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_SPI_CS_PIN, GPIO_PULL_DISABLE);
            
        mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_SPI_SCK_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_SPI_SCK_PIN, GPIO_PULL_DISABLE);
        
        mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_SPI_MISO_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_SPI_MISO_PIN, GPIO_PULL_DISABLE);
        
        mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_GPIO);
        mt_set_gpio_dir(GPIO_SPI_MOSI_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_SPI_MOSI_PIN, GPIO_PULL_DISABLE);
    }    

#endif
}

static int spi_xfer(unsigned char *txbuf,unsigned char *rxbuf, int len)
{
	int ret;
    struct spi_transfer transfer_1[2];  
	int const pkt_count = len / CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	int const remainder = len % CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	struct spi_message msg;
	spi_message_init(&msg);

	spi_io_enable(1);

	//CW_INFO(" len=%d, txbuf=0x%x, rxbuf=0x%x", len, txbuf, rxbuf);
	if(len>CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
		transfer_1[0].tx_buf =(txbuf==NULL)?NULL: txbuf;
		transfer_1[0].rx_buf =(rxbuf==NULL)?NULL: rxbuf;
		transfer_1[0].len = CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count;
		spi_message_add_tail(&transfer_1[0], &msg);

		if(0 != remainder)	 { 
			transfer_1[1].tx_buf =(txbuf==NULL)?NULL:txbuf+ (CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer_1[1].rx_buf =(rxbuf==NULL)?NULL:rxbuf+ (CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer_1[1].len = remainder;
			spi_message_add_tail(&transfer_1[1], &msg);
		}
	}
	else{
		transfer_1[0].tx_buf =(txbuf==NULL)?NULL: txbuf;
		transfer_1[0].rx_buf =(rxbuf==NULL)?NULL: rxbuf;
		transfer_1[0].len = len;
		spi_message_add_tail(&transfer_1[0], &msg);
	}
	if(spi_sync(sensor->spi, &msg))
		ret =  -1;	
	else
		ret = 0;

	spi_io_enable(0);

	return ret;
}

static int spi_write_bytes_serial(unsigned char *buffer, int len)
{
	int ret = 0;
	unsigned char* tx_buf=NULL, *rx_buf=NULL;

	if(len > CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
        tx_buf = buffer;
        rx_buf =NULL;
	}
	else{
		if(cwmcu_spi_src_buffer_all==NULL)
			return -1;
		if(buffer != cwmcu_spi_src_buffer_all)
			memcpy(cwmcu_spi_src_buffer_all,buffer,len);
            tx_buf = cwmcu_spi_src_buffer_all;
            rx_buf =NULL;
	}
	ret = spi_xfer(tx_buf,rx_buf, len);

	return ret;
}

static int spi_read_bytes_serial(unsigned char *buffer, int len)       
{
	int ret = 0;
	unsigned char *tx_buf=NULL, *rx_buf=NULL;

	if(len>CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	         
              tx_buf = NULL;
              rx_buf = buffer;
  		ret = spi_xfer(tx_buf,rx_buf, len);
	}
	else{                                                                        
		if(cwmcu_spi_src_buffer_all == NULL)
			return -1;
        tx_buf = NULL;
        rx_buf = cwmcu_spi_src_buffer_all;
		ret = spi_xfer(tx_buf,rx_buf, len);
		if(ret == 0)
            memcpy(buffer,cwmcu_spi_src_buffer_all,len);             
	}	  	
    return ret;
}

static int spi_read_bytes(u8 reg_addr, u8 *buffer, u8 len)
{  
	int ret = 0;       

    //CW_DEBUG("[SPI]R_Reg:0x%x, len=%d", reg_addr, len);
	if(cwmcu_spi_src_buffer_all == NULL)
	    return -1;
#ifdef CWMCU_SPI_OLD_PROTOCOL    
	cwmcu_spi_src_buffer_all[0]= reg_addr;

	ret = spi_xfer(cwmcu_spi_src_buffer_all, NULL, 1);
	if (ret < 0)
    {
        CW_ERROR("spi_read_bytes: write failed");
        return ret;
    }    
    ret = spi_read_bytes_serial(buffer, len);
    if (ret < 0)
    {
        CW_ERROR("spi_read_bytes: read failed");
        return ret;
    }
#else //New Protocol
    cwmcu_spi_src_buffer_all[0]= 0x0;
    cwmcu_spi_src_buffer_all[1]= 0xA1;
    cwmcu_spi_src_buffer_all[2]= reg_addr;
    
    ret = spi_xfer(cwmcu_spi_src_buffer_all, NULL, 15);
	if (ret < 0)
    {
        CW_ERROR("spi_read_bytes: write failed");
        return ret;
    }  

    ret = spi_read_bytes_serial(cwmcu_spi_src_buffer_all, 15);
    if (ret < 0)
    {
        CW_ERROR("spi_read_bytes: read failed");
        return ret;
    }
    memcpy(buffer, &cwmcu_spi_src_buffer_all[3], len);
#endif
    return ret;
}

static int spi_write_bytes(u8 reg_addr, u8 *buffer, u8 len)
{  
	int ret = 0;       
	if(cwmcu_spi_src_buffer_all == NULL)
	    return -1;

    //CW_DEBUG("[SPI]W_Reg:0x%x, len=%d", reg_addr, len);
#if 0    
	cwmcu_spi_src_buffer_all[0] = reg_addr;
    memcpy(&cwmcu_spi_src_buffer_all[1], buffer, len);

	ret = spi_write_bytes_serial(cwmcu_spi_src_buffer_all, len+1);
	if (ret < 0)
    {
        CW_ERROR("spi_write_bytes: write failed");
        return ret;
    }:
#elif defined(CWMCU_SPI_OLD_PROTOCOL)
    cwmcu_spi_src_buffer_all[0] = reg_addr;
    ret = spi_xfer(cwmcu_spi_src_buffer_all, NULL, 1);
    if (ret < 0)
    {
        CW_ERROR("spi_write_bytes 1st: write failed");
        return ret;
    }
    memcpy(&cwmcu_spi_src_buffer_all[0], buffer, len);
	ret = spi_write_bytes_serial(cwmcu_spi_src_buffer_all, len);
	if (ret < 0)
    {
        CW_ERROR("spi_write_bytes 2nd: write failed");
        return ret;
    }   
#else //New protocol
    cwmcu_spi_src_buffer_all[0] = 0x0;
    cwmcu_spi_src_buffer_all[1] = 0xA0;
    cwmcu_spi_src_buffer_all[2] = reg_addr;
    memcpy(&cwmcu_spi_src_buffer_all[3], buffer, len);
    ret = spi_write_bytes_serial(cwmcu_spi_src_buffer_all, 15);
	if (ret < 0)
    {
        CW_ERROR("spi_write_bytes: write failed");
        return ret;
    }

#endif
}

int spi_rw_bytes_serial(u8 *wbuf, u8 *rbuf, u8 len)
{
    int ret;
    unsigned char *tx_buf=NULL, *rx_buf=NULL;

    tx_buf = wbuf;
    rx_buf = cwmcu_spi_src_buffer_all;
    
    ret = spi_xfer(tx_buf, rx_buf, len);
    if(ret == 0)
        memcpy(rbuf,cwmcu_spi_src_buffer_all, len);  
    else if (ret < 0)
    {
        CW_ERROR("spi_rw_bytes: read failed");
    }
    
    return ret;
}

void spi_init(void *dev)
{
	struct mt_chip_conf* spi_par;

    sensor->spi = dev;

    spi_io_enable(1);
    
	cwmcu_spi_src_buffer_all = kmalloc(CWMCU_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES,GFP_KERNEL);
	if(cwmcu_spi_src_buffer_all== NULL){
		CW_ERROR("error kmalloc fail cwmcu_spi_src_buffer_all");
	}
    //INNODev->spi->controller_data =(void*)&spi_conf; 
    spi_par =&spi_conf;
	if(!spi_par){
		CW_ERROR("spi config fail");
	}

	spi_par->setuptime = 15;
	spi_par->holdtime = 15;
	spi_par->high_time = 10;       //10--6m   15--4m   20--3m  30--2m  [ 60--1m 120--0.5m  300--0.2m]
	spi_par->low_time = 10;
	spi_par->cs_idletime = 20;

	spi_par->rx_mlsb = 1; 
	spi_par->tx_mlsb = 1;		 
	spi_par->tx_endian = 0;
	spi_par->rx_endian = 0;

	spi_par->cpol = 0;
	spi_par->cpha = 0;
	spi_par->com_mod = DMA_TRANSFER;

	spi_par->pause = 0;
	spi_par->finish_intr = 1;
	spi_par->deassert = 0;

	if(spi_setup(sensor->spi)){
		CW_ERROR("spi_setup fail");
	}    

	spi_io_enable(0);
    //TODO
    //SPI DMA SETTING
}

int CWMCU_spi_probe(struct spi_device *spi)
{
    CW_INFO("CWMCU_spi_probe entry");
       
    CWMCU_probe(spi);
    
    pm_runtime_enable(&spi->dev);

	return 0;
}

int CWMCU_spi_remove(struct spi_device *spi)
{
    sensor->spi = NULL;
  
	return 0; 
}

int CWMCU_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
    CWMCU_suspend(&spi->dev); 
    return 0;
}

int CWMCU_spi_resume(struct spi_device *spi)
{
    CWMCU_resume(&spi->dev);
}

static struct spi_board_info spi_cw_boardinfo __initdata = {
	.modalias = CWMCU_NAME,
	.bus_num = 0,
	.chip_select=0,
	.mode = SPI_MODE_3,
};

static struct spi_driver CWMCU_spi_driver = {
	.driver = {
		.name =		CWMCU_NAME,                 
		.owner =	THIS_MODULE,
	},
	.probe   =	CWMCU_spi_probe,
	.remove  =	CWMCU_spi_remove,
	.suspend =  CWMCU_spi_suspend,
	.resume  =  CWMCU_spi_resume,
};


#endif //bus type
/*======================================================== */
void CWMCU_bus_init(void *bus_dev)
{
#if defined(CWMCU_I2C_INTERFACE)
    i2c_init(bus_dev);
#elif defined(CWMCU_SPI_INTERFACE)
    spi_init(bus_dev);
#endif
}

int CWMCU_bus_register(void)
{
#if defined(CWMCU_I2C_INTERFACE)	
    i2c_register_board_info(CWMCU_I2C_NUMBER, &i2c_cw_boardinfo, 1);
    return i2c_add_driver(&CWMCU_i2c_driver);
#elif defined(CWMCU_SPI_INTERFACE)
    spi_register_board_info(&spi_cw_boardinfo, 1);
    return spi_register_driver(&CWMCU_spi_driver);
#endif
}

void CWMCU_bus_unregister(void)
{
#if defined(CWMCU_I2C_INTERFACE)	
    i2c_del_driver(&CWMCU_i2c_driver);
#elif defined(CWMCU_SPI_INTERFACE)
    
#endif
}

int CWMCU_bus_write(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len)
{
	int ret;

    mutex_lock(&cwmcu_bus_lock);
#if defined(CWMCU_I2C_INTERFACE)	
	ret = i2c_write_bytes_dma(sensor->client, reg_addr, data,len);
#elif defined(CWMCU_SPI_INTERFACE)
    ret = spi_write_bytes(reg_addr, data, len);
#endif
    mutex_unlock(&cwmcu_bus_lock);

	return ret;
}

/* Returns the number of read bytes on success */
int CWMCU_bus_read(struct CWMCU_data *sensor, u8 reg_addr, u8 *data, u8 len)
{
    int ret = 0;
    mutex_lock(&cwmcu_bus_lock);

#if defined(CWMCU_I2C_INTERFACE)
	ret = i2c_read_bytes_dma(sensor->client, reg_addr, data, len);
#elif defined(CWMCU_SPI_INTERFACE)
    ret = spi_read_bytes(reg_addr, data, len);
#endif
    mutex_unlock(&cwmcu_bus_lock);

    return ret;
}

int CWMCU_bus_write_serial(u8 *data, int len)
{
	int ret = 0;

    mutex_lock(&cwmcu_bus_lock);
#if defined(CWMCU_I2C_INTERFACE)	
	ret = i2c_write_bytes_dma_serial(sensor->client, data, len);

	if (ret < 0) {
		CW_ERROR("i2c write error =%d", ret);
		goto BUS_WS_EXIT;
	}
#elif defined(CWMCU_SPI_INTERFACE)
    ret = spi_write_bytes_serial(data, len);

    if (ret < 0) {
        CW_ERROR("spi write error =%d", ret);
        goto BUS_WS_EXIT;
    }
#endif
    mutex_unlock(&cwmcu_bus_lock);

BUS_WS_EXIT:
	return ret;
}

int CWMCU_bus_read_serial(u8 *data, int len)
{
	int ret = 0;

    mutex_lock(&cwmcu_bus_lock);
#if defined(CWMCU_I2C_INTERFACE)	
	ret = i2c_read_bytes_dma_serial(sensor->client, data, len);

	if (ret < 0) {
		CW_ERROR("i2c read error =%d", ret);
		goto BUS_RS_EXIT;
	}
#elif defined(CWMCU_SPI_INTERFACE)
    ret = spi_read_bytes_serial(data, len);

    if (ret < 0) {
        CW_ERROR("spi read error =%d", ret);
        goto BUS_RS_EXIT;
    }
#endif
    mutex_unlock(&cwmcu_bus_lock);

BUS_RS_EXIT:
	return ret;
}




