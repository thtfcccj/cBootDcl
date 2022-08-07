/***********************************************************************

		            cBootDcl主模块

此模块可编在BOOT端(默认)或APP端(需定义SUPPORT_APP_WR_BOOT)
***********************************************************************/
#ifndef _C_BOOT_DCL_H
#define	_C_BOOT_DCL_H

#ifdef SUPPORT_EX_PREINCLUDE//不支持Preinlude時
  #include "Preinclude.h"
#endif

//此软件模块使用MIT协议， 请在您在你设备适当位置放置此字符串的显示
#define C_BOOT_DCL_VERSION    "cBootDcl V1.00"  
//boot区带显示提示
#define C_BOOT_DCL_LOADER_VERSION    "cBootDcl Loader V1.00"  

/***********************************************************************
                          工作流程说明
***********************************************************************/
//1. 建立对特定区域(类型ID)写入的通讯连接：
//   电脑端:向设备发送 "1.1建立普通(或1.2加密)连接"。
//   设备端: 收到并检验为合法联接后，进入更新boot状态(可显示提示)并倒计时
//2. 为BOOT区时，写入BOOT关键程序
//   BOOT区关键程序用以保证写入此程序后，即使没有后继数据，也可正在进入APP
//   电脑端: 视情况采用“3.1小数据分段写入协议”进行一次性分段操作,若在256
//           字节内，能够保证正确进入APP,则可直接采用“3.2大数据多帧写入协议”
//   设备端：收到此数据(并解密)校验成功后，擦除全部关键BOOT区，并立即写入FLASH
//   完成后将写入的FLASH数据校验后(若有BOOT有多个扇区则在此擦除后续)返回电脑。
//3. 写入FLASH数据
//   电脑端：向设备一直发送“3.2大数据多帧写入协议”，直到全部写入完成
//   设备端：收到此数据(并解密)校验成功后，立即写入FLASH
//4. 写入程序完备描述
//   电脑端：向设备发送“3.5 写入程序完备描述协议”。
//   设备端：收到此数据后,计算该区域校验码，若匹配则写入完备描述返写入完成！
//5. 断开并重启设备
//   电脑端：向设备发送“1.3断开连接并重启设备”。
//   设备端：收到此数据后,将完备信息写入BOOT区，并立即重启设备,通讯结束。
//   注：程离在BOOT区时，若需重新写入下个特定区域(类型ID)，则需重新建立连接

//注：
//1. 若设备端检测到超时，则将在超时后重启设备。
//2. 只有在未建立BOOT区通讯连接时，发送“建立更新普通数据区连接”才可进入boot
//3. boot区写app或资源文件流程，与上述相同。但换成了普通数据。

/***********************************************************************
                         相关配置与定义
***********************************************************************/

//APP端使用此模块时, 需全局定义
//#define SUPPORT_APP_WR_BOOT  //支持APP端写BOOT时，否则为BOOT程序写APP

//支持此模块工作状态提示时，需全局定义
//#define SUPPORT_C_BOOT_DCL_STATE_NOTE

//连接关闭时间，超过此时间无进一步通信过来，则退出
#ifndef C_BOOT_DCL_DOING_OV
  #define C_BOOT_DCL_DOING_OV      (240)
#endif

//返回错误码定义(不可更改)
#define C_BOOT_DCL_ERR_NO            0 //正确无故障
#define C_BOOT_DCL_ERR_ID_MATCH     -1 //ID号与幻数不匹配
#define C_BOOT_DCL_ERR_ID_COMFLICT  -2 //ID号与程序冲突(boot写boot,app写非boot区了)
#define C_BOOT_DCL_ERR_KEY          -3 //密码错误
#define C_BOOT_DCL_ERR_BUSY         -4 //系统忙
#define C_BOOT_DCL_ERR_POW          -5 //电源问题(如电量低)
#define C_BOOT_DCL_ERR_CONNECT      -6 //未建立连接
#define C_BOOT_DCL_ERR_ID_OTHER     -7 //为其它类型ID了
#define C_BOOT_DCL_ERR_LEN          -8 //数据过短或不匹配
#define C_BOOT_DCL_ERR_CH_MATCH     -9 //加密通道不匹配
#define C_BOOT_DCL_ERR_FLASH_WR     -10//Flash擦除或写入错误
#define C_BOOT_DCL_ERR_FUN_CODE     -11//无此功能码

//-20起：
#define C_BOOT_DCL_ERR_DATA_START   -20//数据起始错误
#define C_BOOT_DCL_ERR_DECRYPT      -21//解密过程错误
#define C_BOOT_DCL_ERR_CHECK        -22//数据校验错误
#define C_BOOT_DCL_ERR_PACKET_ID    -23//包ID超限
#define C_BOOT_DCL_ERR_PAGE_ID      -24//FLASH页超限

/***********************************************************************
                         相关结构
***********************************************************************/

struct _cBootDcl{
  unsigned char Timer;      //倒计时器,非0表示写Boot模式
  unsigned char IsEncrypted;  //是否为加密链接
  unsigned char Id;        //本次建立链接的ID号
  unsigned char WrCountD4; //最后一次写入数据个数/4值
  unsigned short FrameNo;  //最后一次写入的数据帧编号
};

extern struct _cBootDcl cBootDcl;

/***********************************************************************
                              相关函数
***********************************************************************/

//--------------------------------初始化-------------------------------
void cBootDcl_Init(void);

//--------------------------------任务函数------------------------------
//放入1s进程,若不需要自动退出功能(如boot由外部管理)，则可不调用此函数
void cBootDcl_Task(void);

//------------------------------是否在工作状态--------------------------
#define cBootDcl_IsDoing()  (cBootDcl.Timer)

//-----------------------------接收数据处理----------------------------
//返回发送数据个数，0或负不返回
signed short cBootDcl_RcvPro(unsigned char *pData,//从子功能码起
                              unsigned short Len);//数据长度

/***********************************************************************
                          回调函数-应用层相关
***********************************************************************/

//----------------------由类型ID与其幻数是否匹配------------------------
//返回：匹配时：     C_BOOT_DCL_ERR_NO
//     不匹配时：    C_BOOT_DCL_ERR_ID_MATCH
//     类型ID冲突时：C_BOOT_DCL_ERR_ID_COMFLICT
signed char cBootDcl_cbIdIsMatch(unsigned char Id,
                                  unsigned short MagicNo);

//--------------------------是否允许进入写状态-------------------------
//电池供电时，建议在主电与电量充足情况下允许。
//非加密链接时，需在用户手工同意，或Key匹配时允许
//若同意链接，可在此函数内，采取如关闭必要功能等措施。
//返回：允许进入时 C_BOOT_DCL_ERR_NO
//      Key错误时：C_BOOT_DCL_ERR_KEY
//      系统忙时： C_BOOT_DCL_ERR_BUSY
//      电源问题时:C_BOOT_DCL_ERR_POW
signed char cBootDcl_cbIsEnEnter(signed char IsEncrypted,//加密链接
                                 unsigned long Key); //0时询问，否则为Key

//---------------------------是否允许退出写状态--------------------------
//返回0成功(应该重启了)，非0错误码
signed char cBootDcl_cbIsEnQuit(unsigned long Key);

//----------------------------是否写入Flash准备-------------------------
//数据帧起始在cBootDcl.FrameNo里，数据区长度在cBootDcl.WrCountD4里
//若为加密数据，应负责解密
//返回：准备好时 返回0
//      准备好但需擦除后再写入时 返回1
//      其它异常时：返回值从-20开始
signed char cBootDcl_cbIsWrFlashRdy(signed char IsEncrypted,//是否为加密数据
                                     unsigned char *pData); //从类型ID起数据

/***********************************************************************
                          回调函数-读写Flash相关
***********************************************************************/

//------------------------------写Flash操作---------------------------
//用回调可实现对如NANDFLASH的擦写。
//cBootDcl.Id 标识的区域 cBootDcl.FrameNo扇区起始,cBootDcl.WrCountD4写入总数
//返回0正确，非0错误
signed char cBootDcl_cbFlashWr(signed char State, //0直接写,1擦除后写,2写完成标志
                                unsigned char *pData);//有效数据

//--------------------------Flash区数据校验---------------------------
//返回0正常，否则正校验不对，负：错误位置
signed char cBootDcl_cbFlashCheck(unsigned char State,//0编号,1扇区，2所有
                                   unsigned short Pos,//0，1时对应位置
                                   unsigned long  CheckCode);//通讯送入的校验码

/***********************************************************************
                          回调函数-状态通道相关
***********************************************************************/

//-------------------------------状态通报-----------------------------
//支持显示提示等可实现，状态码定义为: (对应值为负时，表示对应状态错误)
//0: 退出。
//1: 握手并进入 cBootDcl.Id 标识的区域, cBootDcl.IsCrypto标识是否为加密通道
//2：已擦除某个扇区提示，cBootDcl.FrameNo可获知扇区起始
//3. 已写入一组FLASH数据,cBootDcl.FrameNo扇区起始,cBootDcl.WrCountD4写入总数
//4. 已校验并结束。
#ifdef SUPPORT_C_BOOT_DCL_STATE_NOTE
  void cBootDcl_cbStateNotify(signed char State);//状态码
#else //不支持时
  #define cBootDcl_cbStateNotify(state) do{}while(0)
#endif

#endif

