/*--------------------------------------------------------------------------*/
/**@file    FM_radio.c
   @brief   FM 模式主循环
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "fm_radio.h"
#include "FM_API.h"
#include "hot_msg.h"
#include "msg.h"
#include "key.h"
#include "main.h"
#include "iic.h"
#include "UI_API.h"
#include "music_play.h"
#include "tff.h"
#include "dac.h"
#include "clock.h"

extern ENUM_WORK_MODE _data work_mode;
_no_init FM_MODE_VAR _data fm_mode_var;
_no_init u8 _data scan_mode;
extern bool  sdcheck_flag;
/*----------------------------------------------------------------------------*/
/**@brief    FM 模式主循环
   @param    无
   @return   无
   @note     void fm_play(void)
*/
/*----------------------------------------------------------------------------*/
void fm_play(void) AT(FM_CODE)
{
    u8 scan_counter;

    while (1)
    {
        u8 key = app_get_msg();

        switch (key)
        {
        case MSG_MUSIC_NEW_DEVICE_IN:
            work_mode = MUSIC_MODE;
        case MSG_CHANGE_WORK_MODE:
            return;

//        case MSG_STOP_SCAN:
//            scan_counter = 1;                   //再搜索一个频点就停止
//            break;

        case MSG_FM_SCAN_ALL_INIT:
            if (scan_mode == FM_SCAN_STOP)
            {
                set_memory(MEM_CHAN, 0);
        		set_memory(MEM_FRE, 0);
                clear_all_fm_point();
                fm_mode_var.bTotalChannel = 0;
                fm_mode_var.bFreChannel = 0;
                fm_mode_var.wFreq = MIN_FRE - 1;					//自动搜索从最低的频点开始
                scan_counter = MAX_CHANNL;
                scan_mode = FM_SCAN_ALL;
            }
            else
            {
                scan_counter = 1;                   //再搜索一个频点就停止
            }

        case MSG_FM_SCAN_ALL:
            flush_all_msg();
            if (fm_scan(scan_mode))
            {
                if (scan_mode == FM_SCAN_ALL)
                    delay_n10ms(80);                               //播放1S
                else
                {
                    scan_mode = FM_SCAN_STOP;
                    break;
                }
            }

            scan_counter--;
            if (scan_counter == 0)
            {
                if (scan_mode == FM_SCAN_ALL)                 //全频点搜索结束，播放第一个台
                {
                    if (fm_mode_var.bTotalChannel == 0)
                    {
                        fm_mode_var.wFreq = MIN_FRE;				
                        set_fre(FM_CUR_FRE);
                        set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
                        set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
                        fm_module_mute(0);
                        dac_mute(0);
						scan_mode = FM_SCAN_STOP;				
                        break;
                    }

                    put_msg_lifo(MSG_FM_NEXT_STATION);
                    scan_mode = FM_SCAN_STOP;
                }
                else                            //半自动搜索，播放当前频点
                {
                    scan_mode = FM_SCAN_STOP;
                    fm_module_mute(0);
	                dac_mute(0);
                    break;
                }
            }
            else                                                    //搜索过程中只响应事件
            {
                put_msg_lifo(MSG_FM_SCAN_ALL);
            }
			break;

        case MSG_FM_SCAN_ALL_DOWN:
            scan_mode = FM_SCAN_PREV;		
            put_msg_lifo(MSG_FM_SCAN_ALL);
            break;

        case MSG_FM_SCAN_ALL_UP:
            scan_mode = FM_SCAN_NEXT;
            put_msg_lifo(MSG_FM_SCAN_ALL);
			break;

        case MSG_FM_PREV_STEP:
            flush_all_msg();
            set_fre(FM_FRE_DEC);
            fm_mode_var.bFreChannel = get_channel_via_fre(fm_mode_var.wFreq - MIN_FRE);						//查找该频点是否有记忆过
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
			fm_module_mute(0);
			UI_menu(MENU_FM_MAIN);
			dac_mute(0);
            break;

        case MSG_FM_NEXT_STEP:
            flush_all_msg();
            set_fre(FM_FRE_INC);
            fm_mode_var.bFreChannel = get_channel_via_fre(fm_mode_var.wFreq - MIN_FRE);						//查找该频点是否有记忆过
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
			fm_module_mute(0);
			UI_menu(MENU_FM_MAIN);
			dac_mute(0);
            break;

        case MSG_FM_PREV_STATION:
            flush_all_msg();
            if (fm_mode_var.bTotalChannel <= 1)
            {
               put_msg_lifo(MSG_FM_SCAN_ALL_DOWN);
                break;
            }
            fm_mode_var.bFreChannel -= 2;
        case MSG_FM_NEXT_STATION:
            if (fm_mode_var.bTotalChannel <= 1)
            {
               put_msg_lifo(MSG_FM_SCAN_ALL_UP);
                break;
            }

			dac_mute(1);
            fm_mode_var.bFreChannel++;

            if ((fm_mode_var.bFreChannel == 0) || (fm_mode_var.bFreChannel == 0xff))
            {
                fm_mode_var.bFreChannel = fm_mode_var.bTotalChannel;
            }
            else if (fm_mode_var.bFreChannel > fm_mode_var.bTotalChannel)
            {
                fm_mode_var.bFreChannel = 1;
            }
            fm_mode_var.wFreq = get_fre_via_channle(fm_mode_var.bFreChannel) + MIN_FRE;				//根据台号找频点
            set_fre(FM_CUR_FRE);
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
			// my_printf(" %04u     ",fm_mode_var.wFreq);
			 //my_printf(" %04u\n",fm_mode_var.bFreChannel);
			fm_module_mute(0);
		    dac_mute(0);
			UI_menu(MENU_FM_CHANNEL);
            break;

        case MSG_CH_SAVE:
            ch_save();
            break;

        case MSG_HALF_SECOND:
            LED_FADE_OFF();
            UI_menu(MENU_MAIN);
            break;

        case MSG_MUSIC_PP:


			
			put_msg_lifo(MSG_MUTE);

#ifdef UI_ENABLE

            if (UI_var.bCurMenu == MENU_INPUT_NUMBER)   //暂停和播放
            {
                put_msg_lifo(MSG_INPUT_TIMEOUT);
                break;
            }
#else
            break;
#endif  

        case MSG_INPUT_TIMEOUT:
            /*由红外界面返回*/
            if (input_number <= MAX_CHANNL)							//输入的是台号
            {
                key = get_fre_via_channle(input_number);
                if (key != 0xff)
                {
                    fm_mode_var.wFreq = key + MIN_FRE;
                    fm_mode_var.bFreChannel = input_number;
                    set_fre(FM_CUR_FRE);
                    fm_module_mute(0);
                    UI_menu(MENU_FM_DISP_FRE);
                }
            }
            else if ((input_number >= MIN_FRE) && (input_number <= MAX_FRE)) //输入的是频点
            {
                fm_mode_var.wFreq = input_number;
                fm_mode_var.bFreChannel = get_channel_via_fre(fm_mode_var.wFreq - MIN_FRE);
                set_fre(FM_CUR_FRE);
                fm_module_mute(0);
            }
            input_number = 0;
            set_memory(MEM_FRE, fm_mode_var.wFreq - MIN_FRE);
			set_memory(MEM_CHAN, fm_mode_var.bFreChannel);
            UI_menu(MENU_FM_DISP_FRE);
            break;

        default:
            ap_handle_hotkey(key);
            break;
        }

    }
}

/*----------------------------------------------------------------------------*/
/**@brief    FM 模式
   @param    无
   @return   无
   @note     void fm_mode(void)
*/
/*----------------------------------------------------------------------------*/
extern bool scan_flag;
  
extern	_no_init  bool	_bit  first;
extern void QND_SetVol(u8 vol);
extern  bool  muteflag;

void fm_mode(void) AT(FM_CODE)
{
#ifndef SHARE_RTC_OSC_TO_FM
    P05_config(P05_OSC_CLK);
#endif
	//delay_n10ms(10);
      sdcheck_flag = 1;

   
	muteflag = 0;
    first=0;
    scan_flag = 0;
#ifdef UART_PRINT_ENABLE
   if(1)
#else
   if(init_fm_rev())
#endif
    {
       // my_printf("%04x\n",0xfee1);
        
        fm_info_init();
		
		//my_printf("%04u\n",main_vol_R);
		
		main_vol_R = main_vol_L;
		//QND_SetVol(main_vol_R);
		set_main_vol(main_vol_L, main_vol_R);
        //dac_channel_sel(DAC_DECODER);
        dac_channel_sel(DAC_AMUX0_R);
        system_clk_div(CLK_12M);
		
		//fm_module_vol(main_vol_L);	
		fm_module_mute(0);
          sdcheck_flag=0;
		  delay_n10ms(4);
		  dac_mute(0);
        fm_play();
		dac_mute(1);
		delay_n10ms(10);
        fm_rev_powerdown();
    }
    else					// no fm module
    {
        work_mode = MUSIC_MODE;
    }
      sdcheck_flag=0;
#ifndef SHARE_RTC_OSC_TO_FM
    P05_config(P05_NORMAL_IO);
#endif
}
