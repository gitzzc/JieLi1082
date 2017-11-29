/*--------------------------------------------------------------------------*/
/**@file    device.c
   @brief   设备管理功能接口
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "device.h"
#include "play_file.h"
#include "get_music_file.h"
#include "music_play.h"
#include "sdmmc_api.h"
#include "break_point.h"
#include "clock.h"
#include "usb_host.h"
#include "iic.h"

_no_init u8 _data device_online;			    ///<设备在线标记
_no_init u8 _data device_active;                ///<当前在线活动设备
_no_init bool _bit find_break_point_file_flag;
extern __no_init u8 device_flag;
extern bool eeprom_flag;

//extern __no_init u32 sdmmcA_reserve_lba; ///<SDMMC 保留区的地址
//extern __no_init u32 sdmmcB_reserve_lba; ///<SDMMC 保留区的地址
//extern __no_init u32 udisk_reserve_lba; ///<UDISK 保留区的地址
//extern __no_init u8 info_change;


#ifndef NDEBUG
u8 const _code file_type[] = "WAV";  ///<解码文件格式
#else
u8 const _code file_type[] = "WAVMP?";  ///<解码文件格式
#endif

u8 (_near_func *const disk_read_table[MAX_DEVICE + 1])(u8 _xdata * buf, u32 lba) =
{
#ifdef USB_DISK_EN
    usb_host_read,
#endif

#ifdef SDMMCA_EN
    sdmmcA_read,
#endif

#ifdef SDMMCB_EN
    sdmmcB_read,
#endif
};

/*----------------------------------------------------------------------------*/
/**@brief   检测当前活动的设备
   @param   void
   @return  0：当前活动的设备不在线：其他：当前活动的设备
   @author: Juntham
   @note    u8 device_check(void)
*/
/*----------------------------------------------------------------------------*/
u8 device_check(void) AT(GET_DEVICE)
{
#ifdef SDMMCA_EN
    if (get_sdmmc_online_status(SDMMCA_ID) == 0)
    {
        device_online &= ~BIT(DEVICE_SDMMCA);
    }
    else
        device_online |= BIT(DEVICE_SDMMCA);
#endif

#ifdef SDMMCB_EN
    if (get_sdmmc_online_status(SDMMCB_ID) == 0)
    {
        device_online &= ~BIT(DEVICE_SDMMCB);
    }
    else
        device_online |= BIT(DEVICE_SDMMCB);
#endif

#ifdef USB_DISK_EN
    if (get_usb_online_status() & BIT(0))   //Host online
        device_online |= BIT(DEVICE_UDISK);
    else
        device_online &= ~BIT(DEVICE_UDISK);
#endif

       //my_printf("%02x\n",((BIT(device_active)) & device_online));

    return (BIT(device_active)) & device_online;
}


/*----------------------------------------------------------------------------*/
/**@brief   文件扫描
   @param   void
   @return  0：扫描成功：其他：失败
   @author: Juntham
   @note    u8 fs_get_total_files(void)
*/
/*----------------------------------------------------------------------------*/
u8 fs_get_total_files(void) AT(GET_DEVICE)
{
    /*Maskrom File by number*/
    ff_api.fs_msg.fileNumber = 0x0101;
    fs_getfile_bynumber(&ff_api,ff_api.fs_msg.fileNumber);

    my_memset((u8 _xdata *)&ff_api, 0, sizeof(FF_APIS));

    find_break_point_file_flag = 0;

#ifdef LAST_MEM_FILE_PLAY_EN
    load_music_break_point(device_active);
#endif

    if (music_device_info[device_active].wfileTotal == 0)
    {
#ifdef LAST_MEM_FILE_PLAY_EN
        ff_api.fs_msg.sclust_id = music_break_info.wSclustID;
        ff_api.fs_msg.fsize_id = music_break_info.wFsizeID;
#endif
        //system_clk(48/48);      //为了加快搜索速度，CPU全速运行

        if (fs_scan_disk_app(&ff_api))
        {
            find_break_point_file_flag = 1;
        }
        if (ff_api.fs_msg.fileCounter == 0)
        {
            return 0xfe;
        }
        else
            music_device_info[device_active].wfileTotal = ff_api.fs_msg.fileCounter;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   设备初始化
   @param   无
   @return  0：找到有效设备；其他：没有找到有效设备
   @author: Juntham
   @note    u8 device_init(void)
*/
/*----------------------------------------------------------------------------*/
u8 device_init(u8 dev) AT(GET_DEVICE)
{
  //if (device_check())  ///<当前需要活动的设备在线
  {
        u8 pkt_size = 1;
        u16 rsvdsec = 0;

        fs_ext_setting(file_type);

        if (dev == DEVICE_UDISK)
        {
#ifdef USB_DISK_EN
            delay_n10ms(10);
            /*retry-3/time_out-254/reset_delay-40*/
            if (usb_host_mount_app(3, 254, 4, 10))     ///<USB MASS STORAGE 枚举
            {
                force_usb_suspend();
                return 0xff;             ///<枚举失败
            }

            pkt_size = get_usb_pkt_size();
#else
            return 0xff;
#endif
        }
        else
        {
#if (defined (SDMMCA_EN) || defined (SDMMCB_EN))
            if (identif_sdmmc(dev))                  ///<sd 卡鉴定
            {
                return 0xff;                       ///<鉴定失败
            }
#else
            return 0xff;
#endif
            SD_CON1 |= BIT(6);
        }

        f_init(disk_read_table[dev], pkt_size);

        if (!f_mount(0))                ///<文件系统初始化
        {
            if(eeprom_flag == 0)
            {
                rsvdsec = (((u16)fat_fs.win[15])<<8)|fat_fs.win[14];
                if(DEVICE_UDISK == dev)
                {
                    if((1 == pkt_size)&&(rsvdsec>1)&&(fat_fs.fs_type<4))
                        udisk_reserve_lba = (fat_fs.fatbase - 1)<<fat_fs.secotr_512size;
                    else
                    {
                        udisk_reserve_lba = 0;
                    }
                }
                else
                {
                    if((rsvdsec>1)&&(fat_fs.fs_type<4))
                    {
                        if(DEVICE_SDMMCA == dev)
                        {
                            sdmmcA_reserve_lba = (fat_fs.fatbase - 1)<<fat_fs.secotr_512size;
                        }
                        else
                        {
                            sdmmcB_reserve_lba = (fat_fs.fatbase - 1)<<fat_fs.secotr_512size;
                        }
                    }
                    else
                    {
                        if(DEVICE_SDMMCA == dev)
                        {
                            sdmmcA_reserve_lba = 0;
                        }
                        else
                        {
                            sdmmcB_reserve_lba = 0;
                        }
                    }
                }
                info_change = 1;
                device_flag = 0x80 | BIT(DEVICE_SDMMCA)|BIT(DEVICE_UDISK);
                updata_fat_memory();
            }
            return 0;
        }
        else
            return 0xff;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief   查找指定的设备或自动查找下一个有效设备
   @param   select : 1：查找SD设备；2：查找USB mass storage设备; 0x81:查找上一个设备；0x82:查找下一个设备;其它：无效
   @return  0：成功查找到一个设备；1:指定的设备没有找到；2：只有一个设备，查找上一个设备或下一个设备无效;3:找到设备，但初始化不成功;4：无在线设备
   @author: Juntham
   @note    u8 find_device(u8 select)
*/
/*----------------------------------------------------------------------------*/
u8 find_device(ENUM_DEVICE_SELECT_MODE select) AT(GET_DEVICE)
{
    u8 i;

    /*Maskrom File by number*/
    //fs_getfile_bynumber(&ff_api,ff_api.fs_msg.fileNumber);

    device_check();

    i = get_one_count(device_online & 0x07);

    if (i == 0)				//无设备
    {
        device_active = NO_DEVICE;
        return NO_DEV_ONLINE;
    }

    if (select == DEVICE_AUTO_PREV)		//查找上一个设备
    {
        for (i = 0;i <= MAX_DEVICE;i++)
        {
            device_active--;

            if (device_active > MAX_DEVICE)
            {
            #ifdef SDMMCB_EN
                device_active = DEVICE_SDMMCB;
            #elif defined (SDMMCA_EN)
                device_active = DEVICE_SDMMCA;
            #else
                device_active = DEVICE_UDISK;
            #endif
            }
            if (device_check() == 0)
                continue;

            if (device_init(device_active))             //找到有效设备
                continue;

            if (!fs_get_total_files())
                return FIND_DEV_OK;
        }
        device_active = NO_DEVICE;
        return NO_EFFECTIVE_DEV;				//无有效可以使用的设备
    }
    else if (select == DEVICE_AUTO_NEXT)    //查找下一个设备
    {
        for (i = 0;i <= MAX_DEVICE;i++)
        {
            device_active++;
            if (device_active > MAX_DEVICE)
                device_active = DEVICE_UDISK;

            if (device_check() == 0)
                continue;

            if (device_init(device_active))             //找到有效设备
                continue;

            if (!fs_get_total_files())
                return FIND_DEV_OK;
        }

        device_active = NO_DEVICE;
        return NO_EFFECTIVE_DEV;							//无有效可以使用的设备
    }
    else                //选择指定的设备
    {
        device_active = select;
        if (BIT(select) & device_online)
        {
            if (!device_init(device_active))
            {
                if (!fs_get_total_files())
                {
                    return FIND_DEV_OK;
                }
            }
            {
                return DEV_INIT_ERR;
            }
        }
        else
            return NO_DEFINE_DEV;
    }
}

