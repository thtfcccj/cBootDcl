/***********************************************************************

		            cBootDcl主模块-Boot与APP端共享RAM实现

***********************************************************************/
#ifndef _C_BOOT_DCL_SHARE_RAM_H
#define	_C_BOOT_DCL_SHARE_RAM_H

#ifdef SUPPORT_EX_PREINCLUDE//不支持Preinlude時
  #include "Preinclude.h"
#endif

/***********************************************************************
                         相关配置
***********************************************************************/

//定义cBootDclShareRAM.Adler32使用识别器交换数据,否则使用Adler32校验码
//同时定义识别值，BOOT误别到此特殊值时，进入BOOT
//#define C_BOOT_DCL_SHARE_RAM_SW_IDENT    0x62653654

//共享区域大小，需 >=4且以4倍为关系
#ifndef C_BOOT_DCL_SHARE_RAM_COUNT    
  #define C_BOOT_DCL_SHARE_RAM_COUNT  16
#endif

/***********************************************************************
                         相关结构
***********************************************************************/

struct _cBootDclShareRAM{
  //数据区，可存放如通讯地址，帧间隔，通讯参数(如波特率)等需交代给boot的信息
  unsigned char Data[C_BOOT_DCL_SHARE_RAM_COUNT];  
  //数据区校验码,boot端用此检查Data是否有效
  unsigned long Adler32; 
};

//单例化声明，定义由应用层实现： 并于开发环境相关，建议放在堆栈最后：
//app端时：禁止优化，需初始化为全0
//boot端时：禁止优化，禁止实始化
extern struct _cBootDclShareRAM cBootDclShareRAM; 

/***********************************************************************
                              相关函数
***********************************************************************/

#include <string.h>
//---------------------------App端初始化-------------------------------
//此函数仅在APP端调用
#define cBootDclShareRAM_AppInit() do{\
  memset(&cBootDclShareRAM, 0, sizeof(struct _cBootDclShareRAM));}while(0)

//-----------------------App端Data改变后更新----------------------------
#ifdef C_BOOT_DCL_SHARE_RAM_SW_IDENT //用识别码交换数据
  #include "Adler32.h"    
  #define cBootDclShareRAM_AppDataChangedUpdate() do{\
    cBootDclShareRAM.Adler32 = C_BOOT_DCL_SHARE_RAM_SW_IDENT;}while(0)
#else   //Adler32值交换数据
  #define cBootDclShareRAM_AppDataChangedUpdate() do{cBootDclShareRAM.Adler32 = \
    Adler32_Get(1, cBootDclShareRAM.Data, C_BOOT_DCL_SHARE_RAM_COUNT);}while(0)
#endif

//----------------------boot端检查此区域是否有效------------------------
//用以检查是否为app端写入的数据
#ifdef C_BOOT_DCL_SHARE_RAM_SW_IDENT //用识别码交换数据
  #define cBootDclShareRAM_BootIsValid() \
    (cBootDclShareRAM.Adler32 == C_BOOT_DCL_SHARE_RAM_SW_IDENT)
#else   //Adler32值交换数据
  #define cBootDclShareRAM_BootIsValid()  (cBootDclShareRAM.Adler32 == \
    Adler32_Get(1, cBootDclShareRAM.Data, C_BOOT_DCL_SHARE_RAM_COUNT))
#endif

#endif

