/*--------------------------------------------------------------------------*/
/**@file    get_music_file.c
   @brief   文件查找接口函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/
#include "config.h"
#include "music_play.h"
#include "get_music_file.h"
#include "device.h"
#include "iic.h"
#include "play_file.h"

_no_init SELECT_PLAY_FILE _data playfile;
_no_init FF_APIS _xdata ff_api;
_no_init FF_APIS ff_api_tmp AT(FF_API_TEMP);                ///<在没有解码时使用

/*Path search example*/
const u8 Specific_path[] =
{
    "/1       /B???????MP3",
};

const u8 Specific_path1[] =
{
    "/1       /B*MP3",
};
/*----------------------------------------------------------------------------*/
/**@brief   获取一个随机数
   @param   无
   @return  获取到的随机数
   @author: Juntham
   @note    u16 get_u16_random()
*/
/*----------------------------------------------------------------------------*/
u16 get_u16_random(void) AT(GET_MUSIC_FILE)
{
    u16 *ptr;
    u16 res;

    ptr = (u16 *)0x200;
    res = *ptr++;
    res = *ptr + TMR2_CNTL;

    return res;
}

/*----------------------------------------------------------------------------*/
/** @brief: 获取文件夹内文件总数函数（仅在没有解码时使用）
    @param: void
    @return:void
    @author:Juntham
    @note:  void get_dir_file_total(void)
*/
/*----------------------------------------------------------------------------*/
#ifdef FOLDER_PLAY_EN
static void get_dir_file_total(void) AT(GET_MUSIC_FILE)
{
    u16 fileNum;
    //my_printf("\n0-InDir : %04x/OutDir : %04x\n", ff_api.fs_msg.fileTotalInDir, ff_api.fs_msg.fileTotalOutDir);
    if (ff_api.fs_msg.fileTotalInDir == 0)
    {
        stop_decode();
        my_memcpy((u8 *)&ff_api_tmp, (u8 *)&ff_api, sizeof(FF_APIS));

        /*同步Maskrom 指针*/
        ff_api_tmp.fs_msg.fileNumber = 0x0101;
        fs_getfile_bynumber(&ff_api_tmp,0x0101);

        ff_api.fs_msg.fileTotalInDir = fs_getfile_totalindir_app(&ff_api_tmp);
        //my_printf("1-InDir : %04x/OutDir : %04x\n", ff_api.fs_msg.fileTotalInDir, ff_api.fs_msg.fileTotalOutDir);
        /*恢复Maskrom 指针*/
        fileNum = ff_api.fs_msg.fileNumber;
        ff_api.fs_msg.fileNumber = 0x0101;
        fs_getfile_bynumber(&ff_api,ff_api.fs_msg.fileNumber);
        ff_api.fs_msg.fileNumber = fileNum;
    }
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief   获取文件号
   @param   playmode   循环播放模式
   @param   searchMode  0：上一个文件；1：下一个文件；2：在强行REPEAT_ONE模式時在REPEAT_ALL模式下查找文件
   @return  0:fail 1:success
   @author: Juntham
   @note    u8 fs_get_filenum(ENUM_PLAY_MODE playmode, ENUM_FILE_SELECT_MODE searchMode)
*/
/*----------------------------------------------------------------------------*/
u8 fs_get_filenum(ENUM_PLAY_MODE playmode, ENUM_FILE_SELECT_MODE searchMode) AT(GET_MUSIC_FILE)
{
    u16 fileTotal = music_device_info[device_active].wfileTotal;

    if (searchMode == PLAY_FIRST_FILE)				//播放第一个文件
    {
        playfile.given_file_number = 1;
        return GET_FILE_OK;
    }
    else if (searchMode == PLAY_LAST_FILE)				//播放最后一个文件
    {
        playfile.given_file_number = fileTotal;
        return GET_FILE_OK;
    }
    else if (searchMode == PLAY_BREAK_POINT)				//播放记忆的文件序号和断点
    {
#ifdef LAST_MEM_FILE_PLAY_EN
        if (find_break_point_file_flag)                 /*记忆文件有效*/
        {
            find_break_point_file_flag = 0;
            playfile.given_file_number = 0;
        }
        else
        {
            playfile.given_file_number = music_device_info[device_active].wBreak_point_filenum;
            if (playfile.given_file_number == 0)
                playfile.given_file_number = 1;
        }
#else
        playfile.given_file_number = 1;
#endif
        return GET_FILE_OK;
    }

    if ((playmode == REPEAT_ONE) && (searchMode != PLAY_AUTO_NEXT))
    {
        playmode = REPEAT_ALL;					//在单曲循环模式下，转换成全循环模式
    }
    switch (playmode)
    {
#ifdef RANDOM_PLAY_EN
    case REPEAT_RANDOM:
        playfile.given_file_number = get_u16_random();
        playfile.given_file_number = (playfile.given_file_number % fileTotal) + 1;
        break;
#endif

    case REPEAT_ALL:
        if (searchMode == PLAY_PREV_FILE)					//prev file
        {
            playfile.given_file_number--;
            if (playfile.given_file_number == 0)
            {
                return GET_DEVICE_HEAD;
            }
        }
        else					                //next file
        {
            playfile.given_file_number++;
            if (playfile.given_file_number > fileTotal)
            {
                playfile.given_file_number = 1;
                return GET_DEVICE_END;
            }
        }
        break;
#ifdef FOLDER_PLAY_EN
    case REPEAT_FOLDER:
        {
            get_dir_file_total();
            if (searchMode == PLAY_PREV_FILE)
            {
                playfile.given_file_number--;
                if (playfile.given_file_number == ff_api.fs_msg.fileTotalOutDir)
                    playfile.given_file_number = ff_api.fs_msg.fileTotalOutDir + ff_api.fs_msg.fileTotalInDir;
            }
            else
            {
                playfile.given_file_number++;
                if (playfile.given_file_number > (ff_api.fs_msg.fileTotalOutDir + ff_api.fs_msg.fileTotalInDir))
                    playfile.given_file_number = ff_api.fs_msg.fileTotalOutDir + 1;
            }
        }
        break;
#endif
    }

    return GET_FILE_OK;
}



