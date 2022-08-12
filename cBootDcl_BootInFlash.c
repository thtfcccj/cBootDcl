/***********************************************************************

		   cBootDcl��ģ��-��дBoot��boot��Ϊ�ڲ�FLASHʱ��ʵ��

�˻ص�֧�ֶ������������ͬ��С��Flash������ɵ�boot
***********************************************************************/
#include "cBootDcl.h"
#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ

#include "cBootDcl_BootInFlash.h" 
#include "math_3.h"
#include "Adler32.h" //У����
#include "Flash.h"   //Flash������

#ifndef WDT_Week
  #include "IoCtrl.h" 
#endif

//----------------------------------Ĭ�϶���-----------------------------
//ΪBOOT�������ID��(��>=128)
#ifndef BOOT_ID 
  #define  BOOT_ID  0xaa
#endif

//BOOT������
#ifndef BOOT_MAGIC 
  #define  BOOT_MAGIC  0x1234
#endif

//Boot��������
#ifndef BOOT_SECTOR_COUNT
  #define BOOT_SECTOR_COUNT   1
#endif

//Boot����ɱ�־λ��, ������24������Ӧ���Ϊ0xff,�����û�ྲ̬��ʵ��
#ifndef BOOT_FINAL_BASE
  #define BOOT_FINAL_BASE 0x00000100
#endif

//Boot��������ַ���ұ�(�Դ�֧�ֲ������ռ�)
#ifndef BOOT_SECTOR_BASE_LUT
  #define BOOT_SECTOR_BASE_LUT {0x00000000}//Ĭ��1������
#endif

//Boot�������������ұ�(�Դ�֧�ֲ�ͬ��С��FLASH�ռ�)
#ifndef BOOT_SECTOR_CAPABILITI_LUT
  #define BOOT_SECTOR_CAPABILITI_LUT {FLASH_PAGE_SIZE}//Ĭ��1������
#endif

//Boot�ְ�������ұ��±�0ΪBOOT���������������±����ζ�Ӧ����������ʼλ��
#ifndef BOOT_SECTOR_TO_PACKET_LUT
  #define BOOT_SECTOR_TO_PACKET_LUT {0, FLASH_PAGE_SIZE / 256}//Ĭ��1������
#endif

/***********************************************************************
                          �ڲ�����������
***********************************************************************/
//Boot��������ַ���ұ���
static const unsigned long _SectorBaseLut[BOOT_SECTOR_COUNT] = 
  BOOT_SECTOR_BASE_LUT; 
//Boot�������������ұ�
static const unsigned long _SectorCapabilityLut[BOOT_SECTOR_COUNT] = 
  BOOT_SECTOR_CAPABILITI_LUT; 
//Boot�ְ�������ұ���
static const unsigned short  _SectorToPacktLut[BOOT_SECTOR_COUNT + 1] = 
  BOOT_SECTOR_TO_PACKET_LUT;

//-----------------------------���ݽ��ܺ���-----------------------------
//��������ʽ�����ͨѶЭ�飺
//  pBuf[0]����ID, pBuf[1~2]����,pBuf[4-5]����֡˳��:
//����0������ȷ�����쳣
extern signed char cBootDclCrypto_Decrypt(unsigned long Key,//��Կ,��UID
                                   unsigned char *pBuf,//������
                                   unsigned char Start,//����������ʼ
                                   unsigned short Count);//�������ݳ���

/***********************************************************************
                          �ص�����-Ӧ�ò����
***********************************************************************/

//----------------------������ID��������Ƿ�ƥ��------------------------
//���أ�ƥ��ʱ��     C_BOOT_DCL_ERR_NO
//     ��ƥ��ʱ��    C_BOOT_DCL_ERR_ID_MATCH
//     ����ID��ͻʱ��C_BOOT_DCL_ERR_ID_COMFLICT
signed char cBootDcl_cbIdIsMatch(unsigned char Id,
                                  unsigned short MagicNo)
{
  if(Id != BOOT_ID) return C_BOOT_DCL_ERR_ID_COMFLICT;//����ID��ͻ
  if(MagicNo != BOOT_MAGIC) return C_BOOT_DCL_ERR_ID_MATCH;//��ƥ��
  return 0;
}

//--------------------------�Ƿ��������״̬---------------------
//��ع���ʱ������������������������������
//�Ǽ�������ʱ�������û��ֹ�ͬ�⣬��Keyƥ��ʱ����
//��ͬ�����ӣ����ڴ˺����ڣ���ȡ��رձ�Ҫ���ܵȴ�ʩ��
//���أ��������ʱ C_BOOT_DCL_ERR_NO
//      Key����ʱ��C_BOOT_DCL_ERR_KEY
//      ϵͳæʱ�� C_BOOT_DCL_ERR_BUSY
//      ��Դ����ʱ:C_BOOT_DCL_ERR_POW
signed char cBootDcl_cbIsEnEnter(signed char IsEncrypted,//��������
                                 unsigned long Key) //0ʱѯ�ʣ�����ΪKey
{
  signed char Resume = cBootDcl_BootInFlash_cbIsEnEnter();
  if(Resume) return Resume;//��ǰϵͳ״̬���������
  if(IsEncrypted){//��������ʱ���ȽϹ̶�����
    if(Key != 0x55aaaa55) return C_BOOT_DCL_ERR_KEY;
  }
  //��ͨ�������벻�ԣ����������
  else if(Key != cBootDcl_BootInFlash_cbGetUID()) 
    return C_BOOT_DCL_ERR_KEY;
  //��ȷ��
  return C_BOOT_DCL_ERR_NO;
}

//---------------------------�Ƿ������˳�д״̬--------------------------
//����0�ɹ�(Ӧ��������)����0������
signed char cBootDcl_cbIsEnQuit(unsigned long Key)
{
  if((Key == 0x55aaaa55) ||  (Key == cBootDcl_BootInFlash_cbGetUID())){
    cBootDcl_BootInFlash_cbEnterBoot(0); //��������,������BOOT״̬    
  }
  return C_BOOT_DCL_ERR_NO;  //���˳�ģʽ
}

//----------------------------�Ƿ�д��Flash׼��-------------------------
//����֡��ʼ��cBootDcl.FrameNo� ������������cBootDcl.WrCountD4��
//��Ϊ�������ݣ�Ӧ�������
//���أ�׼����ʱ ����0
//      ׼���õ����������д��ʱ ����1
//      �����쳣ʱ������ֵ��-20��ʼ
signed char cBootDcl_cbIsWrFlashRdy(signed char IsEncrypted,//�Ƿ�Ϊ��������
                                     unsigned char *pData) //������ID������
{
  if(cBootDcl.FrameNo >= _SectorToPacktLut[BOOT_SECTOR_COUNT])//��ID����
      return C_BOOT_DCL_ERR_PACKET_ID;

  //��������ʱ���Ƚ���
  unsigned short Len = cBootDcl.WrCountD4 * 4;
  if(IsEncrypted){
    signed char Resume = cBootDclCrypto_Decrypt(cBootDcl_BootInFlash_cbGetUID(),
                                                pData, 6, Len);
    if(Resume) return Resume; //���ܴ���                      
  }
  //���������У������ȷ��
  pData += 6;//ָ����������
  unsigned long Adler32 = Adler32_Get(C_BOOT_ADLER32_INIT, pData, Len); 
  if(Adler32 != MsbFull2L(pData + Len)) 
    return C_BOOT_DCL_ERR_CHECK; //����У����� 
  
  //������ȷ��
  if(cBootDcl.FrameNo == 0){//BOOT�����ʱ����һ������ǰ������ɡ�
    Flash_Unlock();
    #if BOOT_SECTOR_COUNT > 1 //������ʱ
      for(unsigned char Pos = 0; Pos < BOOT_SECTOR_COUNT; Pos++){
        Flash_ErasePage(_SectorBaseLut[Pos]);
      }
    #else //������
      Flash_ErasePage(_SectorBaseLut[0]);
    #endif
    Flash_Lock();
  }
  return 0;//׼����д����
}

/***********************************************************************
                          �ص�����-��дFlash���
***********************************************************************/

//-----------------------��FrameNo�û�ַ----------------------
static unsigned long _GetBase(unsigned short FrameNo)
{
  #if BOOT_SECTOR_COUNT > 1 //������ʱ,����cBootDcl.FrameNo����д����ʼ
    unsigned char Pos = 0;//�ɵ͵����ң�ֱ���ڸ÷�Χ
    for( ; Pos < BOOT_SECTOR_COUNT; Pos++){
      if(FrameNo >= _SectorToPacktLut[Pos]) break;//�ҵ���
    }
    return _SectorBaseLut[Pos] +  (FrameNo - _SectorToPacktLut[Pos]) * 256;
  #else //������ʱ
    return _SectorBaseLut[0] + FrameNo * 256;
  #endif  
}

//------------------------------дFlash����---------------------------
//�ûص���ʵ�ֶ���NANDFLASH�Ĳ�д��
//cBootDcl.Id ��ʶ������ cBootDcl.FrameNo������ʼ,cBootDcl.WrCountD4д������
//����0��ȷ����0����
signed char cBootDcl_cbFlashWr(signed char State, //0ֱ��д,1������д,2д��ɱ�־
                                unsigned char *pData)//��Ч����
{
  if(State == 2){//д��ɱ�־
    pData -= 4; //ʹ�ô˻�����
    MsbL2Full(_SectorBaseLut[0], pData); //д��boot����ַ
    Flash_Unlock();
    Flash_Write(BOOT_FINAL_BASE, pData, 24);//�̶�24������
    Flash_Lock();
    return 0;//�ɹ�
  }
  //BOOT�����ʱ��cBootDcl_cbIsWrFlashRdy()һ������ǰ��������ˣ��ʲ���������־
  
  //д������
  Flash_Unlock();  
  Flash_Write(_GetBase(cBootDcl.FrameNo), pData, cBootDcl.WrCountD4 * 4);
  Flash_Lock();
  return 0;//�ɹ�
}

//--------------------------Flash������У��---------------------------
//����0������������У�鲻�ԣ���������λ��
signed char cBootDcl_cbFlashCheck(unsigned char State,//0���,1������2����
                                   unsigned short Pos,//0��1ʱ��Ӧλ��
                                   unsigned long  CheckCode)//ͨѶ�����У����
{
  //ȫ��BOOT��У��
  if(State == 2){
    unsigned long Adler32 = C_BOOT_ADLER32_INIT;
    for(unsigned char Pos = 0 ; Pos < BOOT_SECTOR_COUNT; Pos++){
      WDT_Week();//�ٶ�����У��ʱ�䲻�����Ź�ʱ��
      Adler32 = Adler32_Get(Adler32, 
                            (const unsigned char *)_SectorBaseLut[Pos], 
                            _SectorCapabilityLut[Pos]);
    }
    if(Adler32 == CheckCode) return 0;
    return 1;//У�����
  }
  
  //����������У��
  unsigned long Base;
  unsigned long Len;
  if(State == 0){//���
    if(Pos >= _SectorToPacktLut[BOOT_SECTOR_COUNT])//��ID����
      return C_BOOT_DCL_ERR_PACKET_ID;
    Len = 256;
    Base = _GetBase(Pos);
  }
  else if(State == 1){//����
    if(Pos >= BOOT_SECTOR_COUNT)//FLASHҳ����
      return C_BOOT_DCL_ERR_PAGE_ID;
    Len = _SectorCapabilityLut[Pos];
    Base = _SectorBaseLut[Pos];    
  }
  else return C_BOOT_DCL_ERR_FUN_CODE;//�쳣
  
  WDT_Week();//�ٶ�����У��ʱ�䲻�����Ź�ʱ��
  if(Adler32_Get(C_BOOT_ADLER32_INIT, 
                 (const unsigned char *)Base, Len) == CheckCode)
    return 0;
  return 1;//У�����
}


#endif //#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ







