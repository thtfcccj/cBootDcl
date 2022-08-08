/***********************************************************************

		   cBootDcl��ģ��-��дBoot��boot��ΪMCU�ڲ�FLASHʱ��ʵ��

***********************************************************************/
#ifndef __C_BOOT_DCL_BOOT_IN_FLASH_H
#define __C_BOOT_DCL_BOOT_IN_FLASH_H

#include "cBootDcl.h"
#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ

/**********************************************************************
                           �������
***********************************************************************/

//����Adler32��ʼֵ����ʵ�ֿ�Դ���˽�л�,32λ
#ifndef C_BOOT_ADLER32_INIT
  #define C_BOOT_ADLER32_INIT  1
#endif

/**********************************************************************
                           �ص�����
***********************************************************************/

//-----------------------------�õ�UID--------------------------------
//��IDӦ���豸���ܹ���ʾ���ܹ����ʵ�����:MCU�ڲ���ID�ŵ�
unsigned long cBootDcl_BootInFlash_cbGetUID(void);

//--------------------------�Ƿ��������дboot״̬---------------------
//��ع���ʱ������������������������������
//���أ��������ʱ C_BOOT_DCL_ERR_NO
//      ϵͳæʱ�� C_BOOT_DCL_ERR_BUSY
//      ��Դ����ʱ:C_BOOT_DCL_ERR_POW
signed char cBootDcl_BootInFlash_cbIsEnEnter(void);

//------------------------����������Boot����״̬---------------------
//Ӧע�Ᵽ��ϵͳ��ز�����������BOOT״̬
//ͨ��Reason����Boot���ݽ���ԭ��0�������룬�����û�����
void cBootDcl_BootInFlash_cbEnterBoot(signed char Reason);

#endif //#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ
#endif






