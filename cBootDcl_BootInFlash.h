/***********************************************************************

		   cBootDcl��ģ��-��дBoot��boot��ΪMCU�ڲ�FLASHʱ��ʵ��

***********************************************************************/
#ifndef __C_BOOT_DCL_BOOT_IN_FLASH_H
#define __C_BOOT_DCL_BOOT_IN_FLASH_H

#include "cBootDcl.h"
#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ

/**********************************************************************
                           �ص�����
***********************************************************************/

//-----------------------------�õ�UID--------------------------------
//��IDӦ���豸���ܹ���ʾ���ܹ����ʵ�����:MCU�ڲ���ID�ŵ�
unsigned short cBootDcl_BootInFlash_cbGetUID(void);

//--------------------------�Ƿ��������дboot״̬---------------------
//��ع���ʱ������������������������������
//���أ��������ʱ C_BOOT_DCL_ERR_NO
//      ϵͳæʱ�� C_BOOT_DCL_ERR_BUSY
//      ��Դ����ʱ:C_BOOT_DCL_ERR_POW
signed char cBootDcl_BootInFlash_cbIsEnEnter(void);

//------------------------����������Boot����״̬---------------------
//Ӧע�Ᵽ��ϵͳ��ز�����������BOOT״̬
void cBootDcl_BootInFlash_cbEnterBoot(void);


#endif //#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ
#endif






