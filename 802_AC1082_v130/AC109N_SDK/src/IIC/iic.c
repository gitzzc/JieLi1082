/*--------------------------------------------------------------------------*/
/**@file    iic.c
   @brief   IIC 接口函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/#include "iic.h"
#include "RTC_API.h"
#include "sdmmc_api.h"
#include "play_file.h"
#include "device.h"
#include "IRTC.h"
#include "main.h"

bool iic_busy;
bool eeprom_flag = 1;
extern bool  first_power_on_read_24c02;


__no_init u8 _xdata mem_buf_eeprom[130]; ///<SDMMCA 缓存Buf


__code const u16 iic_io_tab[8] AT(IIC_TABLE_CODE)=
{
    (u16)iic_data_out,
    (u16)iic_data_in,
    (u16)iic_data_r,
    (u16)iic_data,
    (u16)iic_clk_out,
    (u16)iic_clk,
};


/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O 设置输出函数
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_data_out(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_data_out(void) AT(IIC_CODE)
{
    P0DIR &= ~(1<<3);
}


/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O 设置输入函数
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_data_in(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_data_in(void) AT(IIC_CODE)
{
    P0DIR |= (1<<3);
    P0PU |= (1<<3);
}


/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O 读取函数
    @param: void
    @return:读取数值
    @author:Juntham
    @note:  bool iic_data_r(void)
*/
/*----------------------------------------------------------------------------*/
_near_func bool iic_data_r(void) AT(IIC_CODE)
{
    return P03;
}

/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O 输出值设置函数
    @param: flag：输出电平
    @return:void
    @author:Juntham
    @note:  void iic_data(bool flag)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_data(bool flag) AT(IIC_CODE)
{
    P03 = flag;
}

/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O 输出值设置函数
    @param: flag：输出电平
    @return:void
    @author:Juntham
    @note:  void iic_clk_out(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_clk_out(void) AT(IIC_CODE)
{
    P0DIR &= ~(1<<4);
}

/*----------------------------------------------------------------------------*/
/** @brief: IIC dat I/O 输出值设置函数
    @param: flag：输出电平
    @return:void
    @author:Juntham
    @note:  void iic_clk(bool flag)
*/
/*----------------------------------------------------------------------------*/
_near_func void iic_clk(bool flag) AT(IIC_CODE)
{
    P04 = flag;
}

/*----------------SD I/O 复用------------------*/
_near_func void sd_iic_data_out(void) AT(IIC_CODE)
{
    P0DIR &= ~(1<<1);
}

_near_func void sd_iic_data_in(void) AT(IIC_CODE)
{
    P0DIR |= (1<<1);
    P0PU |= (1<<1);
}

_near_func bool sd_iic_data_r(void) AT(IIC_CODE)
{
    return P01;
}

_near_func void sd_iic_data(bool flag) AT(IIC_CODE)
{
    P01 = flag;
}

_near_func void sd_iic_clk_out(void) AT(IIC_CODE)
{
    P3DIR &= ~(1<<5);
}

_near_func void sd_iic_clk(bool flag) AT(IIC_CODE)
{
    P35 = flag;
}
__code const u16 sd_iic_io_tab[8] AT(IIC_TABLE_CODE)=
{
    (u16)sd_iic_data_out,
    (u16)sd_iic_data_in,
    (u16)sd_iic_data_r,
    (u16)sd_iic_data,
    (u16)sd_iic_clk_out,
    (u16)sd_iic_clk,
};
/*----------------------------------------------------------------------------*/
/**@brief   EEPROM 写函数(在IIC 与 SD I/O 复用情况下使用)
   @param   addr ：写入地址
   @param   dat：写入数据
   @note    void sd_write_eeprom(u8 addr, u8 dat)
*/
/*----------------------------------------------------------------------------*/
void sd_write_eeprom(u8 addr, u8 dat) AT(IIC_CODE)
{
    sd_iic_write(0xa0, addr, &dat, 1);
}

/*----------------------------------------------------------------------------*/
/**@brief   EEPROM 读函数(在IIC 与 SD I/O 复用情况下使用)
   @param   addr ：读取地址
   @return  读取数据
   @note    u8 sd_read_eeprom(u8 addr)
*/
/*----------------------------------------------------------------------------*/
u8 sd_read_eeprom(u8 addr) AT(IIC_CODE)
{
    u8 p_status;
    u8 res;


    /*Wait SD I/O release*/
    if (device_active == DEVICE_SDMMCA)
        p_status = pause_decode(1);
    sdmmca_force_idle();
    sdmmcb_force_idle();
    while(check_sd_controller());   //根据SD ID等待控制器繁忙
    SD_CON2 &= ~BIT(0);
    IO_MC0 &= ~BIT(3);

	 sdmmc_ctler_flag = DEVICE_SDMMCA;
    /*IIC Communicate*/

    iic_stop();
    res = read_eerom(addr);
     sdmmc_ctler_flag = 0;
    /*Reset SD I/O*/
    SD_CON2 |= BIT(0);
    IO_MC0 |= BIT(3);
    unlock_cmd_chk();
    if ((p_status == MAD_PLAY)&&(device_active == DEVICE_SDMMCA))
        start_decode();

    //my_puts("0-sd_read_eeprom..\n");
    return res;
}


/*----------------------------------------------------------------------------*/
/**@brief   IIC写数据(在IIC 与 SD I/O 复用情况下使用)
   @param   chip_id ：目标IC的ID号
   @param   iic_addr: 目标IC的目标寄存器的地址
   @param   *iic_dat: 写望目标IC的数据的指针
   @param   n:需要写的数据的数目
   @return  无
   @note    void sd_iic_write(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n)
*/
/*----------------------------------------------------------------------------*/
void sd_iic_write(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n) AT(IIC_CODE)
{
    u8 p_status;


    /*Wait SD I/O release*/
    if (device_active == DEVICE_SDMMCA)
        p_status = pause_decode(1);
    sdmmca_force_idle();
    sdmmcb_force_idle();
    while(check_sd_controller());   //根据SD ID等待控制器繁忙
    SD_CON2 &= ~BIT(0);
    IO_MC0 &= ~BIT(3);
    sdmmc_ctler_flag = DEVICE_SDMMCA;
    /*IIC Communicate*/
    iic_stop();
    iic_write(chip_id, iic_addr, iic_dat, n);
     sdmmc_ctler_flag = 0;
    /*Reset SD I/O*/
    SD_CON2 |= BIT(0);
    IO_MC0 |= BIT(3);
    unlock_cmd_chk();
    if ((p_status == MAD_PLAY)&&(device_active == DEVICE_SDMMCA))
        start_decode();
    delay_n10ms(2);

    //my_puts("1-sd_iic_write..\n");
}

/*----------------------------------------------------------------------------*/
/**@brief   IIC读取N个数据(在IIC 与 SD I/O 复用情况下使用)
   @param   address : 目标ID
   @param   *p     :  存档读取到的数据的buffer指针
   @param   number :  需要读取的数据的个数
   @return  无
   @note    void sd_iic_readn(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n)
*/
/*----------------------------------------------------------------------------*/
void sd_iic_readn(u8 chip_id, u8 iic_addr, u8 *iic_dat, u8 n) AT(IIC_CODE)
{
    u8 p_status;


    /*Wait SD I/O release*/
    if (device_active == DEVICE_SDMMCA)
        p_status = pause_decode(1);
    sdmmca_force_idle();
    sdmmcb_force_idle();
    while(check_sd_controller());   //根据SD ID等待控制器繁忙
    SD_CON2 &= ~BIT(0);
    IO_MC0 &= ~BIT(3);

	sdmmc_ctler_flag = DEVICE_SDMMCA;

    /*IIC Communicate*/
    iic_stop();
    iic_readn(chip_id, iic_addr, iic_dat, n);

    /*Reset SD I/O*/

	 sdmmc_ctler_flag = 0;
	
    SD_CON2 |= BIT(0);
    IO_MC0 |= BIT(3);
    unlock_cmd_chk();
    if ((p_status == MAD_PLAY)&&(device_active == DEVICE_SDMMCA))
        start_decode();

}


/*----------------------------------------------------------------------------*/
/** @brief: IIC 模块初始化函数
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_init(void)
*/
/*----------------------------------------------------------------------------*/
void iic_init(void) AT(IIC_CODE)
{
#ifdef SD_IIC_EN
    //my_puts("SD IIC..\n");
    set_iic_io((u16 __code *)sd_iic_io_tab);    ///<sd_iic_io_tab
#else
    //my_puts("Only IIC..\n");
    set_iic_io((u16 __code *)iic_io_tab);       ///<配置接口函数IO
#endif
    get_iic_busy_status(15);                    ///<设置delay 时间
}

/*----------------------------------------------------------------------------*/
/** @brief: EEPROM 校验
    @param: void
    @return:void
    @author:Juntham
    @note:  void iic_verify(void)
*/
/*----------------------------------------------------------------------------*/
void eeprom_verify(void) AT(IIC_CODE)
{

///#ifdef CHECK_EEPROM_ON_POWER_ON

  //  u8 tttt=0;

  //  my_printf("%04x\n",eeprom_flag);
	
		 //  tttt = get_memory(0);

    // my_printf("%04x\n",tttt);

	  //  tttt = get_memory(1);

		// my_printf("%04x\n",tttt);
	eeprom_flag = 1;

    if ((get_memory(0) != 0x55)||(get_memory(1) != 0xAA))
    {

		// my_printf("%04x\n",eeprom_flag);
		
        set_memory(0, 0x55);
        set_memory(1, 0xAA);
    }
 /*
    tttt = get_memory(0);

	 my_printf("%04x\n",tttt);

	  tttt = get_memory(1);

	 my_printf("%04x\n",tttt);
	*/

    if ((get_memory(0) != 0x55)||(get_memory(1) != 0xAA))
    {
        //外接eeprom无效
#if UART_DEBUG
        set_putchar(my_putchar);
        my_puts("No eeprom..\n");
#endif
        eeprom_flag = 0;
    }
    else
    {
        //有外接eeprom
#if UART_DEBUG
        set_putchar(my_putchar);
        my_puts("Find eeprom\n");
#endif
        eeprom_flag = 1;
		delay_n10ms(10);
		get_all_memory();
    }
///#endif

}
/*----------------------------------------------------------------------------*/
/** @brief: 记忆信息到设备（EEPROM）
    @param: addr:存储地址  dat:要存储的数据
    @return:void
    @author:Juntham
    @note:  void write_SDMMC_UDISK(u8 addr, u8 dat)
*/
/*----------------------------------------------------------------------------*/
_near_func void write_SDMMC_UDISK(u8 addr, u8 dat) AT(COMMON_CODE)
{
    if(dat != mem_buf[addr])
    {
        info_change = 4;
        mem_buf[addr] = dat;
    }
}
/*----------------------------------------------------------------------------*/
/** @brief: 记忆信息到存储器（EEPROM）
    @param: void
    @return:void
    @author:Juntham
    @note:  void set_memory(u8 addr, u8 dat)
*/
/*----------------------------------------------------------------------------*/
_near_func void set_memory(u8 addr, u8 dat) AT(COMMON_CODE)
{
    if(eeprom_flag)
    {
#ifdef UART_PRINT_ENABLE			
	 my_printf("ser info to eeprom \n");
#endif     
    	mem_buf_eeprom[addr] = dat;
        app_E2PROM_write(addr, dat);//write_eerom(addr, dat);
    }
#ifdef USE_IRTC_MEMORY
    else
    {
#ifdef UART_PRINT_ENABLE			
	 my_printf("ser info to ram \n");
#endif    
    write_IRTC_RAM(addr, dat);
    }
#else
    else
    {
        write_SDMMC_UDISK(addr,dat);
    }
#endif
    /*
#ifdef USE_EEPROM_MEMORY
    //my_printf("Addr:%02x/Dat:%02x\n", addr, dat);

#else
    write_IRTC_RAM(addr, dat);
#endif   */
}
/*----------------------------------------------------------------------------*/
/** @brief: 记忆设备断点信息到缓存中
    @param: addr:要将数据写到的地址，dat：要写入的数据
    @return:void
    @author:Juntham
    @note:  void set_memory_breakpoint(u8 addr, u8 dat)
*/
/*----------------------------------------------------------------------------*/
_near_func void set_memory_breakpoint(u8 addr, u8 dat) AT(COMMON_CODE)
{
    ///if(device_active == DEVICE_SDMMCA)
        mem_buf[addr] = dat;
    ///else if(device_active == DEVICE_UDISK)
    ///    mem_buf0[addr] = dat;
}
/*----------------------------------------------------------------------------*/
/** @brief: 获取记忆信息（EEPROM）
    @param: void
    @return:void
    @author:Juntham
    @note:  u8 get_memory(u8 addr)
*/
/*----------------------------------------------------------------------------*/
_near_func u8 get_memory(u8 addr) AT(COMMON_CODE)
{
    if(eeprom_flag)
    {
#ifdef UART_PRINT_ENABLE			
	 my_printf("read info from eeprom \n");
#endif     
        if(first_power_on_read_24c02)     
    	return app_E2PROM_read(addr);//return read_eerom(addr);
    	else
		return mem_buf_eeprom[addr];		
	}
#ifdef USE_IRTC_MEMORY
    else
    {
#ifdef UART_PRINT_ENABLE			
	 my_printf("read info from ram \n");
#endif    
       return read_IRTC_RAM(addr);
    }
#else
    else
        return mem_buf[addr];
#endif
    /*
#ifdef USE_EEPROM_MEMORY
    return app_E2PROM_read(addr);//return read_eerom(addr);
#endif

#ifdef USE_IRTC_MEMORY
    return read_IRTC_RAM(addr);
#endif   */
}

/*----------------------------------------------------------------------------*/
/** @brief: 获取记忆信息（EEPROM）
    @param: void
    @return:void
    @author:Juntham
    @note:  u8 get_memory(u8 addr)
*/
/*----------------------------------------------------------------------------*/
_near_func void get_all_memory(void) AT(COMMON_CODE)
{
	u8 i;
    if(eeprom_flag)
    {
    	for(i=0;i<130;i++)
			mem_buf_eeprom[i] = app_E2PROM_read(i);
	}
}



