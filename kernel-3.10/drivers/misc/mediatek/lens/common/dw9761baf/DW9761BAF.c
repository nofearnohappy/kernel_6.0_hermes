
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "DW9761BAF.h"
#include "../camera/kd_camera_hw.h"

#define LENS_I2C_BUSNUM 0
static struct i2c_board_info __initdata kd_lens_dev={ I2C_BOARD_INFO("DW9761BAF", 0x18)};


#define DW9761BAF_DRVNAME "DW9761BAF"
#define DW9761BAF_VCM_WRITE_ID           0x18

#define DW9761BAF_DEBUG
#ifdef DW9761BAF_DEBUG
#define DW9761BAFDB printk
#else
#define DW9761BAFDB(x,...)
#endif

static spinlock_t g_DW9761BAF_SpinLock;

static struct i2c_client * g_pstDW9761BAF_I2Cclient = NULL;

static dev_t g_DW9761BAF_devno;
static struct cdev * g_pDW9761BAF_CharDrv = NULL;
static struct class *actuator_class = NULL;

static int  g_s4DW9761BAF_Opened = 0;
static long g_i4MotorStatus = 0;
static long g_i4Dir = 0;
static unsigned int g_u4DW9761BAF_INF = 0;
static unsigned int g_u4DW9761BAF_MACRO = 1023;
static unsigned int g_u4TargetPosition = 0;
static unsigned int g_u4CurrPosition   = 0;

static unsigned int g_AF_infinite_Cali = 0;

static int g_sr = 3;

static int i2c_read(u8 a_u2Addr , u8 * a_puBuff)
{
    int  i4RetValue = 0;
    char puReadCmd[1] = {(char)(a_u2Addr)};
    
    
    
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puReadCmd, 1);
    if (i4RetValue < 0) {
        DW9761BAFDB(" I2C write failed!! \n");
        return -1;
    }
    
    i4RetValue = i2c_master_recv(g_pstDW9761BAF_I2Cclient, (char *)a_puBuff, 1);
    if (i4RetValue != 1) {
        DW9761BAFDB(" I2C read failed!! \n");
        return -1;
    }
    
    return 0;
}

#define DW9761B_OTP_WRITE_ID         0xB0
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
inline kal_uint16 DW9761B_read_reg(kal_uint32 addr)
{
    kal_uint16 get_byte = 0;
    
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(puSendCmd , 2, (u8*)&get_byte, 1, DW9761B_OTP_WRITE_ID);
    return get_byte&0x00ff;
}

static u8 read_data(u8 addr)
{
    u8 get_byte=0;
    i2c_read( addr ,&get_byte);
    DW9761BAFDB("[DW9761BAF]  get_byte %d \n",  get_byte);
    return get_byte;
}

static int s4DW9761BAF_ReadReg(unsigned short * a_pu2Result)
{
    
    
    
    *a_pu2Result = (read_data(0x03) << 8) + (read_data(0x04)&0xff);
    
    DW9761BAFDB("[DW9761BAF]  s4DW9761AF_ReadReg %d \n",  *a_pu2Result);
    return 0;
}

static int s4DW9761BAF_WriteReg(u16 a_u2Data)
{
    int  i4RetValue = 0;
    
    char puSendCmd[2] = {0x03,(char)(a_u2Data >> 8)};
    
    DW9761BAFDB("[DW9761BAF]  write %d \n",  a_u2Data);
    
    
    
    
    
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd, 2);
    
    puSendCmd[0] = 0x04;
    puSendCmd[1] = a_u2Data & 0xff;
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd, 2);
    
    if (i4RetValue < 0) 
    {
        DW9761BAFDB("[DW9761BAF] I2C send failed!! \n");
        return -1;
    }
    
    return 0;
}

inline static int getDW9761BAFInfo(__user stDW9761BAF_MotorInfo * pstMotorInfo)
{
    stDW9761BAF_MotorInfo stMotorInfo;
    stMotorInfo.u4MacroPosition   = g_u4DW9761BAF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4DW9761BAF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
    stMotorInfo.bIsSupportSR      = TRUE;
    
    if (g_i4MotorStatus == 1)	{stMotorInfo.bIsMotorMoving = 1;}
    else						{stMotorInfo.bIsMotorMoving = 0;}
    
    if (g_s4DW9761BAF_Opened >= 1)	{stMotorInfo.bIsMotorOpen = 1;}
    else						{stMotorInfo.bIsMotorOpen = 0;}
    
    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stDW9761BAF_MotorInfo)))
    {
        DW9761BAFDB("[DW9761BAF] copy to user failed when getting motor information \n");
    }
    
    return 0;
}

inline static int moveDW9761BAF(unsigned long a_u4Position)
{
    int ret = 0;
    
    if((a_u4Position > g_u4DW9761BAF_MACRO) || (a_u4Position < g_u4DW9761BAF_INF))
    {
        DW9761BAFDB("[DW9761BAF] out of range \n");
        return -EINVAL;
    }
    
    if (g_s4DW9761BAF_Opened == 1)
    {
        
        spin_lock(&g_DW9761BAF_SpinLock);
        g_s4DW9761BAF_Opened = 2;
        spin_unlock(&g_DW9761BAF_SpinLock);
    }
    
    
    spin_lock(&g_DW9761BAF_SpinLock);    
    g_u4TargetPosition = a_u4Position;
    spin_unlock(&g_DW9761BAF_SpinLock);	
    
    DW9761BAFDB("[DW9761BAF] move [curr] %d [target] %d\n", g_u4CurrPosition, g_u4TargetPosition);
    
    spin_lock(&g_DW9761BAF_SpinLock);
    g_sr = 3;
    g_i4MotorStatus = 0;
    spin_unlock(&g_DW9761BAF_SpinLock);	
    
    if(s4DW9761BAF_WriteReg((unsigned short)g_u4TargetPosition) == 0)
    {
        spin_lock(&g_DW9761BAF_SpinLock);		
        g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
        spin_unlock(&g_DW9761BAF_SpinLock);				
    }
    else
    {
        DW9761BAFDB("[DW9761BAF] set I2C failed when moving the motor \n");
        spin_lock(&g_DW9761BAF_SpinLock);
        g_i4MotorStatus = -1;
        spin_unlock(&g_DW9761BAF_SpinLock);				
    }
    
    return 0;
}

inline static int setDW9761BAFInf(unsigned long a_u4Position)
{
    spin_lock(&g_DW9761BAF_SpinLock);
    g_u4DW9761BAF_INF = a_u4Position;
    spin_unlock(&g_DW9761BAF_SpinLock);	
    return 0;
}

inline static int setDW9761BAFMacro(unsigned long a_u4Position)
{
    spin_lock(&g_DW9761BAF_SpinLock);
    g_u4DW9761BAF_MACRO = a_u4Position;
    spin_unlock(&g_DW9761BAF_SpinLock);	
    return 0;	
}

static long DW9761BAF_Ioctl(
    struct file * a_pstFile,
    unsigned int a_u4Command,
    unsigned long a_u4Param)
{
    long i4RetValue = 0;
    
    DW9761BAFDB("[DW9761BAF] CMD = 0x%x \n", a_u4Command);
    switch(a_u4Command)
    {
        case DW9761BAFIOC_G_MOTORINFO :
            i4RetValue = getDW9761BAFInfo((__user stDW9761BAF_MotorInfo *)(a_u4Param));
            break;
            
        case DW9761BAFIOC_T_MOVETO :
            i4RetValue = moveDW9761BAF(a_u4Param);
            break;
            
        case DW9761BAFIOC_T_SETINFPOS :
            i4RetValue = setDW9761BAFInf(a_u4Param);
            break;
            
        case DW9761BAFIOC_T_SETMACROPOS :
            i4RetValue = setDW9761BAFMacro(a_u4Param);
            break;
            
        default :
            DW9761BAFDB("[DW9761BAF] No CMD \n");
            i4RetValue = -EPERM;
            break;
    }
    
    return i4RetValue;
}

#if 1  
static int s4DW9761BAF_WriteReg2(u8 a_u2Addr, u16 a_u2Data)
{
    int  i4RetValue = 0;
    
    char puSendCmd[2] = {0x03,(char)(a_u2Data >> 8)};
    
    DW9761BAFDB("[DW9761BAF]  write addr =%d, data =%d \n",  a_u2Addr,a_u2Data);
 
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd, 2);
    
    puSendCmd[0] = a_u2Addr;
    puSendCmd[1] = a_u2Data & 0xff;
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd, 2);
    
    if (i4RetValue < 0) 
    {
        DW9761BAFDB("[DW9761BAF] I2C send failed!! \n");
        return -1;
    }
    
    return 0;
}
#endif

static int DW9761BAF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    long i4RetValue = 0;
    char puSendCmd2[2] = {0x02,0x01};
    char puSendCmd3[2] = {0x02,0x02};
    char VcmID, reg_11, reg_12;
    DW9761BAFDB("[DW9761BAF] DW9761AF_Open - Start\n");
    
    spin_lock(&g_DW9761BAF_SpinLock);
    
    if(g_s4DW9761BAF_Opened)
    {
        spin_unlock(&g_DW9761BAF_SpinLock);
        DW9761BAFDB("[DW9761BAF] the device is opened \n");
        return -EBUSY;
    }
    
    g_s4DW9761BAF_Opened = 1;
    
    spin_unlock(&g_DW9761BAF_SpinLock);
    
    VcmID = DW9761B_read_reg(0x0005);
    reg_12 = DW9761B_read_reg(0x0012);
    reg_11 = DW9761B_read_reg(0x0011);
    g_AF_infinite_Cali = reg_12 << 8 | reg_11;
    
    DW9761BAFDB("[DW9761BAF] VcmID[%x]\n", VcmID);
    
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd2, 2);
    
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd3, 2);
    msleep(1);
    if (VcmID == 0x01)
    {
        puSendCmd2[0] = 0x06;
        puSendCmd2[1] = 0x61;
        puSendCmd3[0] = 0x07;
        puSendCmd3[1] = 0x24;
    }
    else if (VcmID == 0x07)
    {
        puSendCmd2[0] = 0x06;
        puSendCmd2[1] = 0xA0;
        puSendCmd3[0] = 0x07;
        puSendCmd3[1] = 0xA0;
    }
    DW9761BAFDB("[DW9761BAF] tvib [%x] [%x]\n", puSendCmd2[1], puSendCmd3[1]);
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd2, 2);
    
    i4RetValue = i2c_master_send(g_pstDW9761BAF_I2Cclient, puSendCmd3, 2);
    
    #if 0
    s4DW9761BAF_WriteReg2(0x02,0x02); 
    s4DW9761BAF_WriteReg2(0x06,0x60); 
    s4DW9761BAF_WriteReg2(0x07,0x03); 
    #endif
    
    DW9761BAFDB("[DW9761BAF] DW9761AF_Open - End\n");
    
    return 0;
}

static int DW9761BAF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    DW9761BAFDB("[DW9761BAF] DW9761AF_Release - Start\n");
    
    if (g_s4DW9761BAF_Opened)
    {
        DW9761BAFDB("[DW9761BAF] feee \n");
        s4DW9761BAF_WriteReg(g_AF_infinite_Cali+50);
        msleep(27);
        s4DW9761BAF_WriteReg(g_AF_infinite_Cali);
        msleep(27);
        s4DW9761BAF_WriteReg(g_AF_infinite_Cali-50);
        msleep(27);
        s4DW9761BAF_WriteReg(g_AF_infinite_Cali-100);
        msleep(27);
        s4DW9761BAF_WriteReg(g_AF_infinite_Cali-150);
        msleep(27);
        s4DW9761BAF_WriteReg(g_AF_infinite_Cali-200);
        msleep(27);
        s4DW9761BAF_WriteReg(0);
        spin_lock(&g_DW9761BAF_SpinLock);
        g_s4DW9761BAF_Opened = 0;
        spin_unlock(&g_DW9761BAF_SpinLock);
        
    }
    
    DW9761BAFDB("[DW9761BAF] DW9761AF_Release - End\n");
    
    return 0;
}

static const struct file_operations g_stDW9761BAF_fops = 
{
    .owner = THIS_MODULE,
    .open = DW9761BAF_Open,
    .release = DW9761BAF_Release,
    .unlocked_ioctl = DW9761BAF_Ioctl,
    .compat_ioctl    = DW9761BAF_Ioctl
};

inline static int Register_DW9761BAF_CharDrv(void)
{
    struct device* vcm_device = NULL;
    
    DW9761BAFDB("[DW9761BAF] Register_DW9761AF_CharDrv - Start\n");
    
    
    if( alloc_chrdev_region(&g_DW9761BAF_devno, 0, 1,DW9761BAF_DRVNAME) )
    {
        DW9761BAFDB("[DW9761BAF] Allocate device no failed\n");
        
        return -EAGAIN;
    }
    
    
    g_pDW9761BAF_CharDrv = cdev_alloc();
    
    if(NULL == g_pDW9761BAF_CharDrv)
    {
        unregister_chrdev_region(g_DW9761BAF_devno, 1);
        
        DW9761BAFDB("[DW9761BAF] Allocate mem for kobject failed\n");
        
        return -ENOMEM;
    }
    
    
    cdev_init(g_pDW9761BAF_CharDrv, &g_stDW9761BAF_fops);
    
    g_pDW9761BAF_CharDrv->owner = THIS_MODULE;
    
    
    if(cdev_add(g_pDW9761BAF_CharDrv, g_DW9761BAF_devno, 1))
    {
        DW9761BAFDB("[DW9761BAF] Attatch file operation failed\n");
        
        unregister_chrdev_region(g_DW9761BAF_devno, 1);
        
        return -EAGAIN;
    }
    
    actuator_class = class_create(THIS_MODULE, "actuatordrv_dw9761baf");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        DW9761BAFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }
    
    vcm_device = device_create(actuator_class, NULL, g_DW9761BAF_devno, NULL, DW9761BAF_DRVNAME);
    
    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    DW9761BAFDB("[DW9761BAF] Register_DW9761AF_CharDrv - End\n");    
    return 0;
}

inline static void Unregister_DW9761BAF_CharDrv(void)
{
    DW9761BAFDB("[DW9761BAF] Unregister_DW9761AF_CharDrv - Start\n");
    
    
    cdev_del(g_pDW9761BAF_CharDrv);
    
    unregister_chrdev_region(g_DW9761BAF_devno, 1);
    
    device_destroy(actuator_class, g_DW9761BAF_devno);
    
    class_destroy(actuator_class);
    
    DW9761BAFDB("[DW9761BAF] Unregister_DW9761AF_CharDrv - End\n");    
}


static int DW9761BAF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int DW9761BAF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id DW9761BAF_i2c_id[] = {{DW9761BAF_DRVNAME,0},{}};   
struct i2c_driver DW9761BAF_i2c_driver = {                       
    .probe = DW9761BAF_i2c_probe,                                   
    .remove = DW9761BAF_i2c_remove,                           
    .driver.name = DW9761BAF_DRVNAME,                 
    .id_table = DW9761BAF_i2c_id,                             
};  

#if 0 
static int DW9761BAF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {         
strcpy(info->type, DW9761BAF_DRVNAME);                                                         
return 0;                                                                                       
}      
#endif 
static int DW9761BAF_i2c_remove(struct i2c_client *client) {
    return 0;
}

static int DW9761BAF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int i4RetValue = 0;
    
    DW9761BAFDB("[DW9761BAF] DW9761AF_i2c_probe\n");
    
    
    g_pstDW9761BAF_I2Cclient = client;
    g_pstDW9761BAF_I2Cclient->addr = DW9761BAF_VCM_WRITE_ID;
    g_pstDW9761BAF_I2Cclient->addr = g_pstDW9761BAF_I2Cclient->addr >> 1;
    
    
    i4RetValue = Register_DW9761BAF_CharDrv();
    
    if(i4RetValue){
        
        DW9761BAFDB("[DW9761BAF] register char device failed!\n");
        
        return i4RetValue;
    }
    
    spin_lock_init(&g_DW9761BAF_SpinLock);
    
    DW9761BAFDB("[DW9761BAF] Attached!! \n");
    
    return 0;
}

static int DW9761BAF_probe(struct platform_device *pdev)
{
    printk("<%s:%d>\n", __func__, __LINE__);    
    return i2c_add_driver(&DW9761BAF_i2c_driver);
    
    
}

static int DW9761BAF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&DW9761BAF_i2c_driver);
    return 0;
}

static int DW9761BAF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int DW9761BAF_resume(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver g_stDW9761BAF_Driver = {
    .probe		= DW9761BAF_probe,
    .remove	= DW9761BAF_remove,
    .suspend	= DW9761BAF_suspend,
    .resume	= DW9761BAF_resume,
    .driver		= {
        .name	= "lens_actuator_dw9761baf",
        .owner	= THIS_MODULE,
    }
};
static struct platform_device g_stDW9761BAF_device = {
    .name = "lens_actuator_dw9761baf",
    .id = 0,
    .dev = {}
};
static int __init DW9761BAF_i2C_init(void)
{
    printk("<%s:%d>\n", __func__, __LINE__);
    i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
    printk("<%s:%d>\n", __func__, __LINE__);	
    if(platform_device_register(&g_stDW9761BAF_device)){
        DW9761BAFDB("failed to register DW9761BAF driver\n");
        return -ENODEV;
    }
    if(platform_driver_register(&g_stDW9761BAF_Driver)){
        DW9761BAFDB("failed to register DW9761BAF driver\n");
        return -ENODEV;
    }
    
    return 0;
}

static void __exit DW9761BAF_i2C_exit(void)
{
    platform_driver_unregister(&g_stDW9761BAF_Driver);
}

module_init(DW9761BAF_i2C_init);
module_exit(DW9761BAF_i2C_exit);

MODULE_DESCRIPTION("DW9761BAF lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");


