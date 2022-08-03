/***********************************************************************

		   cBootDcl主模块-在写Boot且boot区为MCU内部FLASH时的实现

***********************************************************************/
#ifndef __C_BOOT_DCL_BOOT_IN_FLASH_H
#define __C_BOOT_DCL_BOOT_IN_FLASH_H

#include "cBootDcl.h"
#ifdef SUPPORT_APP_WR_BOOT  //支持APP端写BOOT时

/**********************************************************************
                           回调函数
***********************************************************************/

//-----------------------------得到UID--------------------------------
//此ID应在设备上能够显示用能够访问到，如:MCU内部的ID号等
unsigned short cBootDcl_BootInFlash_cbGetUID(void);

//--------------------------是否允许进入写boot状态---------------------
//电池供电时，建议在主电与电量充足情况下允许。
//返回：允许进入时 C_BOOT_DCL_ERR_NO
//      系统忙时： C_BOOT_DCL_ERR_BUSY
//      电源问题时:C_BOOT_DCL_ERR_POW
signed char cBootDcl_BootInFlash_cbIsEnEnter(void);

//------------------------重启并进入Boot工作状态---------------------
//应注意保存系统相关参数后，重启至BOOT状态
void cBootDcl_BootInFlash_cbEnterBoot(void);


#endif //#ifdef SUPPORT_APP_WR_BOOT  //支持APP端写BOOT时
#endif






