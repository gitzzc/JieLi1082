/*--------------------------------------------------------------------------*/
/**@file    hot_msg.c
   @brief   系统公共消息处理函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "hot_msg.h"
#include "msg.h"
#include "device.h"
#include "get_music_file.h"
#include "main.h"
#include "key.h"
#include "iic.h"
#include "DAC.h"
#include "music_play.h"
#include "UI_API.h"
#include "RTC_API.h"
#include "play_file.h"
#include "break_point.h"
#include "dac.h"
#include "IRTC.h"
#include "FM_API.h"
#ifdef POWER_CHECK_ENABLE	
extern _no_init  bool	_bit  power_off_online;
#endif
extern _no_init ENUM_WORK_MODE _data back_work_mode;

extern ENUM_WORK_MODE _data work_mode;

extern	_no_init  bool	_bit  fmone;
_no_init  bool	_bit  first;
extern u8 find_device(ENUM_DEVICE_SELECT_MODE select);

extern u8 _data main_vol_L, main_vol_R;
u8 _data  powerflag=0;
_no_init u16 _data input_number;	    ///<输入的数字的实际值
extern _near_func u8 get_usb_online_status(void);
extern _near_func u8 get_sdmmc_online_status(u8 sd_id);

extern	_no_init  bool	_bit  aux_online;
extern  bool init_fm_rev(void) ;
extern  __root void QN8035_powerdown(void);

//extern	void QND_SetVol(u8 vol) ;
	
_no_init u16 _data input_10;
extern  _no_init u16  countnum;
bool  muteflag=0;
_no_init u16 _data input_100;
//extern void QN8035_mute(u8 On);

u8 uabcard_out = 0;

#ifdef AD_MIC_DET
extern bool kalaok_button;
#endif


/*----------------------------------------------------------------------------*/
/** @brief:
    @param:
    @return:
    @author:Juntham
    @note:
*/
/*----------------------------------------------------------------------------*/
static void new_device_in(u8 dev) AT(CSTART)
{
    LED_FADE_ON();
    dev -= MSG_USB_DISK_IN;
    music_device_info[dev].wBreak_point_filenum = 0;
    music_device_info[dev].wfileTotal = 0;
    playfile.given_file_method = PLAY_BREAK_POINT;
    playfile.given_device = (ENUM_DEVICE_SELECT_MODE)dev;
    put_msg_lifo(MSG_MUSIC_NEW_DEVICE_IN);
}
/*----------------------------------------------------------------------------*/
/**@brief   公共消息集中处理函数
   @param   key： 需要处理的消息
   @return  void
   @author: Juntham
   @note    void ap_handle_hotkey(u8 key)
*/
/*----------------------------------------------------------------------------*/
void ap_handle_hotkey(u8 key) AT(CSTART)
{
    switch (key)
    {
#ifdef RTC_EN
    case MSG_ALM_ON:
#ifdef STANDBY_EN
        read_RTC((RTC_TIME *)(&curr_time));

        if((curr_time.bHour == curr_alarm.bHour)
            &&(curr_time.bMin == curr_alarm.bMin)
              &&(curr_alarm.bSw))
#endif
        {
#ifdef STANDBY_EN
            alm_on_flag = 1;
#endif
            UI_menu(MENU_ALM_UP);
            RTC_setting_var.bAlarmOn = 1;
            write_next_ALM();
            RTC_setting_var.bLastWorkMode = work_mode;
            if (work_mode != RTC_MODE)
            {
                work_mode = RTC_MODE;
                put_msg_lifo(MSG_CHANGE_WORK_MODE);     //必须使用高优先级消息，避免与解码错误消息冲突
            }
        }

        break;
#endif

    case MSG_MUTE:												//静音
       //my_printf("%04x\n",0XFe00);
      //  if (!Sys_Volume)
           // break;
        if(kalaok_button)  
        {
           break;
        }
        muteflag = !muteflag;
     // my_printf("%04x\n",muteflag);
    if (muteflag)
    {
      dac_mute(1);
	  //dac_mute_toggle(void)
	  if(work_mode == FM_RADIO_MODE)
	  	{
          fm_module_mute(1);

	    }
	  	
    // my_printf("%04x\n",muteflag);
    }
    else
    {
         if(work_mode == FM_RADIO_MODE)
	  	{
          fm_module_mute(0);
	    }
	
        dac_mute(0);
    }
		
      //  dac_mute_toggle();
        break;

    case MSG_USB_DISK_OUT:
    case MSG_SDMMCA_OUT:
#ifdef SDMMCB_EN
    case MSG_SDMMCB_OUT:
#endif
         //my_printf("%04x\n",0XFef1);
        LED_FADE_ON();

        device_check();
        if(work_mode == MUSIC_MODE)
        {
          if((device_online&0x03) == 0)	
          {
           //work_mode = FM_RADIO_MODE;

 #ifdef AD_MIC_DET
		    if (kalaok_button)
			 {
                 work_mode = SPE_MODE;
				
			 }
            else
#endif					
            {
				work_mode = FM_RADIO_MODE;
				
            }

		    put_msg_lifo(MSG_CHANGE_WORK_MODE);
			break;
          }
        }		
        /*当前活动设备不在线*/
        if ((device_check() == 0x0)&&(work_mode == MUSIC_MODE))
        {
                 /* if((device_online & 0x03) != 0)
				 	{
				 	    //my_printf("%04u\n",aux_online);
                        playfile.play_mode = REPEAT_ALL;
	                    LED_FADE_ON();
	                    music_device_info[DEVICE_UDISK].wBreak_point_filenum = 0;
	                    music_device_info[DEVICE_UDISK].wfileTotal = 0;
	                    playfile.given_file_method = PLAY_BREAK_POINT;
	                    playfile.given_device = DEVICE_UDISK;
	                    //put_msg_lifo(MSG_MUSIC_NEW_DEVICE_IN);
	                    work_mode = MUSIC_MODE;
						
					
				    }
	                  else*/
#if 0
                 if((device_online & 0x03) == 0)
                 	{
					  if(fmone)
					 work_mode = FM_RADIO_MODE;
					 else  if(aux_online)
					 work_mode = AUX_MODE;	
					  else
					  	work_mode = AUX_MODE;	
                      put_msg_lifo(MSG_CHANGE_WORK_MODE);
					       break;
                 	}
 #endif
            playfile.given_file_method = PLAY_BREAK_POINT;
            playfile.given_device = DEVICE_AUTO_NEXT;
            put_msg_lifo(MSG_MUSIC_SELECT_NEW_DEVICE);
            /*记忆断点信息*/
//#if defined BREAK_POINT_PLAY_EN && defined USE_EEPROM_MEMORY
#if defined BREAK_POINT_PLAY_EN && !defined USE_IRTC_MEMORY
            save_music_break_point(device_active);
#endif
         //  uabcard_out = 2;
        }
		else
		{
		// uabcard_out = 0;
		}
        break;

#ifdef USB_DEVICE_EN
    case MSG_USB_PC_IN:
#ifdef SDMMCA_EN
        music_device_info[DEVICE_SDMMCA].wBreak_point_filenum = 0;
        music_device_info[DEVICE_SDMMCA].wfileTotal = 0;
#endif
#ifdef SDMMCB_EN
        music_device_info[DEVICE_SDMMCB].wBreak_point_filenum = 0;
        music_device_info[DEVICE_SDMMCB].wfileTotal = 0;
#endif
        LED_FADE_ON();
        work_mode = USB_DEVICE_MODE;
        put_msg_lifo(MSG_CHANGE_WORK_MODE);
        break;

#endif
    case MSG_AUX_IN :
        LED_FADE_ON();
		 if(work_mode == AUX_MODE)
		 {
		    dac_channel_sel(DAC_AMUX0_L);
		 	break;
		 }
#ifdef USB_DEVICE_EN
        if (work_mode != USB_DEVICE_MODE)
#endif
        {
            work_mode = AUX_MODE;
            put_msg_lifo(MSG_CHANGE_WORK_MODE);
        }
        break;

    case MSG_AUX_OUT :
        LED_FADE_ON();
		if(work_mode == AUX_MODE)
			{
			    device_check();
		        if(device_online&0x03)
		        {
					work_mode = MUSIC_MODE;
					put_msg_lifo(MSG_CHANGE_WORK_MODE);
		        }	
				else
				{
			     dac_channel_sel(DAC_DECODER);				
				}
			}
        break;
#ifdef SDMMCA_EN
    case MSG_SDMMCA_IN :
#endif

#ifdef SDMMCB_EN
    case MSG_SDMMCB_IN :
#endif

#ifdef USB_DISK_EN
    case MSG_USB_DISK_IN  :
#endif
       //my_printf(" %04u\n",key);
       // if(aux_online)////////////////////////////byf
		//   break;
		muteflag = 0;
        new_device_in(key);
        break;

    case MSG_NEXT_WORKMODE:
#ifdef USB_DEVICE_EN
		if(work_mode == USB_DEVICE_MODE)
		{
		  break;
		}	
#endif
        device_check();
        if(work_mode == MUSIC_MODE)
        {
                if((device_online&0x03) == 0x03)
                {
                    if (device_active == DEVICE_UDISK)
                    {
                        put_msg_lifo(MSG_SDMMCA_IN);
                        break;
                    }
                    if ((device_active == DEVICE_SDMMCA)&&(!kalaok_button))
                    {
                        put_msg_lifo(MSG_USB_DISK_IN);
                        break;
                    }					
                }
				
#ifdef AD_MIC_DET
			  if(kalaok_button)
			  {
				work_mode = SPE_MODE;
			  }
			  else
#endif
			  {
				//work_mode = FM_RADIO_MODE;
				break;
			  }

        }
#ifdef AD_MIC_DET	
        else if(work_mode == SPE_MODE)
        {
            if((device_online&0x03))
            {
                work_mode = MUSIC_MODE;
                playfile.given_device = DEVICE_UDISK;
            }
			else
                break;
            //work_mode = FM_RADIO_MODE;
        }	
#endif
        else if(work_mode == FM_RADIO_MODE)
        {
            if((device_online&0x03))
            {
               work_mode = MUSIC_MODE;
                playfile.given_device = DEVICE_UDISK;
             }
#ifdef AD_MIC_DET	
			else if(kalaok_button)
			{
			  work_mode = SPE_MODE;
			}
#endif
			else
			{
                break;
			}
        }		
        else if(work_mode == AUX_MODE)
        {
        break;
		            /*if((device_online&0x03))
		            {
		                work_mode = MUSIC_MODE;
		                playfile.given_device = DEVICE_UDISK;
		            }
					else
		              work_mode = FM_RADIO_MODE;*/
        }
        put_msg_lifo(MSG_CHANGE_WORK_MODE);
        break;

#if 0
     case  MSG_USB_TO_SD:


		
		device_check();
		
		//my_printf("%04u\n",(device_online&0x03));
		if(work_mode == MUSIC_MODE)
		{
		  if(((device_online & 0x03)!= 0x03)&&(!fmone)&&(!aux_online))
		  	     break;
			if((device_online & 0x03) == 0x03)
            {
               // my_printf("%04u\n",(device_online&0x03));

			    //my_printf("%04u\n",aux_online);
                if(device_active  == DEVICE_UDISK)
                {
                	playfile.play_mode = REPEAT_ALL;
                    LED_FADE_ON();
                    music_device_info[DEVICE_SDMMCA].wBreak_point_filenum = 0;
                    music_device_info[DEVICE_SDMMCA].wfileTotal = 0;
                    playfile.given_file_method = PLAY_BREAK_POINT;
                    playfile.given_device = DEVICE_SDMMCA;
                    put_msg_lifo(MSG_MUSIC_NEW_DEVICE_IN);
					break;
                }	
				else  if(fmone)
				work_mode = FM_RADIO_MODE;
				 else  if(aux_online)
				 work_mode = AUX_MODE;	
				 else  if((device_online & 0x03) != 0)
				 	{
				 	    //my_printf("%04u\n",aux_online);
                        playfile.play_mode = REPEAT_ALL;
	                    LED_FADE_ON();
	                    music_device_info[DEVICE_UDISK].wBreak_point_filenum = 0;
	                    music_device_info[DEVICE_UDISK].wfileTotal = 0;
	                    playfile.given_file_method = PLAY_BREAK_POINT;
	                    playfile.given_device = DEVICE_UDISK;
	                    put_msg_lifo(MSG_MUSIC_NEW_DEVICE_IN);
						break;
					
				    }
				     else
					 	break;
			}
             else  if(fmone)
				work_mode = FM_RADIO_MODE;
				 else  if(aux_online)
				 work_mode = AUX_MODE;	
				 else  if((device_online & 0x03) != 0)
				 	{
				 	    //my_printf("%04u\n",aux_online);
                        playfile.play_mode = REPEAT_ALL;
	                    LED_FADE_ON();
	                    music_device_info[DEVICE_UDISK].wBreak_point_filenum = 0;
	                    music_device_info[DEVICE_UDISK].wfileTotal = 0;
	                    playfile.given_file_method = PLAY_BREAK_POINT;
	                    playfile.given_device = DEVICE_UDISK;
	                    put_msg_lifo(MSG_MUSIC_NEW_DEVICE_IN);
						break;
					
				    }
				     else
					 	break;

			
			///if(device_online & 0x03)
			///	work_mode = AUX_MODE;
			///else
			
				 	
		}
		else if(work_mode == FM_RADIO_MODE)
		{
			if(aux_online)
				work_mode = AUX_MODE;
			else if((device_online & 0x03))
			{
				work_mode = MUSIC_MODE;
				playfile.given_device = DEVICE_UDISK;
				put_msg_lifo(MSG_CHANGE_WORK_MODE);
			}
			else
				break;
		}
		else if(work_mode == AUX_MODE)
		///e if(work_mode == RTC_MODE)
		{
			if((device_online & 0x03))
			{
				work_mode = MUSIC_MODE;
				playfile.given_device = DEVICE_UDISK;
				put_msg_lifo(MSG_CHANGE_WORK_MODE);
			}
			else  if(fmone)
				work_mode = FM_RADIO_MODE;
			    else
					break;
		}
        ///work_mode++;
        put_msg_lifo(MSG_CHANGE_WORK_MODE);
        break;
#endif		

#if 0

           device_check();

		    my_printf("%04u\n",(device_online&0x03));

          if((device_online&0x03)==0)
          	{

			    if(work_mode == FM_RADIO_MODE)
	           	{
	                if(aux_online)
	                {

						work_mode=AUX_MODE;
				        put_msg_lifo(MSG_CHANGE_WORK_MODE);
				        break;


					}


			    }
				 else  if(fmone&&(work_mode == AUX_MODE))
				 	{

						
                        work_mode=FM_RADIO_MODE;
				        put_msg_lifo(MSG_CHANGE_WORK_MODE);
				        break;


				    }
				     else  if(fmone)
				     	{

                            work_mode=FM_RADIO_MODE;
					        put_msg_lifo(MSG_CHANGE_WORK_MODE);
					        break;

					    }


		    }


		
			
           if(work_mode == FM_RADIO_MODE)
           	{
                if(aux_online)
                {

					work_mode=AUX_MODE;
			        put_msg_lifo(MSG_CHANGE_WORK_MODE);
			        break;


				}
			


		    }
		



          if(((device_online&0x03) == 0x01)||((device_online&0x03) == 0x02))
           	{
           	   // my_printf("%04x\n",0XFe01);

				//if( device_active == DEVICE_SDMMCA)
					{
	                   if(fmone&&first)
	                  	{
	                  	    first=0;
							
							
	                        work_mode=FM_RADIO_MODE;
					        put_msg_lifo(MSG_CHANGE_WORK_MODE);
					        break;

					    }
					}

                device_active = DEVICE_UDISK;
		    }

			
			
           if((device_online&0x03) == 0x03)
           	{
                // my_printf("%04x\n",(device_online&0x03));
			
                       if ((get_sdmmc_online_status(0x01) != 0)&&(device_active == DEVICE_UDISK))
                       // if((device_online&0x01))
				    	{
		                    device_online |= BIT(DEVICE_SDMMCA);
							put_msg_lifo(MSG_SDMMCA_IN);
		                    //  work_mode= MUSIC_MODE;
		                        break;

					    }		
	                    else  if((device_active == DEVICE_SDMMCA)&&first)
	                	{
	                	
								
	                            work_mode=FM_RADIO_MODE;
						        put_msg_lifo(MSG_CHANGE_WORK_MODE);
						        break;

					    }
				      else   if (((get_usb_online_status() & BIT(0)) != 0)&&(device_active == DEVICE_SDMMCA)&&(!first))
					    	{
				              device_online |= BIT(DEVICE_UDISK);
							 //  work_mode= MUSIC_MODE;
				                  put_msg_lifo(MSG_USB_DISK_IN);
							            break;
					    	}

		    }
		     else
		     	{

				//  my_printf("%04x\n",0XFe03);

				  //  my_printf("%04x\n",(device_online&0x03));
				
                    if((device_online&0x03)==0x02)
                	{
                        if ((get_sdmmc_online_status(0x01) != 0)&&(device_active == DEVICE_UDISK))
				    	{
		                    device_online |= BIT(DEVICE_SDMMCA);
							put_msg_lifo(MSG_SDMMCA_IN);
		                    //  work_mode= MUSIC_MODE;

					    }

                	}
					 else  if((device_online&0x03)==0x01)
					 	{
					 	       //my_printf("%04x\n",0Xeeee);
					 	      device_active = DEVICE_SDMMCA;
                              if (((get_usb_online_status() & BIT(0)) != 0)&&(device_active == DEVICE_SDMMCA))
						    	{
						    	 // my_printf("%04x\n",0XFe01);
					              device_online |= BIT(DEVICE_UDISK);
								 //  work_mode= MUSIC_MODE;
					                  put_msg_lifo(MSG_USB_DISK_IN);
						    	}


					    }



					/*
				   else   if (((get_usb_online_status() & BIT(0)) != 0)&&(device_active == DEVICE_SDMMCA))
			    	{
		              device_online |= BIT(DEVICE_UDISK);
					 //  work_mode= MUSIC_MODE;
		                  put_msg_lifo(MSG_USB_DISK_IN);
			    	}
				    else   if ((get_sdmmc_online_status(0x01) != 0)&&(device_active == DEVICE_UDISK))
				    	{
		                    device_online |= BIT(DEVICE_SDMMCA);
							put_msg_lifo(MSG_SDMMCA_IN);
		                    //  work_mode= MUSIC_MODE;

					    } */
		
		     	}

			
		  break;
#endif
		
	case MSG_POWER_ON:
		 // if(power_off_online)
		 // 	break;
		 powerflag=1;
    case MSG_POWER_IN:	
		   back_work_mode = work_mode;
		   work_mode=NO_DEVICE_OFF;
		   put_msg_lifo(MSG_CHANGE_WORK_MODE);	
          break;

    case MSG_VOL_UP:
        flush_all_msg();
        if (!Sys_Volume)
            break;
        main_vol_L++;
        main_vol_L++;
    case MSG_VOL_DOWN:
        flush_all_msg();
        if (!Sys_Volume)
            break;
        main_vol_L--;
        if (main_vol_L == 255)
            main_vol_L = 0;
        else if (main_vol_L > MAX_MAIN_VOL_L)
            main_vol_L = MAX_MAIN_VOL_L;

        main_vol_R = main_vol_L;

		// my_printf("%04u\n",main_vol_R);
		
		//if(work_mode == FM_RADIO_MODE)
		//fm_module_vol(main_vol_L);	
		//else
        set_main_vol(main_vol_L, main_vol_R);
		
		if(main_vol_R == 0)
		{
		   dac_mute(1);
		  //my_puts("muteon\n");
		}
		else
		{
		
		  if(main_vol_R > 0)
		  	{
		         if((work_mode == FM_RADIO_MODE)&&muteflag)
			  	{
		          // QN8035_mute(0);	
		           fm_module_mute(0);
			    }		  	
		  	   if(muteflag == 1)
		  	   	{
		          dac_mute(0);	
		  	   	}
		  	}

		}
			
        //deg("Vol-L %u/R %u\n", main_vol_L, main_vol_R);
        ///set_memory(MEM_VOL_L, main_vol_L);
        ///set_memory(MEM_VOL_R, main_vol_R);
      //  my_printf("%04u\n",main_vol_L);
        //UI_menu(MENU_MAIN_VOL);
        break;



    case MSG_0:
    case MSG_1:
    case MSG_2:
    case MSG_3:
    case MSG_4:
    case MSG_5:
    case MSG_6:
    case MSG_7:
    case MSG_8:
    case MSG_9:
	

        if (!Sys_IRInput)
            break;

	   if((input_100 != 0)||(input_10 != 0))
	   	countnum=countnum+key;
	   else
	   	{
	     input_number = input_number * 10 + key;
          countnum = input_number;

	   	}

	       if (countnum > 9999)
        	{
               input_number = 0;
			   input_10=0;
			   countnum=0;
			   input_100=0;

        	}
#ifdef UART_PRINT_ENABLE			
				my_printf("input_number =%d\n",input_number);
#endif	


		//my_printf("countnum = %04u\n",countnum); //u  10   x  16
        UI_menu(MENU_INPUT_NUMBER);
        break;
#if 0		
      case MSG_10NIC:

		
          input_10 = input_10+10;

		  countnum=countnum+10;

			
		    if (countnum > 9999)
        	{
               input_number = 0;
			   input_10=0;
			   countnum=0;
			   input_100=0;

        	}

           UI_menu(MENU_INPUT_NUMBER);
         	//my_printf("countnum = %04u\n",countnum);
          break;

		case MSG_100NIC:

			 input_100=input_100+100;

			 countnum=100+countnum;


			
			 if (countnum > 9999)
			   {
				  input_number = 0;
				  input_10=0;
				  countnum=0;
				  input_100=0;
	
			   }
			// my_printf("countnum = %04u\n",countnum);
             UI_menu(MENU_INPUT_NUMBER);
			break;
#endif
		
    }
}

