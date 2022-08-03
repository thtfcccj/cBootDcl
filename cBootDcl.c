/***********************************************************************

		            cBootDcl主模块实现


***********************************************************************/

#include "cBootDcl.h"
#include "math_3.h"

struct _cBootDcl cBootDcl;

//------------------------BOOT与APP协议内部定义------------------
#ifdef SUPPORT_APP_WR_BOOT  //支持APP端写BOOT时
  #define _HAND_NOR     0x12 //普通连接握手
  #define _HAND_CRYPTO  0x21  //加密连接握手
#else //BOOT写APP时
  #define _HAND_NOR     0x34 //普通连接握手
  #define _HAND_CRYPTO  0x43  //加密连接握手
#endif

/***********************************************************************
                              相关函数实现
***********************************************************************/

//--------------------------------初始化-------------------------------
void cBootDcl_Init(void)
{
  cBootDcl.Timer = 0;
}

//--------------------------------任务函数------------------------------
//放入1s进程
void cBootDcl_Task(void)
{
  if(!cBootDcl.Timer) return;
  cBootDcl.Timer--;
  if(cBootDcl.Timer) return; //时间未到
  
  #ifndef SUPPORT_APP_WR_BOOT  //BOOT端时，重启以进入用户程序
    while(1);
  #endif
}

//----------------------------通讯数据是否无效------------------------------
static signed char _DataIsInvalid(unsigned char IsEncrypted)//是否为加密数据
{
  if(IsEncrypted){
    if(!cBootDcl.IsEncrypted) return 1;//加密数据在普通通道了
  }
  else if(cBootDcl.IsEncrypted) return -1;//普通数据在加密通道了
  return 0;//符合了
}

//-----------------------------接收数据处理----------------------------
//返回发送数据个数，0或负不返回
signed short cBootDcl_RcvPro(unsigned char *pData,//从子功能码起
                              unsigned short Len)//数据长度
{
  if(Len < 6) return 0;//至少6个数据
  unsigned char FunCode = *pData++;
  if((0xff - FunCode) != *pData++) return 0; //子功能码不对
  //此时pData指向功能码下一位了!
  
  //============================管理相关============================
  //断开连接并重启设备
  if(FunCode == 0x66){
    signed char Resume = cBootDcl_cbIsEnQuit(MsbFull2L(pData));
    cBootDcl.Timer = 0;//直接退出,并退出连接
    cBootDcl_cbStateNotify(0);//通报
    *pData = Resume;
    return 2 + 1;
  }
  //检查ID与幻数是否匹配或冲突
  signed char Resume = cBootDcl_cbIdIsMatch(*pData, MsbFull2S(pData + 1)); 
  if(Resume){//匹配或冲突了
    *pData = (unsigned char)Resume; //返回结果
    return 2 + 1;
  }
  //建立连接时
  if((FunCode == _HAND_NOR) || (FunCode == _HAND_CRYPTO)){
    unsigned char IsCrypto = FunCode - _HAND_NOR;//加密通道标志
    cBootDcl_cbIsEnEnter(IsCrypto, MsbFull2L(pData + 3));
    if(Resume == 0){//同意进入了
      cBootDcl.Timer = C_BOOT_DCL_DOING_OV;//进入
      cBootDcl.IsEncrypted = IsCrypto;//加密连接
      cBootDcl.Id = *pData; //记住ID
      cBootDcl_cbStateNotify(1);//通报
    }
    //else ;//不同意连接：保持现状
    *pData = Resume; //返回结果
    return 2 + 1; 
  }
  //其它通讯在已建立链接时，重新计时   
  else if(cBootDcl.Timer) 
    cBootDcl.Timer = C_BOOT_DCL_DOING_OV;      
  else{//其它通讯未建立链接
    *pData = (unsigned char)C_BOOT_DCL_ERR_CONNECT; //未建立连接
    return 2 + 1; 
  }
  
  //=========================写Flash协议相关==========================
  if(cBootDcl.Id != *pData){//ID不匹配
    *pData = (unsigned char)C_BOOT_DCL_ERR_ID_OTHER; //其它类型ID了
    return 2 + 1;     
  }
  //分别处理
  switch(FunCode){  
    case 0x5a:  //3.2大数据多帧写入协议,普通数据
    case 0x5c:{ //3.2大数据多帧写入协议,加密数据
      cBootDcl.WrCountD4 = *(pData + 3);      
      if(Len != (12 + cBootDcl.WrCountD4 * 4)){//数据过短或不匹配
        *pData = (unsigned char)C_BOOT_DCL_ERR_LEN;
        return 2 + 1;
      }
      unsigned char IsCrypto = FunCode - _HAND_NOR;//加密通道标志
      if(_DataIsInvalid(IsCrypto)){
        *pData = (unsigned char)C_BOOT_DCL_ERR_CH_MATCH; //加密通道不匹配
        return 2 + 1;
      }
      cBootDcl.FrameNo = MsbFull2S(pData + 4);
      signed char Resume = cBootDcl_cbIsWrFlashRdy(IsCrypto, pData);
      if(Resume < 0){//错误了
        *pData = (unsigned char)Resume; //返回错误码
        return 2 + 1; 
      }
      if(cBootDcl_cbFlashWr(Resume, pData + 6) == 0){//写入正确了
        if(Resume) cBootDcl_cbStateNotify(2);//通报擦除了
        cBootDcl_cbStateNotify(3); //写入完成
      }
      else{//写入错误了
        *pData = (unsigned char)C_BOOT_DCL_ERR_FLASH_WR; //返回错误码
        return 2 + 1;         
      }
      break;
    }
    case 0x55: //3.3 FLASH扇区校验协议
    case 0x5f:{ //3.4 FLASH扇区内部数据块校验协议      
      unsigned char Count = *(pData + 3); 
      if(Len != (12 + Count * 4)){//数据过短或不匹配
        *pData = (unsigned char)C_BOOT_DCL_ERR_LEN;
        return 2 + 1;
      }
      unsigned short Start = MsbFull2S(pData + 4);
      pData += 6; //Adler32位置了
      unsigned char State;
      if(FunCode == 0x55) State = 1;
      else State = 0;
      for(unsigned char Pos = 0; Pos < Count; Pos++, pData += 4){
        Resume = cBootDcl_cbFlashCheck(State, Start + Pos, MsbFull2L(pData));
        if(Resume){//不匹配时
          if(Resume > 0) *pData = Pos; //正返回错误位置
          else *pData = Resume;//返回错误码
          return 2 + 1;
        }//end if
      } //end for
      break;
    }
    case 0xaa:{//3.5 写入程序完备描述，并重启设备
      if(Len != (5 + 20)){//数据过短或不匹配
        *pData = (unsigned char)C_BOOT_DCL_ERR_LEN;
        return 2 + 1;        
      }
    }
    //扇区校验
    if(cBootDcl_cbFlashCheck(2, 0, MsbFull2L(pData + 5))){
      *pData = 1; //不匹配了
      return 2 + 1;      
    }
    //匹配了，写入结束符及文件名
    cBootDcl_cbFlashWr(2, pData + 9);
    break;
    default: //不能识别的功能码
      *pData = (unsigned char)C_BOOT_DCL_ERR_FUN_CODE; 
      return 2 + 1;    
  }
  //数据正确时返回
  *pData = C_BOOT_DCL_ERR_NO; 
  return 2 + 1; 
}







