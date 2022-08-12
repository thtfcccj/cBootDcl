/***********************************************************************

		   cBootDcl主模块-在写Boot且boot区为内部FLASH时的实现

此回调支持多个不连续，不同大小的Flash扇区组成的boot
***********************************************************************/
#include "cBootDcl.h"
#ifdef SUPPORT_APP_WR_BOOT  //支持APP端写BOOT时

#include "cBootDcl_BootInFlash.h" 
#include "math_3.h"
#include "Adler32.h" //校验码
#include "Flash.h"   //Flash区操作

#ifndef WDT_Week
  #include "IoCtrl.h" 
#endif

//----------------------------------默认定义-----------------------------
//为BOOT区分配的ID号(需>=128)
#ifndef BOOT_ID 
  #define  BOOT_ID  0xaa
#endif

//BOOT区幻数
#ifndef BOOT_MAGIC 
  #define  BOOT_MAGIC  0x1234
#endif

//Boot扇区总数
#ifndef BOOT_SECTOR_COUNT
  #define BOOT_SECTOR_COUNT   1
#endif

//Boot区完成标志位置, 此区域24个数据应填充为0xff,建议用汇编静态表实现
#ifndef BOOT_FINAL_BASE
  #define BOOT_FINAL_BASE 0x00000100
#endif

//Boot各分区基址查找表(以此支持不连续空间)
#ifndef BOOT_SECTOR_BASE_LUT
  #define BOOT_SECTOR_BASE_LUT {0x00000000}//默认1个扇区
#endif

//Boot各分区容量查找表(以此支持不同大小的FLASH空间)
#ifndef BOOT_SECTOR_CAPABILITI_LUT
  #define BOOT_SECTOR_CAPABILITI_LUT {FLASH_PAGE_SIZE}//默认1个扇区
#endif

//Boot分包情况查找表，下标0为BOOT扇区个数，其它下标依次对应分区包的起始位置
#ifndef BOOT_SECTOR_TO_PACKET_LUT
  #define BOOT_SECTOR_TO_PACKET_LUT {0, FLASH_PAGE_SIZE / 256}//默认1个扇区
#endif

/***********************************************************************
                          内部定义与声明
***********************************************************************/
//Boot各分区基址查找表定义
static const unsigned long _SectorBaseLut[BOOT_SECTOR_COUNT] = 
  BOOT_SECTOR_BASE_LUT; 
//Boot各分区容量查找表
static const unsigned long _SectorCapabilityLut[BOOT_SECTOR_COUNT] = 
  BOOT_SECTOR_CAPABILITI_LUT; 
//Boot分包情况查找表定义
static const unsigned short  _SectorToPacktLut[BOOT_SECTOR_COUNT + 1] = 
  BOOT_SECTOR_TO_PACKET_LUT;

//-----------------------------数据解密函数-----------------------------
//数据流格式需符合通讯协议：
//  pBuf[0]类型ID, pBuf[1~2]幻数,pBuf[4-5]数据帧顺序:
//返回0解码正确，负异常
extern signed char cBootDclCrypto_Decrypt(unsigned long Key,//密钥,即UID
                                   unsigned char *pBuf,//数据流
                                   unsigned char Start,//加密数据起始
                                   unsigned short Count);//加密数据长度

/***********************************************************************
                          回调函数-应用层相关
***********************************************************************/

//----------------------由类型ID与其幻数是否匹配------------------------
//返回：匹配时：     C_BOOT_DCL_ERR_NO
//     不匹配时：    C_BOOT_DCL_ERR_ID_MATCH
//     类型ID冲突时：C_BOOT_DCL_ERR_ID_COMFLICT
signed char cBootDcl_cbIdIsMatch(unsigned char Id,
                                  unsigned short MagicNo)
{
  if(Id != BOOT_ID) return C_BOOT_DCL_ERR_ID_COMFLICT;//类型ID冲突
  if(MagicNo != BOOT_MAGIC) return C_BOOT_DCL_ERR_ID_MATCH;//不匹配
  return 0;
}

//--------------------------是否允许进入状态---------------------
//电池供电时，建议在主电与电量充足情况下允许。
//非加密链接时，需在用户手工同意，或Key匹配时允许
//若同意链接，可在此函数内，采取如关闭必要功能等措施。
//返回：允许进入时 C_BOOT_DCL_ERR_NO
//      Key错误时：C_BOOT_DCL_ERR_KEY
//      系统忙时： C_BOOT_DCL_ERR_BUSY
//      电源问题时:C_BOOT_DCL_ERR_POW
signed char cBootDcl_cbIsEnEnter(signed char IsEncrypted,//加密链接
                                 unsigned long Key) //0时询问，否则为Key
{
  signed char Resume = cBootDcl_BootInFlash_cbIsEnEnter();
  if(Resume) return Resume;//当前系统状态不允许进入
  if(IsEncrypted){//加密链接时，比较固定密码
    if(Key != 0x55aaaa55) return C_BOOT_DCL_ERR_KEY;
  }
  //普通连接密码不对，不允许进入
  else if(Key != cBootDcl_BootInFlash_cbGetUID()) 
    return C_BOOT_DCL_ERR_KEY;
  //正确了
  return C_BOOT_DCL_ERR_NO;
}

//---------------------------是否允许退出写状态--------------------------
//返回0成功(应该重启了)，非0错误码
signed char cBootDcl_cbIsEnQuit(unsigned long Key)
{
  if((Key == 0x55aaaa55) ||  (Key == cBootDcl_BootInFlash_cbGetUID())){
    cBootDcl_BootInFlash_cbEnterBoot(0); //这里重启,并进入BOOT状态    
  }
  return C_BOOT_DCL_ERR_NO;  //仅退出模式
}

//----------------------------是否写入Flash准备-------------------------
//数据帧起始在cBootDcl.FrameNo里， 数据区长度在cBootDcl.WrCountD4里
//若为加密数据，应负责解密
//返回：准备好时 返回0
//      准备好但需擦除后再写入时 返回1
//      其它异常时：返回值从-20开始
signed char cBootDcl_cbIsWrFlashRdy(signed char IsEncrypted,//是否为加密数据
                                     unsigned char *pData) //从类型ID起数据
{
  if(cBootDcl.FrameNo >= _SectorToPacktLut[BOOT_SECTOR_COUNT])//包ID超限
      return C_BOOT_DCL_ERR_PACKET_ID;

  //加密数据时，先解密
  unsigned short Len = cBootDcl.WrCountD4 * 4;
  if(IsEncrypted){
    signed char Resume = cBootDclCrypto_Decrypt(cBootDcl_BootInFlash_cbGetUID(),
                                                pData, 6, Len);
    if(Resume) return Resume; //解密错误                      
  }
  //检查数据区校验码正确性
  pData += 6;//指向数据区了
  unsigned long Adler32 = Adler32_Get(C_BOOT_ADLER32_INIT, pData, Len); 
  if(Adler32 != MsbFull2L(pData + Len)) 
    return C_BOOT_DCL_ERR_CHECK; //数据校验错误 
  
  //数据正确了
  if(cBootDcl.FrameNo == 0){//BOOT区编程时，需一次性提前擦除完成。
    Flash_Unlock();
    #if BOOT_SECTOR_COUNT > 1 //多扇区时
      for(unsigned char Pos = 0; Pos < BOOT_SECTOR_COUNT; Pos++){
        Flash_ErasePage(_SectorBaseLut[Pos]);
      }
    #else //单扇区
      Flash_ErasePage(_SectorBaseLut[0]);
    #endif
    Flash_Lock();
  }
  return 0;//准备好写入了
}

/***********************************************************************
                          回调函数-读写Flash相关
***********************************************************************/

//-----------------------由FrameNo得基址----------------------
static unsigned long _GetBase(unsigned short FrameNo)
{
  #if BOOT_SECTOR_COUNT > 1 //多扇区时,需由cBootDcl.FrameNo查找写入起始
    unsigned char Pos = 0;//由低到高找，直接在该范围
    for( ; Pos < BOOT_SECTOR_COUNT; Pos++){
      if(FrameNo >= _SectorToPacktLut[Pos]) break;//找到了
    }
    return _SectorBaseLut[Pos] +  (FrameNo - _SectorToPacktLut[Pos]) * 256;
  #else //单扇区时
    return _SectorBaseLut[0] + FrameNo * 256;
  #endif  
}

//------------------------------写Flash操作---------------------------
//用回调可实现对如NANDFLASH的擦写。
//cBootDcl.Id 标识的区域 cBootDcl.FrameNo扇区起始,cBootDcl.WrCountD4写入总数
//返回0正确，非0错误
signed char cBootDcl_cbFlashWr(signed char State, //0直接写,1擦除后写,2写完成标志
                                unsigned char *pData)//有效数据
{
  if(State == 2){//写完成标志
    pData -= 4; //使用此缓冲区
    MsbL2Full(_SectorBaseLut[0], pData); //写入boot区基址
    Flash_Unlock();
    Flash_Write(BOOT_FINAL_BASE, pData, 24);//固定24个数据
    Flash_Lock();
    return 0;//成功
  }
  //BOOT区编程时，cBootDcl_cbIsWrFlashRdy()一次性提前擦除完成了，故不检查擦除标志
  
  //写入数据
  Flash_Unlock();  
  Flash_Write(_GetBase(cBootDcl.FrameNo), pData, cBootDcl.WrCountD4 * 4);
  Flash_Lock();
  return 0;//成功
}

//--------------------------Flash区数据校验---------------------------
//返回0正常，否则正校验不对，负：错误位置
signed char cBootDcl_cbFlashCheck(unsigned char State,//0编号,1扇区，2所有
                                   unsigned short Pos,//0，1时对应位置
                                   unsigned long  CheckCode)//通讯送入的校验码
{
  //全部BOOT区校验
  if(State == 2){
    unsigned long Adler32 = C_BOOT_ADLER32_INIT;
    for(unsigned char Pos = 0 ; Pos < BOOT_SECTOR_COUNT; Pos++){
      WDT_Week();//假定扇区校验时间不超看门狗时间
      Adler32 = Adler32_Get(Adler32, 
                            (const unsigned char *)_SectorBaseLut[Pos], 
                            _SectorCapabilityLut[Pos]);
    }
    if(Adler32 == CheckCode) return 0;
    return 1;//校验错误
  }
  
  //扇区或编号区校验
  unsigned long Base;
  unsigned long Len;
  if(State == 0){//编号
    if(Pos >= _SectorToPacktLut[BOOT_SECTOR_COUNT])//包ID超限
      return C_BOOT_DCL_ERR_PACKET_ID;
    Len = 256;
    Base = _GetBase(Pos);
  }
  else if(State == 1){//扇区
    if(Pos >= BOOT_SECTOR_COUNT)//FLASH页超限
      return C_BOOT_DCL_ERR_PAGE_ID;
    Len = _SectorCapabilityLut[Pos];
    Base = _SectorBaseLut[Pos];    
  }
  else return C_BOOT_DCL_ERR_FUN_CODE;//异常
  
  WDT_Week();//假定扇区校验时间不超看门狗时间
  if(Adler32_Get(C_BOOT_ADLER32_INIT, 
                 (const unsigned char *)Base, Len) == CheckCode)
    return 0;
  return 1;//校验错误
}


#endif //#ifdef SUPPORT_APP_WR_BOOT  //支持APP端写BOOT时







