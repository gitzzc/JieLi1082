/*--------------------------------------------------------------------------*/
/**@file     main.c
   @brief    主流程
   @details
   @author
   @date    2011-3-7
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "main.h"
#include "clock.h"
#include "music_play.h"
#include "fm_radio.h"
#include "USB_Device_Mode.h"
#include "key.h"
#include "RTC_Mode.h"
#include "RTC_API.h"
#include "msg.h"
#include "iic.h"
#include "device.h"
#include "DAC.h"
#include "get_music_file.h"
#include "usb_host.h"
#include "sdmmc_api.h"
#include "LCD.h"
#include "LED5X7.h"
#include "UI_API.h"
#include "Line_in_mode.h"
#include "LCD_SEG4X8.h"
#include "SPI.h"
#include "LCD_SEG5X9.h"
#include "play_file.h"
#include "FM_API.h"
#include "mic.h"

extern u8 _data  powerflag;

_no_init void (_near_func * _pdata int_enter_pro[16])(void) AT(INTERRUPT_TABLE);
extern  _no_init u16  countnum;

_no_init ENUM_WORK_MODE _data work_mode;
_no_init ENUM_WORK_MODE _data back_work_mode;

extern _no_init u16 _data input_10;
extern  __root bool QN8035_Read_ID(void);
bool aux_ad_check_flag = 0;

#ifdef POWER_CHECK_ENABLE	
bool power_ad_check_flag = 0;
#endif
#ifdef AD_MIC_DET
extern _no_init bool kalaok_DET;
bool kalaok_button;
bool MP3_pause = 0;

#endif

extern	_no_init u16 _data input_100;
//_root _no_init u8 _data B_CURRENTBANK  0x1F;
_no_init bool _bit Sys_Volume;
_no_init bool _bit Sys_IRInput;
_no_init bool _bit Sys_HalfSec;
_no_init bool _bit alm_on_flag;
_no_init  bool  _bit  fmone;
bool  sdcheck_flag = 0;
 _no_init  bool  _bit  aux_online;
 #ifdef POWER_CHECK_ENABLE	
 _no_init  bool  _bit  power_off_online;
 #endif
_no_init bool first_in_mp3;
_no_init bool _bit set_sd_clk; ///<sd clk检测用，要操作卡时要设置为1,不操作卡时要设置为0，用途是不读卡时把clk脚保持一个稳定的电平
extern bool eeprom_flag;
bool  first_power_on_read_24c02 = 0;

extern void eeprom_verify(void);
/*----------------------------------------------------------------------------*/
/**@brief   显示设备扫描函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void monitor_scan(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void monitor_scan(void) AT(COMMON_CODE)
{
#ifdef LED_5X7
    LED5X7_scan();
#endif
}


_near_func  void led_scan(void)  AT(COMMON_CODE)
{
       static   u8 _xdata count = 0;

	   count++;

	
	   switch(work_mode)
	   	{
           case  MUSIC_MODE:

             if(Music_Play_var.bPlayStatus== MAD_PAUSE)
             	{
                     LED_ON;
					 count=0;

			    }
			    else  if(Music_Play_var.bPlayStatus== MAD_PLAY)
			    	{

					 // my_printf("%04x\n",playfile.play_mode );
				    	if(playfile.play_mode == REPEAT_ALL)
				    		{
							   if(count>60)
							   	{
				                   count=0;
								  LED_FLISH;
							    }
				    		}
						     else  if(playfile.play_mode == REPEAT_ONE)
						     	{

								
                                    if(count>22)
								   	{
					                   count=0;
									   LED_FLISH;
								    }

							    }
							 	

			    	}
				
                break;
		    case  FM_RADIO_MODE:

					if(scan_mode == FM_SCAN_STOP)
					 {	LED_ON; }
					else  if((scan_mode != FM_SCAN_STOP))
						{
                                  if(count>30)
								   	{
					                   count=0;
									   LED_FLISH;
								    }

					    }
				
                    break;
			case AUX_MODE:
	            if(aux_online)
				{
				LED_ON;
					//LED_OFF;
				   }
					else
				 {
					LED_ON;
				}	
					break;
 #ifdef USB_DEVICE_EN
			case USB_DEVICE_MODE:
				LED_ON;
				  break;
#endif
		   case NO_DEVICE_OFF:
			   if(powerflag)
			   {
				 LED_OFF;
			   }
				else
			   {
				LED_OFF;
			   }
			   break;
        default:
            LED_ON;
            break;			
		
				

	    }


}




_near_func void aux_check(void) AT(COMMON_CODE)
{


	//P1DIR |= BIT(7);
	//P1PU |= BIT(7);
	//delay(50);		   	
#ifdef AUX_CHECK_ENABLE	
   static u8 aux_cnt_online;
   // if((!P17))
   if(aux_ad_check_flag)
    {
   	 aux_cnt_online++;
        if (aux_cnt_online > 50)
        {
            if (!aux_online)
            {
                aux_online = 1;
                put_msg_lifo(MSG_AUX_IN);
            }
        }	
    }
    else
    {
        aux_cnt_online = 0;
        if (aux_online)
        {
            aux_online = 0;
            put_msg_lifo(MSG_AUX_OUT);
        }
    }
#else
   aux_online = 1;
#endif
}

_near_func void power_off_check(void) AT(COMMON_CODE)
{


	//P1DIR |= BIT(7);
	//P1PU |= BIT(7);
	//delay(50);		   	
#ifdef POWER_CHECK_ENABLE	
   static u8 power_cnt_online;
   // if((!P17))
   if(power_ad_check_flag)
    {
   	 power_cnt_online++;
        if (power_cnt_online > 50)
        {
            if (!power_off_online)
            {
                power_off_online = 1;
                put_msg_lifo(MSG_POWER_IN);
            }
			power_cnt_online = 200;
        }	
    }
    else
    {
        power_cnt_online = 0;
        if (power_off_online)
        {
            power_off_online = 0;
            put_msg_lifo(MSG_POWER_OUT);
        }
    }
#else
   //power_off_online = 0;
#endif
}

void kalaok_check(void)AT(LINE_IN_CODE)// AT(COMMON_CODE)
{
	static	u8 kalaok_cnt_online=0;	
	static	u8 kalaok_cnt_offline=0;	
	u8 i;
	
	//u8 temp;
	
  if (work_mode == MUSIC_MODE)	
   {
	if ((MP3_pause)&&(Music_Play_var.bPlayStatus == MAD_PLAY))
	 {
	  put_msg_lifo(MSG_MUSIC_PP);
	  MP3_pause=0;
	   // dac_mute(0);
	 }
  }
 // if(!P25)
   kalaok_DET=1;
  if(kalaok_DET)
	{	
	    kalaok_cnt_offline = 0;
		
		if(kalaok_cnt_online < 40)
		kalaok_cnt_online++;		
		if (kalaok_cnt_online == 40)
		{
			kalaok_cnt_online = 50;
			if (!kalaok_button)
			{
#ifdef UART_PRINT_ENABLE			
	 my_printf("kalaok in \n");
#endif			
			  kalaok_button = 1;	
			  if (work_mode != MUSIC_MODE)
				  {
					work_mode = SPE_MODE;
					put_msg_lifo(MSG_CHANGE_WORK_MODE);
					return;
				  }

				MUTE_OFF;
				 if (work_mode == MUSIC_MODE)	
				 {
				   device_check();
				   if(!(device_online & 0x03))
				   	{
				   	   work_mode = SPE_MODE;
					    put_msg_lifo(MSG_CHANGE_WORK_MODE);
				        return;
				   	}
				  else if (Music_Play_var.bPlayStatus== MAD_PAUSE)
					 {
					  // dac_mute(1);
					   put_msg_lifo(MSG_MUSIC_PP);
					   MP3_pause=1;
					 }	
				 }
				
			  for(i = 0;i<200;i++)			 _nop_();
			  dac_mic_control(1);
			
			}
		}
	}
	else
	{
		kalaok_cnt_online = 0;
		if(kalaok_cnt_offline <40)
		kalaok_cnt_offline++;
		if(kalaok_cnt_offline == 40)
		{
		   kalaok_cnt_offline = 50;
				if (kalaok_button)
				{
#ifdef UART_PRINT_ENABLE			
	 my_printf("kalaok out d\n");
#endif				
				  kalaok_button = 0;								
					dac_mic_control(0);				
					if (work_mode == SPE_MODE)
						{
						 work_mode = MUSIC_MODE;
						 put_msg_lifo(MSG_CHANGE_WORK_MODE);
						}
				}
		}
	}
}

/*----------------------------------------------------------------------------*/
/** @brief: Timer1 中断服务程序
    @param: void
    @return:void
    @author:Change.tsai
    @note:  void Timer1_ISR(void)
*/
/*----------------------------------------------------------------------------*/
_near_func __root void Timer1_ISR(void) AT(COMMON_CODE)
{
    static _no_init u8 _data cnt;

    TMR1_CON0 |= BIT(6);            //clear pending

    dac_check();

    /*UI Update*/
   // monitor_scan();

    /*Device Scan*/
    USB_online_otp();

    /*SD DET*/
#ifdef SDMMCA_DET_CLK
    if(sdcheck_flag == 0)
    sdmmc_clk_det0();
#endif
#ifdef SDMMCA_DET_CMD
    sdmmc_command_detect();
#endif

    ADC_scan();

#ifdef IR_REMOTE_EN
    /*IR overflow*///if (IR_code.boverflow)
    {
        IR_code.boverflow++;
        if (IR_code.boverflow > 56) //56*2ms = 112ms
        {
            //IR_code.boverflow = 0;
            /*IR Signal Clear*/
            IR_code.bState = 0;
        }
    }
#endif

    cnt++;
    if ((cnt % 5) == 0)             //10ms
    {
       // aux_check();
#ifdef POWER_CHECK_ENABLE		
		power_off_check();
#endif
#ifdef AD_MIC_DET
      kalaok_check();
#endif
        key_scan();
        dec_delay_counter();
		led_scan();
    }
	
	if ((cnt % 50) == 0)
	{
		put_msg_fifo(MSG_100MS);
	}
	
    if ((cnt % 250) == 0)
    {
        ///my_printf("%04u\n",work_mode);
        cnt = 0;
        Sys_HalfSec = !Sys_HalfSec;
        put_msg_fifo(MSG_HALF_SECOND);
#ifdef UART_PRINT_ENABLE			
	// my_printf("kalaok_DET =%d\n",kalaok_DET);
#endif			
    }
#ifdef RTC_ALARM_EN
    if (RTC_CON0 & BIT(7))
    {
        IRTC_WKCLRPND;
        put_event(EVENT_ALM_ON);
    }
#endif

#ifdef SDMMCA_DET_CLK
    if(sdcheck_flag == 0)
    sdmmc_clk_det1();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief   Timer2 中断服务程序
   @param   void
   @return  void
   @author  Change.tsai
   @note    void Timer1_ISR(void)
*/
/*----------------------------------------------------------------------------*/
const u16 timer2_1ms_table[] AT(TABLE_CODE)=
{
    0x20,0x40,0x80,0x2f0,0x178,0x2f0
};
_near_func __root void Timer2_ISR(void) AT(COMMON_CODE)
{
    u16 bCap2;
    u8 cap;

    TMR2_CON |= BIT(6);

    bCap2 = (TMR2_PRDH << 8) | TMR2_PRDL;

    cap = bCap2 / timer2_1ms_table[system_clk];

    if (cap == 1)
    {
        IR_code.wData >>= 1;
        IR_code.bState++;
        IR_code.boverflow = 0;
    }
    else if (cap == 2)
    {
        IR_code.wData >>= 1;
        IR_code.bState++;
        IR_code.wData |= 0x8000;
        IR_code.boverflow = 0;
    }
    /*13ms-Sync*/
    else if ((cap == 13) || (cap < 8) || (cap > 100))
    {
        IR_code.bState = 0;
    }
    else
        IR_code.boverflow = 0;

    if (IR_code.bState == 16)
    {
        IR_code.wUserCode = IR_code.wData;
    }
}
/*----------------------------------------------------------------------------*/
/** @brief: 串口模块初始化
    @param: void
    @return:void
    @author:Juntham
    @note:  void uart_setup(void)
*/
/*----------------------------------------------------------------------------*/
void uart_setup(void) AT(CSTART)
{
    u8 ad ;
    uart_init();
    set_putchar(my_putchar);
	ad = 15;
	my_printf("ad = %d \n",ad);


}



/*----------------------------------------------------------------------------*/
/**@brief   UI 初始化
   @param   void
   @return  void
   @author  Change.tsai
   @note    void ui_init(void)
*/
/*----------------------------------------------------------------------------*/
static void ui_init(void) AT(CSTART)
{

#if defined LED_5X7
   // LED5X7_init();
#endif

#if defined LCD_SEG_4X8 || LCD_SEG_3X9
    LCD_SEG4X8_init();
#endif

#if defined LCD_SEG_5X9
    LCD_SEG5X9_init();
#endif

#if defined LCD_96X32_SERIAL || defined LCD_128X32_SERIAL   \
    || defined LCD_128X64_SERIAL || defined LCD_128X64_PARALLEL
    LCD_init();
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief   系统初始化
   @param   void
   @return  void
   @author  Change.tsai
   @note    void sys_init(void)
*/
/*----------------------------------------------------------------------------*/
static void sys_init(void) AT(CSTART)
{
    /*IIC I/O init*/
    iic_init();

    /*UI init*/
    ui_init();

    /*Time Base init*/
    int_enter_pro[TIMER1_INT] = Timer1_ISR;

#ifdef IR_REMOTE_EN
    /*IR Capture init*/
    int_enter_pro[TIMER2_INT] = Timer2_ISR;
#endif
    /*Timebase interrupt init*/
    IRTC_init();

    /*SD I/O & Controller init*/
    sdmmc_init();

    /*ADKey init*/
    AD_init();

    /*Hello UI*/
    //UI_menu(MENU_POWER_UP);

    /*ALL interrupt*/
    EA = 1;

    /*Wait Device steady Plug In...*/
    delay_n10ms(95);
    /*PLL初始化，会关总中断*/
	pll_init();
	
    /*EEPROM verify*/
	first_power_on_read_24c02 = 1;
    eeprom_verify();
	first_power_on_read_24c02 = 0;
#ifdef UART_PRINT_ENABLE			
	 my_printf("eeprom_flag =%d\n",eeprom_flag);
#endif	

 //  my_printf("%04u\n",eeprom_flag);
//#ifdef SDMMC_UDISK_MEMORY
  //   eeprom_flag = 0;

    if(eeprom_flag == 0)
    {
#ifndef USE_IRTC_MEMORY
       power_up_device_info(); //从设备读出信息
#endif
    }
//#endif

	/*DAC 初始化*/
    DAC_init();

    /*系统变量初始化*/
    ///work_mode =(ENUM_WORK_MODE)get_memory(MEM_SYSMODE);
  //  work_mode = MUSIC_MODE;

  //  playfile.given_device = NO_DEVICE;

}

/*----------------------------------------------------------------------------*/
/**@brief   空 模式
   @param   void
   @return  void
   @author  Change.tsai
   @note    void Line_in_mode(void)
*/
/*----------------------------------------------------------------------------*/
void nodevice_in_mode(void) AT(COMMON_CODE)
{
      u8 msg;

    Sys_IRInput = 0;
    Sys_Volume = 0;
    dac_mute(1);
    /*---------AUX MAIN UI--------------*/
    SET_UI_MAIN(MENU_AUX_MAIN);
   // UI_menu(MENU_AUX_MAIN);
    key_table_sel(0);

   system_clk_div(CLK_24M);
    dac_channel_sel(DAC_DECODER);

    while (1)
    {
        msg = app_get_msg();

        switch (msg)
        {
      /*  case MSG_AUX_IN:
            break;
        case MSG_AUX_OUT :
            work_mode++;
            return;
        case MSG_MUSIC_NEW_DEVICE_IN:
            work_mode = MUSIC_MODE;*/
     //   case MSG_CHANGE_WORK_MODE:
          //  deg_puts("Exit RTC mode\n");
     //       return;
     case MSG_POWER_ON:
           device_check();
		   work_mode = back_work_mode;
		  if(work_mode == MUSIC_MODE)
		  {
		     if(device_online & 0x03)
		     {
		  	  work_mode = MUSIC_MODE;
		     }
#ifdef AD_MIC_DET
			 else if(kalaok_button)
			 {
			  work_mode = SPE_MODE;
			 }
#endif
			 else
			 {
			  work_mode = FM_RADIO_MODE;
			 }
		  }
#ifdef AD_MIC_DET		
         else if(work_mode == SPE_MODE)
         {
		    if(kalaok_button)
		    {
		      work_mode = SPE_MODE;
		    }
			else if(device_online & 0x03)
			{
			  work_mode = MUSIC_MODE;
			}
			else
			{
			 work_mode = FM_RADIO_MODE;
			}
         }
#endif
         else
         {
            work_mode = FM_RADIO_MODE;
         }
		 return;


        case MSG_HALF_SECOND:
          //  LED_FADE_OFF();
          //  UI_menu(MENU_MAIN);
            break;

        default:
           // ap_handle_hotkey(msg);
            break;
        }
    }


	



}







/*----------------------------------------------------------------------------*/
/** @brief: 系统主循环
    @param: void
    @return:void
    @author:Juntham
    @note:  void main(void)
*/
/*----------------------------------------------------------------------------*/
void main(void) AT(CSTART)/////a68  //  xb9+11//  gs064 1085 //1402///
{
#ifndef NDEBUG
    set_debug_mode(1);          //进入2线调试模式
#else
    set_debug_mode(0);
#endif

#ifdef WDT_EN
    config_wdt(0x5D);           //enable WDT 8S,如果改成4S,有些慢U盘会导致看门狗复位
#endif
    LED_ON;
    input_10=0;
    input_100=0;
    countnum=0;
	aux_online=0;
	work_mode = MUSIC_MODE;

#ifdef   USE28	
    P3PU &= ~BIT(0);
	P3PD &= ~BIT(0);
	P3DIR |= BIT(0);
	P3DIE |= BIT(0);
	   P2PU &= ~BIT(3);
	   P2PD &= ~BIT(3);
	   P2DIR |= BIT(3);
	   P2DIE |= BIT(3);


//	P2PU &= ~BIT(5);
//	P2PD &= ~BIT(5);
//	P2DIR |= BIT(5);
//	P2DIE &= ~BIT(5);

#endif
#ifdef UART_PRINT_ENABLE	
    uart_setup();
#endif
     MUTE_ON;
    sys_init();

    clear_all_event();
    dac_mute(1);

    first_in_mp3 = 1;
	//delay_n10ms(100);

	delay_n10ms(1);
    device_check();		
#ifdef POWER_CHECK_ENABLE		
    if(power_off_online)
    {
      work_mode = NO_DEVICE_OFF;
    }
    else
#endif	
#ifdef USB_DEVICE_EN
	if ((get_usb_online_status() & BIT(1)) != 0x0)
	{
	  work_mode = USB_DEVICE_MODE;
	}
	else
#endif
     if(device_online&0x03)
    {
    // playfile.given_device = (ENUM_DEVICE_SELECT_MODE)get_memory(MEM_ACTIVE_DEV);
	// if(playfile.given_device == NO_DEVICE)
	 playfile.given_device = DEVICE_UDISK;	
	 work_mode = MUSIC_MODE;
    }
#ifdef AD_MIC_DET
	else if(kalaok_button)
	{
	   work_mode = SPE_MODE;
	}
#endif
	else
	{
	   work_mode = FM_RADIO_MODE;
	}	


    while (1)
    {
        flush_all_msg();
		// my_printf("%04x\n",work_mode);
        switch (work_mode)
        {

        case MUSIC_MODE:
#ifdef UART_PRINT_ENABLE			
	    my_puts("MUSIC_MODE \n");
#endif		
			P2PU &= ~BIT(4);
			P2PD &= ~BIT(4);
			P2DIR |= BIT(4);
			P2DIE &= ~BIT(4);

            music_app();
            break;
			
		case SPE_MODE:		
#ifdef UART_PRINT_ENABLE
	my_printf("spe mode \n");
#endif			
			P2PU &= ~BIT(4);
			P2PD &= ~BIT(4);
			P2DIR |= BIT(4);
			P2DIE &= ~BIT(4);					
			first_in_mp3 = 0;
			Speaker_mode();
			break;

#ifdef USB_DEVICE_EN
        case USB_DEVICE_MODE:
			first_in_mp3 = 0;
            USB_device_mode();
            break;
#endif

#ifdef FM_ENABLE
        case FM_RADIO_MODE:
#ifdef UART_PRINT_ENABLE			
	    my_puts("FM_RADIO_MODE \n");
#endif		
			/* if(kalaok_button)
			 {
				 dac_mic_control(0);
				 dac_mic_gain_control(0);
			 }

			 P2PU &= ~BIT(3);
			 P2PD &= ~BIT(3);
			 P2DIR |= BIT(3);
			 P2DIE &= ~BIT(3);

			 MUTE_ON;
			first_in_mp3 = 0;
            fm_mode();
			//MUTE_OFF;*/
         work_mode = AUX_MODE;
            break;
#endif

        case AUX_MODE:
#ifdef UART_PRINT_ENABLE			
	    my_puts("AUX_MODE \n");
#endif				
			first_in_mp3 = 0;
            Line_in_mode();
            break;
		case NO_DEVICE_OFF:
#ifdef UART_PRINT_ENABLE			
	    my_puts("NO_DEVICE_OFF \n");
#endif				
			first_in_mp3 = 0;
			nodevice_in_mode();
			
              break;
#ifdef RTC_EN
        case RTC_MODE:
			first_in_mp3 = 0;
           // my_puts("-RTC Mode\n");
            RTC_mode();
            break;
#endif
        default:
            work_mode = MUSIC_MODE;
            break;

		
        }
        ///set_memory(MEM_SYSMODE, work_mode);
    }
}
