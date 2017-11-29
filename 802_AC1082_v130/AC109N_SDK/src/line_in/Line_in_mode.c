/*--------------------------------------------------------------------------*/
/**@file    Line_in_mode.c
   @brief   Line-In 模式主循环
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "config.h"
#include "Line_in_mode.h"
#include "msg.h"
#include "hot_msg.h"
#include "key.h"
#include "main.h"
#include "device.h"
#include "iic.h"
#include "UI_API.h"
#include "LCD.h"
#include "dac.h"
#include "clock.h"
#include "mic.h"

extern _no_init  bool	_bit  aux_online;

/*----------------------------------------------------------------------------*/
/**@brief   Line-In 模式信息初始化
   @param   void
   @return  void
   @author  Change.tsai
   @note    void Line_in_info_init(void)
*/
/*----------------------------------------------------------------------------*/
void Line_in_info_init(void) AT(LINE_IN_CODE)
{
    /*------------System configuration-----------*/
    Sys_IRInput = 0;
    Sys_Volume = 0;

    /*---------AUX MAIN UI--------------*/
    SET_UI_MAIN(MENU_AUX_MAIN);
    UI_menu(MENU_AUX_MAIN);
    key_table_sel(0);
}


/*----------------------------------------------------------------------------*/
/**@brief   Line-In 模式主循环
   @param   void
   @return  void
   @author  Change.tsai
   @note    void Line_in_loop(void)
*/
/*----------------------------------------------------------------------------*/
void Line_in_mode_loop(void) AT(LINE_IN_CODE)
{
    u8 msg;
   MUTE_OFF;
    while (1)
    {
        msg = app_get_msg();

        switch (msg)
        {
      //  case MSG_AUX_IN:
        //   break;
      //  case MSG_AUX_OUT :
          //  work_mode++;
        //    return;
		
	   case  MSG_MUSIC_PP:

			//put_msg_lifo(MSG_MUTE);
			  break;
        case MSG_MUSIC_NEW_DEVICE_IN:
			//if(!aux_online)
			//{
            work_mode = MUSIC_MODE;
			return ;
			//}
			//break;
        case MSG_CHANGE_WORK_MODE:
        //    deg_puts("Exit RTC mode\n");
            return;

        case MSG_HALF_SECOND:
            LED_FADE_OFF();
            UI_menu(MENU_MAIN);
            break;

        default:
            ap_handle_hotkey(msg);
            break;
        }
    }

}

/*----------------------------------------------------------------------------*/
/**@brief   Line-In 模式
   @param   void
   @return  void
   @author  Change.tsai
   @note    void Line_in_mode(void)
*/
/*----------------------------------------------------------------------------*/

extern	_no_init  bool	_bit  first;
extern bool muteflag;

void Line_in_mode(void) AT(LINE_IN_CODE)
{

    first=0;
	muteflag = 0;
    Line_in_info_init();
	//if(aux_online)
    //dac_channel_sel(DAC_AMUX0_R);
	//else
	//dac_channel_sel(DAC_DECODER);
#ifndef UART_PRINT_ENABLE
    system_clk_div(CLK_12M);
#endif
	 delay_n10ms(74);
	dac_mute(1);
    Line_in_mode_loop();
	dac_mute(1);
	delay_n10ms(10);
	 dac_channel_sel(DAC_DECODER);
	
}


void Spe_mode_loop(void) AT(LINE_IN_CODE)
{
    u8 msg;

    while (1)
    {
        msg = app_get_msg();

        switch (msg)
        {
        case MSG_MUSIC_NEW_DEVICE_IN:
            work_mode = MUSIC_MODE;
        case MSG_CHANGE_WORK_MODE:
            return;

        case MSG_HALF_SECOND:
			dac_mic_control(1);

        default:
            ap_handle_hotkey(msg);
            break;
        }
    }

}

void Speaker_mode(void) AT(LINE_IN_CODE)
{

	MUTE_ON;		
	//mute_flag=0;
    Line_in_info_init();		
    dac_channel_sel(DAC_DECODER);
    system_clk_div(CLK_24M);
	delay_n10ms(40);
	//dac_mute(0);
	MUTE_OFF;
	dac_channel_sel_mic(DAC_AMUX1_R);	
	dac_mute(0);
	 Sys_Volume = 0;
  // set_analog_vol(MAX_MAIN_VOL_L, MAX_MAIN_VOL_L);
    set_analog_vol(27, 27);
    Spe_mode_loop();	
	MUTE_ON;
	dac_mic_control(0);
    system_clk_div(CLK_24M);
}

