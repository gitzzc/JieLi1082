/*******************************************************************************************
File Name:  DAC.h

Version:    1.00

Discription: DAC 模块头文件

Author:     Caibingquan

Email:      change.tsai@gmail.com

Date:       2012.06.04

Copyright:(c) 2012 , All Rights Reserved.
*******************************************************************************************/
#ifndef	_MIC_H_
#define _MIC_H_


#include "config.h"


#ifdef __SMART_CPU__

extern _no_init u8 _data MicOnline;

void dac_mic_init(void);
_near_func void dac_mic_control(u8 on);
_near_func void dac_mic_gain_control(u8 vol);
_near_func void dac_channel_sel_mic(u8 channel);

#define DAC_AMUX0_L 3
#define DAC_AMUX0_R 4
#define DAC_AMUX1_L 5
#define DAC_AMUX1_R 6

#endif


#endif
