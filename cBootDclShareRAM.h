/***********************************************************************

		            cBootDcl��ģ��-Boot��APP�˹���RAMʵ��

***********************************************************************/
#ifndef _C_BOOT_DCL_SHARE_RAM_H
#define	_C_BOOT_DCL_SHARE_RAM_H

#ifdef SUPPORT_EX_PREINCLUDE//��֧��Preinlude�r
  #include "Preinclude.h"
#endif

/***********************************************************************
                         �������
***********************************************************************/

//���������С���� >=4����4��Ϊ��ϵ
#ifndef C_BOOT_DCL_SHARE_RAM_COUNT    
  #define C_BOOT_DCL_SHARE_RAM_COUNT  16
#endif

/***********************************************************************
                         ��ؽṹ
***********************************************************************/

struct _cBootDclShareRAM{
  //���������ɴ����ͨѶ��ַ��֡�����ͨѶ����(�粨����)���轻����boot����Ϣ
  unsigned char Data[C_BOOT_DCL_SHARE_RAM_COUNT];  
  //������У����,boot���ô˼��Data�Ƿ���Ч
  unsigned long Adler32; 
};

//������������������Ӧ�ò�ʵ�֣� ���ڿ���������أ�������ڶ�ջ���
//app��ʱ����ֹ�Ż������ʼ��Ϊȫ0
//boot��ʱ����ֹ�Ż�����ֹʵʼ��
extern struct _cBootDclShareRAM cBootDclShareRAM; 

/***********************************************************************
                              ��غ���
***********************************************************************/
#include "Adler32.h"
#include <string.h>
//---------------------------App�˳�ʼ��-------------------------------
//�˺�������APP�˵���
#define cBootDclShareRAM_AppInit() do{\
  memset(&cBootDclShareRAM, 0, sizeof(struct _cBootDclShareRAM));}while(0)

//-----------------------App��Data�ı�����----------------------------
//���Ը���Adler32ֵ
#define cBootDclShareRAM_AppDataChangedUpdate() do{cBootDclShareRAM.Adler32 = \
  Adler32_Get(1, cBootDclShareRAM.Data, C_BOOT_DCL_SHARE_RAM_COUNT);}while(0)

//----------------------boot�˼��������Ƿ���Ч------------------------
//���Լ���Ƿ�Ϊapp��д�������
#define cBootDclShareRAM_BootIsValid()  (cBootDclShareRAM.Adler32 == \
  Adler32_Get(1, cBootDclShareRAM.Data, C_BOOT_DCL_SHARE_RAM_COUNT))


#endif

