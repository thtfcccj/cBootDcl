/***********************************************************************

//		                дBoot������ʵ��

***********************************************************************/

#include "WrBootMng.h"

struct _WrBootMng WrBootMng;

//Boot����������±�0ΪBOOT���������������±����ζ�Ӧ��������ʼλ��
#ifndef BOOT_SECTOR_TO_PACKET_LUT
  #define BOOT_SECTOR_TO_PACKET_LUT {1,FLASH_PAGE_SIZE >> 8}//Ĭ��1������
#endif

static const unsigned short _SectorToPacktLut[] = BOOT_SECTOR_TO_PACKET_LUT;

/***********************************************************************
                              ��غ���ʵ��
***********************************************************************/

//--------------------------------��ʼ��-------------------------------
void WrBootMng_Init(void)
{
  WrBootMng.Timer = 0;
}

//--------------------------------������------------------------------
//����1s����
void WrBootMng_Task(void);


//-----------------------------�������ݴ���-----------------------------
void WrBootMng_Rcv(unsigned char *pData,//���ӹ�������
                     unsigned short Len)//���ݳ���
{
  
  
}


