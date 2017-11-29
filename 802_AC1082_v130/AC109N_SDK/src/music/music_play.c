/*--------------------------------------------------------------------------*/
/**@file    music_play.c
   @brief   音乐模式主循环
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/

#include "music_play.h"
#include "play_file.h"
#include "key.h"
#include "device.h"
#include "hot_msg.h"
#include "get_music_file.h"
#include "main.h"
#include "iic.h"
#include "sdmmc_api.h"
#include "usb_host.h"
#include "break_point.h"
#include "UI_API.h"
#include "dac.h"
#include "clock.h"
#include "mic.h"

//必须在公共区
__root const u8 EQTableCode[5][10] =
{
   // {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},    //NORMAL
   // {0xff,0x81,0x11,0x11,0x11,0x11,0x11,0xcd,0x1f,0xff},    //POP
   // {0x11,0x8f,0xff,0xff,0xff,0xff,0xff,0xde,0xef,0xff},    //ROCK
   // {0xcc,0x81,0x14,0x8a,0xac,0xff,0xff,0xff,0xff,0xff},    //JAZZ
   // {0x11,0x8f,0xf8,0x11,0x11,0x11,0x11,0x11,0x11,0xff},    //CLASSIC

    {0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88},    //RMAL
    {0xff,0xb9,0x87,0x65,0x54,0x56,0x78,0x9b,0xdf,0xcb},    //POP
    {0xa8,0xbc,0xce,0xff,0x46,0xb9,0x89,0xcc,0xff,0xbd},    //ROCK
    {0x80,0xc0,0xf0,0x88,0xff,0xba,0xaa,0xcc,0xdd,0xab},    //JAZZ
    {0xaf,0xff,0xa8,0x76,0x86,0xf6,0xa6,0x88,0xcc,0x9b},    //CLASSIC
#if 0
    {0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88},    //NORMAL
    {0xff,0x81,0x11,0x11,0x11,0x11,0x11,0xcd,0x1f,0xff},    //POP
    {0x11,0x8f,0xff,0xff,0xff,0xff,0xff,0xde,0xef,0xff},    //ROCK
    {0xcc,0x81,0x14,0x8a,0xac,0xff,0xff,0xff,0xff,0xff},    //JAZZ
    {0x11,0x8f,0xf8,0x11,0x11,0x11,0x11,0x11,0x11,0xff},    //CLASSIC
#endif
};///<DECODER音效表


_no_init MUSIC_PLAY_VAR _idata Music_Play_var;
_no_init DEVICE_INFO _idata music_device_info[MAX_DEVICE + 1];
extern _no_init u16 _data input_10;
 _no_init u16  countnum;
extern _no_init u16 _data input_100;
extern	_no_init  bool	_bit  first;

_no_init u8 _data play_delay_time;

extern _no_init bool first_in_mp3;
extern bool eeprom_flag;
extern u8 uabcard_out;
u8 next_end_cnt = 0;
u8 _xdata last_msg;
#ifdef AD_MIC_DET
extern bool kalaok_button;
#endif

/*----------------------------------------------------------------------------*/
/** @brief: Music 模式信息初始化
    @param: void
    @return:void
    @author:Juntham
    @note:  void music_app_init(void)
*/
/*----------------------------------------------------------------------------*/
static void music_info_init(void) AT(MUSIC_PLAY)
{
    /*------------System configuration-----------*/
    Sys_IRInput = 1;
    Sys_Volume = 1;

    device_active = NO_DEVICE;

    if (playfile.given_device == NO_DEVICE)
    {
        playfile.given_device = (ENUM_DEVICE_SELECT_MODE)get_memory(MEM_ACTIVE_DEV);
    }

    if (playfile.given_device == DEVICE_SDMMCA)
    {
        playfile.given_file_method = PLAY_BREAK_POINT;
        playfile.given_device = DEVICE_SDMMCA;
        put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
    }
#ifdef SDMMCB_EN
    else if (playfile.given_device == DEVICE_SDMMCB)
    {
        playfile.given_file_method = PLAY_BREAK_POINT;
        playfile.given_device = DEVICE_SDMMCB;
        put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
    }
#endif
    else if (playfile.given_device == DEVICE_UDISK)
    {
        playfile.given_file_method = PLAY_BREAK_POINT;
        playfile.given_device = DEVICE_UDISK;
        put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
    }
    else
    {
        playfile.given_device = DEVICE_SDMMCA;
        playfile.given_file_method = PLAY_FIRST_FILE;
        put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
    }

    /*--------Music UI*/
    SET_UI_MAIN(MENU_MUSIC_MAIN);
    //UI_menu(MENU_WAIT);
    key_table_sel(0);
}
extern bool  muteflag;
/*----------------------------------------------------------------------------*/
/** @brief: 文件播放函数
    @param: file_break_point：断点记忆控制位
    @return:播放成功标志位
    @author:Juntham
    @note:  bool play_device_file(bool file_break_point)
*/
/*----------------------------------------------------------------------------*/
bool play_device_file(bool file_break_point) AT(MUSIC_PLAY)
{
    bool bres;

    init_decode_api(f_read, f_seek);
    f_open(&music_file, &ff_api.dir_info);
    bres = decode_init(file_break_point);

    /*Main UI recovery*/
    SET_UI_MAIN(MENU_MUSIC_MAIN);
    if (bres)                                       //播放成功
    {
        set_eq(1, Music_Play_var.bEQ);
#ifdef MP3_SPECTRUM
        MP_CON3 |= BIT(2);
#endif
        start_decode();
        delay_n10ms(4);
		if(!muteflag)
		{
        dac_mute(0);
		}
		play_delay_time = 0;
        return true;
    }

    return false;
}
/*----------------------------------------------------------------------------*/
/** @brief: EQ 音效设置函数
    @param: eq_en：EQ使能控制位
    @param: eq：EQ序号
    @return:void
    @author:Juntham
    @note:  void set_eq(bool eq_en, u8 eq)
*/
/*----------------------------------------------------------------------------*/
void set_eq(bool eq_en, u8 eq) AT(MUSIC_PLAY)
{
    if (eq_en)
    {
        dec_msg.eq_table = EQTableCode[eq];
        MP_CON3 |= BIT(1);
    }
    else
        MP_CON3 &= ~BIT(1);
}

/*----------------------------------------------------------------------------*/
/** @brief: Music 模式主循环
    @param: void
    @return:void
    @author:Juntham
    @note:  void music_app_loop(void)
*/
/*----------------------------------------------------------------------------*/
void music_app_loop(void) AT(MUSIC_PLAY)
{
    u8 res;
    bool bres;
	 //MUTE_OFF;
	
     dac_mute(0);
    while (1)
    {
        u8 key;

        sdmmca_idle_mode();     //SDMMC进入IDLE待机模式。
        sdmmcb_idle_mode();

        key = app_get_msg();
        // my_printf(" %04u\n",key);
        switch (key)
        {
        case MSG_CHANGE_WORK_MODE:
//#if defined BREAK_POINT_PLAY_EN && defined USE_EEPROM_MEMORY
#if defined BREAK_POINT_PLAY_EN && !defined USE_IRTC_MEMORY
            save_music_break_point(device_active);
#endif
            if(eeprom_flag == 0)
            {
                info_change = 1;
                updata_fat_memory();
	        }
            return;

        case MSG_MUSIC_NEW_DEVICE_IN:							//有新设备接入
//#if defined BREAK_POINT_PLAY_EN && defined USE_EEPROM_MEMORY
#if defined BREAK_POINT_PLAY_EN && !defined USE_IRTC_MEMORY
            save_music_break_point(device_active);
#endif
        case MSG_MUSIC_SELECT_NEW_DEVICE:					    //重新选择设备
 #ifdef UART_PRINT_ENABLE			
						//my_puts("sel dev  \n");
#endif	
            UI_menu(MENU_WAIT);
            stop_decode();
            res = find_device(playfile.given_device);
#ifdef UART_PRINT_ENABLE			
									//my_puts("no dev111  \n");
#endif	

			//my_printf("%04x\n",res);

            if(res != 0)
			 res = find_device(playfile.given_device);
	#ifdef UART_PRINT_ENABLE			
									//my_puts("no dev22  \n");
#endif			
			  if(res != 0)
            res = find_device(playfile.given_device);
			
			
			
            if ((res == DEV_INIT_ERR) ||
                    (res == NO_DEFINE_DEV))                    //指定的设备不在线，或初始化失败
            {
                playfile.given_device = DEVICE_AUTO_NEXT;			   //自动选择下一个设备
                put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
                break;
            }
            else if ((res == NO_EFFECTIVE_DEV) ||
                     (res == NO_DEV_ONLINE))                    //无可播放的设备
            {
#ifdef UART_PRINT_ENABLE			
						my_puts("no dev  \n");
#endif	

#ifdef AD_MIC_DET
			    if (kalaok_button)
				 {
                     work_mode = SPE_MODE;
					 return;
				 }
                else
#endif					
                {
					work_mode = FM_RADIO_MODE;
					 return;
                }
            }
            //找到可用设备
        case MSG_MUSIC_SELECT_NEW_FILE:
            if (device_check() == 0)                                //当前播放设备已经不在线,则自动选取下一个设备
            {
                playfile.given_device = DEVICE_AUTO_NEXT;
                put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
                break;
            }

            res = fs_get_filenum(playfile.play_mode, playfile.given_file_method);

            if (res == GET_DEVICE_END)								//下一曲到设备的结束，查找下一个设备的第一首
            {
                next_end_cnt=0;
                playfile.given_file_method = PLAY_FIRST_FILE;
                playfile.given_device = DEVICE_AUTO_NEXT;
                put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
                //music_device_info[device_active].wBreak_point_filenum = 1;
                clear_music_break_point(device_active);
                break;
            }
            else if (res == GET_DEVICE_HEAD)								//上一曲到设备的结束，查找下一个设备的最后一首
            {
                playfile.given_file_method = PLAY_LAST_FILE;
                playfile.given_device = DEVICE_AUTO_PREV;
                put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
                break;
            }

        case MSG_MUSIC_PLAY_NEW_FILE:
            stop_decode();
            bres = fs_getfile_bynumber_app(&ff_api, playfile.given_file_number);
            playfile.given_file_number = ff_api.fs_msg.fileNumber;
            if (!bres)
            {
                if (playfile.given_file_method <= PLAY_BREAK_POINT) 		//重新找下一首
                {
                    playfile.given_file_method = PLAY_NEXT_FILE;
                    put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
                }
                else											//重新找上一首
                {
                    playfile.given_file_method = PLAY_PREV_FILE;
                    put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
                }
                break;
            }
            flush_all_msg();
            UI_menu(MENU_FILENUM);
#ifdef AD_MIC_DET
        	if (kalaok_button)
			 {
			    dac_channel_sel_mic(DAC_DECODER);
				 dac_mic_control(1);
			 }	
#endif			
            if (playfile.given_file_method == PLAY_BREAK_POINT)
            {
                bres = play_device_file(1);
            }
            else
            {
                bres = play_device_file(0);
            }
            if (!bres)
            {
                if (playfile.given_file_method <= PLAY_BREAK_POINT) 		//重新找下一首
                {
                    playfile.given_file_method = PLAY_NEXT_FILE;
                    put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
                }
                else											            //重新找上一首
                {
                    playfile.given_file_method = PLAY_PREV_FILE;
                    put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
                }
                break;
            }
            else
            {
                set_memory(MEM_ACTIVE_DEV, device_active);     //设备记忆
                //my_printf("0-Get Device %02x\n", get_memory(MEM_ACTIVE_DEV));
                music_device_info[device_active].wBreak_point_filenum = playfile.given_file_number;

		///info_change = 4;
                save_music_break_point(device_active);
                /*
#ifdef LAST_MEM_FILE_PLAY_EN
#ifdef USE_EEPROM_MEMORY
                save_music_break_point(device_active);
#endif
#ifdef USE_IRTC_MEMORY
                save_music_break_point_rtc_ram(device_active);
#endif
#endif*/
            }
#ifdef UART_PRINT_ENABLE			
	 my_printf("playfile.given_file_number =%d\n",playfile.given_file_number);
#endif				
            break;

        case MSG_MUSIC_CONTINUE_PLAY_FILE:                      //继续播放文件
            play_device_file(1);
            break;
			
		case MSG_100MS:
		/*if (uabcard_out)
		   {
			  uabcard_out--;
			  if (uabcard_out==0)
			  put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);			
		   }
		if (next_end_cnt)
			{
			  next_end_cnt--;
			  if (next_end_cnt==0)
			  put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
		   }*/
		break;

        case MSG_DECODE_FILE_END:                               //解码自动至下一首

		    if(dec_msg.play_time < 5)
            {
            	if(play_delay_time == 0)
					play_delay_time = 5;
				break;
            }
            if(((get_music_play_time()==0xffff)/*||(get_music_play_time()<2)*/)&&(last_msg ==MSG_MUSIC_PREV_FILE ))
            {
                put_msg_lifo(MSG_MUSIC_PREV_FILE);
            }
            else if(((get_music_play_time()==0xffff)/*||(get_music_play_time()<2)*/)&&(last_msg ==MSG_MUSIC_NEXT_FILE ))
            {
                put_msg_lifo(MSG_MUSIC_NEXT_FILE);
            }
            else
            {
                playfile.given_file_method = PLAY_AUTO_NEXT;
                last_msg=0;
                put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
            }

		
           // playfile.given_file_method = PLAY_AUTO_NEXT;
           // put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
            break;

        case MSG_MUSIC_PREV_FILE:												//选择上一个文件进行播放;
               muteflag = 0;
			last_msg=MSG_MUSIC_PREV_FILE;
			playfile.given_file_method = PLAY_PREV_FILE;
            put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
            break;

        case MSG_MUSIC_NEXT_FILE:												//选择下一个文件进行播放
               muteflag = 0;
			last_msg=MSG_MUSIC_NEXT_FILE;
			playfile.given_file_method = PLAY_NEXT_FILE;
            put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
            break;

        case MSG_MUSIC_PP:	                                                    //暂停和播放
#ifdef UI_ENABLE										
            if (UI_var.bCurMenu == MENU_INPUT_NUMBER)
            {
                put_msg_lifo(MSG_INPUT_TIMEOUT);
                break;
            }
#endif
            if (Music_Play_var.bPlayStatus == MAD_PLAY)
            {
                //dac_mute(1);
                pause_decode(1);
                USB_read_remain();
                SET_UI_MAIN(MENU_PAUSE);
                UI_menu(MENU_PAUSE);
				Music_Play_var.bPlayStatus= MAD_PAUSE;
			    if (kalaok_button)
				 {
				   dac_channel_sel_mic(DAC_AMUX1_R);//(DAC_AMUX0_L);	
				   set_analog_vol(28, 28);
				 }
                else
                {
					dac_mute(1);
                }				
            }
            else if (Music_Play_var.bPlayStatus== MAD_PAUSE)
            {
                music_fffr(0xFF, 0);
                start_decode();
                SET_UI_MAIN(MENU_MUSIC_MAIN);
                UI_menu(MENU_MUSIC_MAIN);
				Music_Play_var.bPlayStatus= MAD_PLAY;
			    dac_channel_sel_mic(DAC_DECODER); 	
				dac_mute(0);
				if (kalaok_button)
				 {
					dac_mic_control(1);
				 } 				
            }
            break;

        case MSG_MUSIC_FF:
            music_fffr(MAD_FAST_FORWARD, get_1s_musice()*2);
            UI_menu(MENU_HALF_SEC_REFRESH);
            break;

        case MSG_MUSIC_FR:
            music_fffr(MAD_FAST_REVERASE, get_1s_musice()*2);
            UI_menu(MENU_HALF_SEC_REFRESH);
            break;

        case MSG_MUSIC_NEXT_EQ:
            if (Music_Play_var.bPlayStatus == MAD_PAUSE)
                break;

            Music_Play_var.bEQ++;
            if (Music_Play_var.bEQ > CLASSIC)
            {
                Music_Play_var.bEQ = NORMAL;
            }
            set_eq(1, Music_Play_var.bEQ);
            UI_menu(MENU_EQ);
            break;

        case MSG_NEXT_PLAYMODE:
            playfile.play_mode++;
            if (playfile.play_mode >= MAX_PLAY_MODE)
            {
                playfile.play_mode = REPEAT_ALL;
            }
            UI_menu(MENU_PLAYMODE);
            break;

        case MSG_HALF_SECOND:


		 if(play_delay_time)
			{
				play_delay_time--;
			}
			if(play_delay_time == 1)
			{
            	playfile.given_file_method = PLAY_AUTO_NEXT;
            	put_msg_lifo(MSG_MUSIC_SELECT_NEW_FILE);
			}



			
	    if(eeprom_flag == 0)
            	updata_fat_memory();
            LED_FADE_OFF();
            UI_menu(MENU_MAIN);
#ifdef BREAK_POINT_PLAY_EN
#ifndef USE_EEPROM_MEMORY
            if (Music_Play_var.bPlayStatus == MAD_PLAY)                            //每半秒将断点信息写入到内部 RTC RAM 中，
                save_music_break_point_rtc_ram(device_active);
#endif
#endif
            UI_menu(MENU_HALF_SEC_REFRESH);
            break;

        case MSG_INPUT_TIMEOUT:
            /*由红外界面返回*/

             countnum = input_number+input_10+input_100;

			 // my_printf("countnum = %04u\n",countnum);
			
            if ((countnum <= music_device_info[device_active].wfileTotal)&&(countnum != 0))  //input_number
            {
               muteflag = 0;
                playfile.given_file_number = countnum;
                playfile.given_file_method = PLAY_FIRST_FILE;
                put_msg_lifo(MSG_MUSIC_PLAY_NEW_FILE);
            }
            else
                UI_menu(MENU_MUSIC_MAIN);

            input_number = 0;
			input_10=0;
			countnum=0;
			input_100=0;
            break;
		case  MSG_USB_TO_SD:
			device_check();
#ifdef UART_PRINT_ENABLE			
				my_printf("device_online =%d\n",device_online);
#endif			
                if((device_online&0x03) == 0x03)
                {
                    muteflag = 0;
                    if (device_active == DEVICE_UDISK)
                    {
                        put_msg_lifo(MSG_SDMMCA_IN);
                        break;
                    }
					else
					{
					   put_msg_lifo(MSG_USB_DISK_IN);
                        break;
					}
                }			
			break;

        default :
            ap_handle_hotkey(key);
            break;
        }
    }
}

/*----------------------------------------------------------------------------*/
/** @brief: Music 模式
    @param: void
    @return:void
    @author:Juntham
    @note:  void music_app(void)
*/
/*----------------------------------------------------------------------------*/
extern  bool  muteflag;

void music_app(void) AT(MUSIC_PLAY)
{
#ifdef SDMMCA_DET_CLK
#ifdef SDMMCA_EN
    //P2DIR &=  ~0x01;
     P3DIR &=  ~0x20;
#endif
#ifdef SDMMCB_EN
    P3DIR &=  ~0x20;
#endif
    set_sd_clk = 1;
#endif
    muteflag = 0;
    first = 1;
    system_clk_div(CLK_24M);
    if(eeprom_flag == 0)
    	clear_reserve_lba();
	// if(eeprom_flag == 0)
    // power_up_device_info(); //从设备读出信息
    music_info_init();
	 main_vol_R = main_vol_L;
	// my_puts("mp3_ok\n");
	 play_delay_time = 0;
	 // my_printf(" %04u\n",main_vol_R);
    dac_channel_sel(DAC_DECODER);
    if(first_in_mp3 == 0)
    {

        if(eeprom_flag == 0)
        	{
#ifndef USE_IRTC_MEMORY			
            power_up_device_info();
#endif
        	}
    }
    else
        first_in_mp3 = 0;
	//dac_mute(0);
	dac_on_asm();
	uabcard_out=0;
    music_app_loop();
	dac_mute(1);
#ifdef AD_MIC_DET	
	if(kalaok_button)
	{
		dac_mic_control(0);
		dac_mic_gain_control(0);
	}	
#endif
	delay_n10ms(10);
    stop_decode();
    udisk_force_idle();
    sdmmca_force_idle();
    sdmmcb_force_idle();
    playfile.given_device = NO_DEVICE;
	if(eeprom_flag == 0)
    	clear_reserve_lba();

#ifdef SDMMCA_DET_CLK
    IO_MC0 &= ~(BIT(1)|BIT(0));
    set_sd_clk = 0;
#endif
}
