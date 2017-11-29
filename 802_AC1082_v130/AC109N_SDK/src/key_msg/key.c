/*--------------------------------------------------------------------------*/
/**@file    key.c
   @brief   按键驱动
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "key.h"
#include "irff00.h"
#include "ad_key.h"
#include "DAC.h"
#include "UI_API.h"
#include "play_file.h"
#include "main.h"
#include "clock.h"
#include "RTC_API.h"
#include "RTC_Mode.h"
#include "IRTC.h"
#include "FM_API.h"


_no_init IR_CODE _idata IR_code;    //<红外遥控信息
_no_init AD_VAR _data AD_var;      //AD 按键
_no_init static bool _bit key_type;
_no_init void (_near_func * _data key_msg_filter)(u8, u8);
_no_init  bool  key_judge;
extern bool aux_ad_check_flag;
#ifdef POWER_CHECK_ENABLE	
extern bool power_ad_check_flag;
#endif
extern _no_init u8 _data scan_mode;

#ifdef AD_MIC_DET
_no_init bool kalaok_DET;
#endif
const u8 ad_table[] AT(TABLE_CODE)=
{
    0xf2,               ///<高4位为延时，低4位为通道选择
    0xfc,
    0xfd,
    0xfe,
    0xff,
};

/*----------------------------------------------------------------------------*/
/** @brief:事件消息转换表
    @param:
    @return:
    @author:Juntham
    @note:
*/
/*----------------------------------------------------------------------------*/
const u8 _code event_msg_table[] AT(TABLE_CODE)=
{
    MSG_USB_DISK_IN,
    MSG_USB_DISK_OUT,
    MSG_USB_PC_IN,
    MSG_USB_PC_OUT,
    MSG_SDMMCA_IN,
    MSG_SDMMCA_OUT,
    MSG_SDMMCB_IN,
    MSG_SDMMCB_OUT,
    MSG_PC_MUTE,
    MSG_PC_UNMUTE,
    MSG_AUX_IN,
    MSG_AUX_OUT,
    MSG_ALM_ON,
    MSG_PC_AUDIO_VOL,
    MSG_FM_SCAN_ALL_INIT
};
/*----------------------------------------------------------------------------*/
/**@brief   Music 模式按键消息过滤处理
   @param   msg：按键消息
   @return  无
   @author  Change.tsai
   @note    void music_msg_filter(u8 sel)
*/
/*----------------------------------------------------------------------------*/
_near_func void music_msg_filter(u8 key_status, u8 back_last_key) AT(KEY_CODE)
{
    u8 msg;

    if (key_type == KEY_TYPE_IR)
    {
        msg = irff00_msg_music_table[key_status][back_last_key];
    }
    else
    {
        msg = adkey_msg_music_table[key_status][back_last_key];
    }

    if (work_mode == MUSIC_MODE)            //与其它模式共用，但只有music模式需要此处理
    if ((msg == MSG_MUSIC_NEXT_FILE) || (msg == MSG_MUSIC_PREV_FILE))
    {
        put_msg_lifo(msg);
        user_exit_decoder();
        return;
    }

    put_msg_fifo(msg);
}

/*----------------------------------------------------------------------------*/
/**@brief   FM 模式按键消息过滤处理
   @param   msg：按键消息
   @return  无
   @author  Change.tsai
   @note    void fm_msg_filter(u8 sel)
*/
/*----------------------------------------------------------------------------*/
_near_func void fm_msg_filter(u8 key_status, u8 back_last_key) AT(KEY_CODE)
{
    u8 msg;

    if (key_type == KEY_TYPE_IR)
    {
        msg = irff00_msg_fm_table[key_status][back_last_key];
    }
    else
    {
        msg = adkey_msg_fm_table[key_status][back_last_key];
    }

    if(scan_mode != FM_SCAN_STOP)
    {
     if((msg == MSG_MUSIC_PP)||(msg == MSG_FM_NEXT_STATION)||(msg == MSG_FM_PREV_STATION))
     {
         msg = MSG_FM_SCAN_ALL_INIT;
     }
    }

    if (msg == MSG_FM_SCAN_ALL_INIT)
    {
        put_event(EVENT_SCAN_ALL);
        return;
    }

    put_msg_fifo(msg);
}

/*----------------------------------------------------------------------------*/
/**@brief   按键功能表选择
   @param   sel:
   @return  无
   @author  Change.tsai
   @note    void key_table_sel(u8 sel)
*/
/*----------------------------------------------------------------------------*/
_near_func __monitor void key_table_sel(u8 sel) AT(KEY_CODE)
{
    if (sel == 0)
    {
        key_msg_filter = music_msg_filter;
    }
    else
    {
        key_msg_filter = fm_msg_filter;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   获取消息池的消息,事件最终转化为消息
   @param   无
   @return  消息
   @author  Change.tsai
   @note    u8 app_get_msg(void)
*/
/*----------------------------------------------------------------------------*/
_near_func u8 app_get_msg(void) AT(KEY_CODE)
{
    u8 event, msg;

    WDT_CLEAR();

    event = get_event();

    if (event != NO_EVENT)
    {
        clear_one_event(event);
        msg = event_msg_table[event];
    }
    else
        msg = get_msg();

    //if (msg == NO_MSG)
    //{
    //}

    return msg;
}

/*----------------------------------------------------------------------------*/
/**@brief   IR 遥控按键序号获取函数
   @param   无
   @return  红外遥控按键号
   @author  Change.tsai
   @note    u8 IR_key(void)
*/
/*----------------------------------------------------------------------------*/
_near_func static u8 IR_key(void) AT(COMMON_CODE)
{
    u8 key = NO_KEY;

    if (IR_code.bState != 32)
        return key;

    if ((((u8 _idata*)&IR_code.wData)[0] ^ ((u8 _idata *)&IR_code.wData)[1]) == 0xff)
    {
        //my_printf("%04x\n",IR_code.wUserCode );
	
        if (IR_code.wUserCode == 0xFF00)
        {
           // my_printf("%04x\n",IR_code.wData );
			
            key = IRTabFF00[(u8)IR_code.wData];
        }
#if 0		
		else   if(IR_code.wUserCode == 0xbf00)
			{

				// my_printf("IR_code.wData = %04x\n",IR_code.wData);
				
                key = IRTabBF00[(u8)IR_code.wData];

		    }
#endif		
    }
    else
    {
        IR_code.bState = 0;
    }

    return key;
}

/*----------------------------------------------------------------------------*/
/**@brief   A/D 按键序号获取函数
   @param   key_value：AD键值
   @return  按键序号
   @author  Change.tsai
   @note    u8 AD_key(u16 key_value)
*/
/*----------------------------------------------------------------------------*/
_near_func static u8 AD_key(u16 key_value)  AT(COMMON_CODE)
{
    u8 key_number;

	// my_printf("%04x\n",key_value);

  //  if (key_value > AD_NOKEY)
       // return NO_KEY;
#if 0
      if(key_value > 0xf800)
     	{
          // key_judge=0;
		  //  my_puts("key_out\n");
          return NO_KEY;
	    }
	    else  if(key_value > 0xf000)
	    	{

                  // key_judge=1;
				  //  my_puts("key_in\n");
                  return NO_KEY;

		    }


#ifdef AUX_CHECK_ENABLE	
	 if (key_value > 0xf6c0)
	 {
		 aux_ad_check_flag = 0;
		 return NO_KEY;
	 }
	 else if(key_value > 0xec00)
	 {
		aux_ad_check_flag = 1;
		return NO_KEY;
	 }
#elif defined  POWER_CHECK_ENABLE	
	 if (key_value > AD_NOKEY)
	 	{
	 	power_ad_check_flag = 0;
		return NO_KEY;
		}
#else
	if (key_value > AD_NOKEY)
	   {
	   return NO_KEY;
	   }
#endif
#endif

#ifdef AD_MIC_DET
			if (key_value > 0xf6c0)
			{
				  kalaok_DET = 0;
				return NO_KEY;
			}
			else if(key_value > 0xec00)
			{
				 kalaok_DET = 1;
			   return NO_KEY;
			}
#endif
#ifdef UART_PRINT_ENABLE			
	//	my_printf("key_value =%x\n",key_value);
#endif	

    for (key_number = 0; key_number < sizeof (ad_key_table) / sizeof (ad_key_table[0]); key_number++)
    {
        if (key_value > ad_key_table[key_number])
            break;
    }
	
	//my_printf("%04x\n",key_number);
#ifdef POWER_CHECK_ENABLE		
	if(key_number == 0)
	{
	
	  key_number = NO_KEY;
		power_ad_check_flag = 1;
	}
#endif
    return key_number;
}

/*----------------------------------------------------------------------------*/
/**@brief   按键去抖函数，输出稳定键值
   @param   key：键值
   @return  稳定按键
   @author  Change.tsai
   @note    u8 keyFilter(u8 key)
*/
/*----------------------------------------------------------------------------*/
_near_func static u8 key_filter(u8 key) AT(COMMON_CODE)
{
    static u8 _data used_key = NO_KEY;
    _no_init static u8 _data old_key;
    _no_init static u8 _data key_counter;

    if (old_key != key)
    {
        key_counter = 0;
        old_key = key;
    }
    else
    {
        key_counter++;
        if (key_counter == KEY_BASE_CNT)
        {
            used_key = key;
        }
    }

    return used_key;
}

/*----------------------------------------------------------------------------*/
/**@brief   按键-消息转换函数,按键产生顺序：短按抬起/长按-->连按
   @param   key：按键序号
   @param   key_type：按键类型
   @return  void
   @author  Change.tsai
   @note    void key2msg(u8 key, u8 key_type)
*/
/*----------------------------------------------------------------------------*/
_near_func void key_scan(void) AT(COMMON_CODE)
{
    static u8 _data last_key = NO_KEY;
    _no_init static u8 _data key_press_counter;
    u8 cur_key, key_status, back_last_key;

    //my_printf("--key value : %02x\n", AD_var.wADValue[AD_CH_KEY]);
    cur_key = AD_key(AD_var.wADValue[AD_CH_KEY]);       //AD按键优先

    if (cur_key == NO_KEY)
    {
        cur_key = IR_key();                             //检查是否有遥控
        if (cur_key != NO_KEY)
        {
            key_type = KEY_TYPE_IR;
        }
    }
    else
    {
        key_type = KEY_TYPE_AD;
    }

    cur_key = key_filter(cur_key);                    //过滤按键，相当于去抖

    back_last_key = last_key;

    if (cur_key == last_key)                            //长时间按键
    {
        if (cur_key == NO_KEY)
            return;

        key_press_counter++;

        if (key_press_counter == KEY_LONG_CNT)          //长按
        {
            //if(key_judge)
			//   return;
			  kick_key_voice();
            key_status = KEY_LONG;
        }
        else if (key_press_counter == (KEY_LONG_CNT + KEY_HOLD_CNT))        //连按
        {
            // if(key_judge)
			//   return;
			 	
		            key_status = KEY_HOLD;
		            key_press_counter = KEY_LONG_CNT;
			 	
        }
        else
        {
            return;
        }
    }
    else  //cur_key = NO_KEY, 抬键
    {
        last_key = cur_key;

        if ((key_press_counter < KEY_LONG_CNT) && (cur_key == NO_KEY))      //短按抬起
        {
          kick_key_voice();
            key_press_counter = 0;
            key_status = KEY_SHORT_UP;
        }
        else
        {
            key_press_counter = 0;
            return;
        }
    }
#ifdef RTC_EN
    RTC_setting_var.bStandbyCnt = 0;
#endif
    LED_FADE_ON();
    //kick_key_voice();
    key_msg_filter(key_status, back_last_key);
}

/*----------------------------------------------------------------------------*/
/**@brief   A/D 模块初始化
   @param   void
   @return  void
   @author  Change.tsai
   @note    void AD_init(void)
*/
/*----------------------------------------------------------------------------*/
void AD_init(void) AT(CSTART)
{
    ADC_CON0 = BIT(4) | BIT(3) ;
    P0PD &= ~BIT(2);
    P0PU &= ~BIT(2);
    P0DIR |= BIT(2);
    P0DIE &= ~BIT(2);                               //设置AD key IO方向和模拟输入属性
    LDO_CON |= BIT(1);                              //打开VBG
    key_msg_filter = music_msg_filter;              //默认配置
    ADC_CON1 = ad_table[0];
    ADC_CON0 |= BIT(7);
}

/*----------------------------------------------------------------------------*/
/**@brief   AD 扫描函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void ADC_scan(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void ADC_scan(void) AT(COMMON_CODE)
{
    _no_init static _data u8 channel;

    /*get last set AD channel value*/
    AD_var.wADValue[channel++] = ((u16)ADC_DATH<<8) | ADC_DATL;

    if (channel == MAX_AD_CHANNEL)
        channel = 0;

    ADC_CON1 = ad_table[channel];
    ADC_CON0 |= BIT(7);         //[7]Start
}

