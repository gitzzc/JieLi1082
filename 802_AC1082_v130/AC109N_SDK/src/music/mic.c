/*--------------------------------------------------------------------------*/
/**@file     MIC.c
   @brief    MIC 模块相关函数
   @details
   @author
   @date    2012-11-19
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "DAC.h"
#include "MIC.h"
#include "main.h"
#include "iic.h"
#include "music_play.h"
#include "USB_Device_Mode.h"
#include "RTC_API.h"
#include "play_file.h"

_no_init u8 _data MicOnline;
extern _no_init bool _bit amux;

/*----------------------------------------------------------------------------*/
/** @brief: Mic 初始化
    @param: void
    @return:void
    @author:Juntham
    @note:  void dac_mic_init(void)
*/
/*----------------------------------------------------------------------------*/
void dac_mic_init(void) AT(DAC_CODE)
{
    P2PD &= ~BIT(3);
    P2PU &= ~BIT(3);
    P2DIR |= BIT(3);
    P2DIE &= ~BIT(3);
}

/*----------------------------------------------------------------------------*/
/** @brief: Mic 控制函数
    @param: void
    @return:void
    @author:Juntham
    @note:  void dac_mic_control(u8 on)
*/
/*----------------------------------------------------------------------------*/
_near_func void dac_mic_control(u8 on) AT(COMMON_CODE)
{
    if (on)
    {
        DAA_CON0 |= BIT(4);
        /*AUX L->DAC L/R*/
        DAA_CON0 |= BIT(5);
    /*以下如果或上BIT(6)BIT(7)实现MIC的左右声道输出*/
        DAA_CON3 |= BIT(7)|BIT(6)|BIT(5);
    }
    else
    {
        if (work_mode == MUSIC_MODE)
        {
            /*AUX L->DAC L/R*/
            DAA_CON2 &= ~0xC0;
            DAA_CON0 &= ~BIT(4);
        }
        DAA_CON0 &= ~BIT(5);
        DAA_CON3 = 0x0;
    }
}

/*----------------------------------------------------------------------------*/
/** @brief: Mic 增益控制函数
    @param: void
    @return:void
    @author:Juntham
    @note:  void dac_mic_gain_control(u8 vol)
*/
/*----------------------------------------------------------------------------*/

_near_func void dac_mic_gain_control(u8 vol) AT(COMMON_CODE)
{
    DAA_CON3 = ((DAA_CON3 & 0xe0) | vol);
}

/*----------------------------------------------------------------------------*/
/** @brief: DAC 通道设置函数
    @param: channel：数字通道/模拟通道
    @return:void
    @author:Juntham
    @note:  void dac_channel_sel_mic(u8 channel)
*/
/*----------------------------------------------------------------------------*/
_near_func void dac_channel_sel_mic(u8 channel) AT(COMMON_CODE)
{
    dac_fade_out();

    asm("PUSH 0xA8");
    asm("CLR 0xA8.7");
    DAA_CON4 &= 0xf0;

    if (MicOnline)
        DAA_CON0 &= ~BIT(0);
    else
        DAA_CON0 &= ~(BIT(5) | BIT(4) | BIT(0));

    amux = 0;
    if (channel == DAC_DECODER)
    {
        DAA_CON0 |= BIT(0);
        set_digital_vol(main_vol_L, main_vol_R);
    }
    else if (channel == DAC_AMUX0_L)      //P24
    {
        u8 temp;
        /*AUX 0 I/O Input*/
        P2DIE &= ~0x10;    //当前通道设为模拟IO，需要根据实际使用的IO情况进行修改
        P2DIR |= 0x10;
        temp = DAA_CON4 & 0xf0;
        temp |= 0x01;
        DAA_CON4 = temp;
        DAA_CON0 |= BIT(4);
        /*AUX L->DAC L/R*/
        DAA_CON2 |= 0xC0;
        amux = 1;
    }
    else if(channel == DAC_AMUX0_R)       //P25
    {
        u8 temp;
        /*AUX 0 I/O Input*/
        P2DIE &= ~0x20;    //当前通道设为模拟IO，需要根据实际使用的IO情况进行修改
        P2DIR |= 0x20;
        temp = DAA_CON4 & 0xf0;
        temp |= 0x02;
        DAA_CON4 = temp;
        DAA_CON0 |= BIT(4);
        /*AUX L->DAC L/R*/
        DAA_CON2 |= 0xC0;
        amux = 1;
    }
	else if(channel == DAC_AMUX1_L)       //P26
    {
        u8 temp;
        /*AUX 0 I/O Input*/
        P2DIE &= ~0x40;    //当前通道设为模拟IO，需要根据实际使用的IO情况进行修改
        P2DIR |= 0x40;
        temp = DAA_CON4 & 0xf0;
        temp |= 0x04;
        DAA_CON4 = temp;
        DAA_CON0 |= BIT(4);
        /*AUX L->DAC L/R*/
        DAA_CON2 |= 0xC0;
        amux = 1;
    }
	else if(channel == DAC_AMUX1_R)       //P27
    {
        u8 temp;
        /*AUX 0 I/O Input*/
        P2DIE &= ~0x80;    //当前通道设为模拟IO，需要根据实际使用的IO情况进行修改
        P2DIR |= 0x80;
        temp = DAA_CON4 & 0xf0;
        temp |= 0x08;
        DAA_CON4 = temp;
        DAA_CON0 |= BIT(4);
        /*AUX L->DAC L/R*/
        DAA_CON2 |= 0xC0;
        amux = 1;
    }
    asm("POP 0xA8");
    dac_fade_in();
}



