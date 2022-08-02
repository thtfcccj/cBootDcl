/***********************************************************************

//		                写Boot区管理实现

***********************************************************************/

#include "WrBootMng.h"

struct _WrBootMng WrBootMng;

//Boot分区情况，下标0为BOOT扇区个数，其它下标依次对应分区包起始位置
#ifndef BOOT_SECTOR_TO_PACKET_LUT
  #define BOOT_SECTOR_TO_PACKET_LUT {1,FLASH_PAGE_SIZE >> 8}//默认1个扇区
#endif

static const unsigned short _SectorToPacktLut[] = BOOT_SECTOR_TO_PACKET_LUT;

/***********************************************************************
                              相关函数实现
***********************************************************************/

//--------------------------------初始化-------------------------------
void WrBootMng_Init(void)
{
  WrBootMng.Timer = 0;
}

//--------------------------------任务函数------------------------------
//放入1s进程
void WrBootMng_Task(void);


//-----------------------------接收数据处理-----------------------------
void WrBootMng_Rcv(unsigned char *pData,//从子功能码起
                     unsigned short Len)//数据长度
{
  
  
}


