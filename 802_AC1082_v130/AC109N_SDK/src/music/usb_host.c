/*--------------------------------------------------------------------------*/
/**@file    usb_host.c
   @brief   USB 接口函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/

#include "usb_host.h"
#include "device.h"
#include "USB_Device_Mode.h"


/*----------------------------------------------------------------------------*/
/**@brief   USB OTG 检测函数
   @param   void
   @return  void
   @author  Change.tsai
   @note	void USB_online_otp(void)
*/
/*----------------------------------------------------------------------------*/
_near_func void USB_online_otp(void) AT(COMMON_CODE)
{
    switch (USB_Online())
    {
    case 0x1:
        //my_puts("----Host Out\n");
        usb_host_status(0x0);
        break;
    case 0x2:
        usb_device_status(0x0);
        break;
    case 0x3:
        //my_puts("----Host In\n");
        usb_host_status(0x1);
        break;
    case 0x4:
        usb_device_status(0x1);
        break;
    default:
        break;
    }
}










