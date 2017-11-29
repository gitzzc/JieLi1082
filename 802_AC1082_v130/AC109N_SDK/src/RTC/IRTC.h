#ifndef	_IRTC_H_
#define _IRTC_H_

#include "config.h"

#ifdef __IAR_SYSTEMS_ICC__

typedef struct _RTC_TIME
{
    u16 dYear;		///<年份
    u8 	bMonth;		///<月份
    u8 	bDay;		///<天数
    u8 	bHour;		///<时
    u8 	bMin;		///<分
    u8 	bSec;		///<秒
    u8 	bWeekday;	///<星期几
}RTC_TIME;

typedef struct _ALM_TIME
{
    u8 	bHour;		///<时
    u8 	bMin;		///<分
    u8 	bSw;		///<闹钟开关
    u8  bmWeekdays; ///<响闹日期
}ALM_TIME;

/********************************************/
/*
            Bit Map
    7   |   6   |   5   |   4   |   3   |   2   |   1   |   0
    Sun     Sat     Fri     Thu     Wed     Tue     Mon     Null
*/
/********************************************/
#define SUNDAY      BIT(7)
#define SATURDAY    BIT(6)
#define FRIDAY      BIT(5)
#define THURSDAY    BIT(4)
#define WEDNESDAY   BIT(3)
#define TUESDAY     BIT(2)
#define MONDAY      BIT(1)

#define ALM_WEEKDAYS    (MONDAY|SUNDAY)

#define TRUE 		1
#define FALSE 		0

#define	ONEDAY 		(24*60*60LU)	//86400
#define ONEHOUR 	(60*60)		//3600
#define ONEMIN		60

#define YEAR 		2000U
#define MONTH 		1
#define DAY 		1
#define HOUR 		0
#define MINUTES 	0
#define SECOND 		0

#define DATE 		6

extern RTC_TIME __idata curr_time;
extern ALM_TIME __idata curr_alarm;

/*******************************************/
/*
            Maskrom API
                --IRTC Part
*/
/*******************************************/
/*----------------------------------------------------------------------------*/
/**@brief 	写RTC
   @param 	void
   @return  void
   @note  	void write_RTC(RTC_TIME *curr_time)
*/
/*----------------------------------------------------------------------------*/
_near_func void write_RTC(RTC_TIME _xdata *curr_time);
/*----------------------------------------------------------------------------*/
/**@brief 	读RTC
   @param 	void
   @return  void
   @note  	void read_RTC(RTC_TIME *curr_time)
*/
/*----------------------------------------------------------------------------*/
_near_func void read_RTC(RTC_TIME _xdata *curr_time);
/*----------------------------------------------------------------------------*/
/**@brief 	写RTC ALM
   @param 	void
   @return  void
   @note  	void write_ALM(RTC_TIME *curr_time_alm)
*/
/*----------------------------------------------------------------------------*/
_near_func void write_ALM(RTC_TIME _xdata *curr_time_alm);
/*----------------------------------------------------------------------------*/
/**@brief 	读RTC ALM
   @param 	void
   @return  void
   @note  	void read_ALM(RTC_TIME *curr_time_alm)
*/
/*----------------------------------------------------------------------------*/
_near_func void read_ALM(RTC_TIME _xdata *curr_time_alm);
/*----------------------------------------------------------------------------*/
/**@brief 	写 IRTC RAM
   @param 	addr：地址 dat：数据
   @return  void
   @note  	void write_IRTC_RAM(u8 addr, u8 dat)
*/
/*----------------------------------------------------------------------------*/
_near_func void write_IRTC_RAM(u8 addr, u8 dat);
/*----------------------------------------------------------------------------*/
/**@brief 	读 IRTC RAM
   @param 	addr：地址
   @return  数据
   @note  	u8 read_IRTC_RAM(u8 addr)
*/
/*----------------------------------------------------------------------------*/
_near_func u8 read_IRTC_RAM(u8 addr);
/*----------------------------------------------------------------------------*/
/**@brief 	写IRTC
   @param 	cmd：写指令 w_time:写入日期
   @return  void
   @note  	void write_IRTC(u8 cmd, RTC_TIME *w_time)
*/
/*----------------------------------------------------------------------------*/
_near_func void write_IRTC(u8 cmd, RTC_TIME _xdata *w_time);
/*----------------------------------------------------------------------------*/
/**@brief 	读IRTC
   @param 	cmd：读指令 r_time：读出的日期
   @return  void
   @note  	void read_IRTC(u8 cmd, RTC_TIME *r_time)
*/
/*----------------------------------------------------------------------------*/
_near_func void read_IRTC(u8 cmd, RTC_TIME _xdata *r_time);
/*----------------------------------------------------------------------------*/
/**@brief 	写RTC寄存器
   @param 	cmd：寄存器值
   @return  void
   @note  	void write_RTC_reg(u8 cmd, u8 reg)
*/
/*----------------------------------------------------------------------------*/
_near_func void write_RTC_reg(u8 cmd, u8 reg);
/*----------------------------------------------------------------------------*/
/**@brief 	读RTC寄存器
   @param 	void
   @return  void
   @note  	u8 read_RTC_reg(u8 cmd)
*/
/*----------------------------------------------------------------------------*/
_near_func u8 read_RTC_reg(u8 cmd);
#endif /*	__IAR_SYSTEMS_ICC__	*/


#endif /* _IRTC_H_ */
