/***********************************************************************

		            cBootDcl��ģ��ʵ��


***********************************************************************/

#include "cBootDcl.h"
#include "math_3.h"

struct _cBootDcl cBootDcl;

//------------------------BOOT��APPЭ���ڲ�����------------------
#ifdef SUPPORT_APP_WR_BOOT  //֧��APP��дBOOTʱ
  #define _HAND_NOR     0x12 //��ͨ��������
  #define _HAND_CRYPTO  0x21  //������������
#else //BOOTдAPPʱ
  #define _HAND_NOR     0x34 //��ͨ��������
  #define _HAND_CRYPTO  0x43  //������������
#endif

/***********************************************************************
                              ��غ���ʵ��
***********************************************************************/

//--------------------------------��ʼ��-------------------------------
void cBootDcl_Init(void)
{
  cBootDcl.Timer = 0;
}

//--------------------------------������------------------------------
//����1s����
void cBootDcl_Task(void)
{
  if(!cBootDcl.Timer) return;
  cBootDcl.Timer--;
  if(cBootDcl.Timer) return; //ʱ��δ��
  
  #ifndef SUPPORT_APP_WR_BOOT  //BOOT��ʱ�������Խ����û�����
    while(1);
  #endif
}

//----------------------------ͨѶ�����Ƿ���Ч------------------------------
static signed char _DataIsInvalid(unsigned char IsEncrypted)//�Ƿ�Ϊ��������
{
  if(IsEncrypted){
    if(!cBootDcl.IsEncrypted) return 1;//������������ͨͨ����
  }
  else if(cBootDcl.IsEncrypted) return -1;//��ͨ�����ڼ���ͨ����
  return 0;//������
}

//-----------------------------�������ݴ���----------------------------
//���ط������ݸ�����0�򸺲�����
signed short cBootDcl_RcvPro(unsigned char *pData,//���ӹ�������
                              unsigned short Len)//���ݳ���
{
  if(Len < 6) return 0;//����6������
  unsigned char FunCode = *pData++;
  if((0xff - FunCode) != *pData++) return 0; //�ӹ����벻��
  //��ʱpDataָ��������һλ��!
  
  //============================�������============================
  //�Ͽ����Ӳ������豸
  if(FunCode == 0x66){
    signed char Resume = cBootDcl_cbIsEnQuit(MsbFull2L(pData));
    cBootDcl.Timer = 0;//ֱ���˳�,���˳�����
    cBootDcl_cbStateNotify(0);//ͨ��
    *pData = Resume;
    return 2 + 1;
  }
  //���ID������Ƿ�ƥ����ͻ
  signed char Resume = cBootDcl_cbIdIsMatch(*pData, MsbFull2S(pData + 1)); 
  if(Resume){//ƥ����ͻ��
    *pData = (unsigned char)Resume; //���ؽ��
    return 2 + 1;
  }
  //��������ʱ
  if((FunCode == _HAND_NOR) || (FunCode == _HAND_CRYPTO)){
    unsigned char IsCrypto = FunCode - _HAND_NOR;//����ͨ����־
    cBootDcl_cbIsEnEnter(IsCrypto, MsbFull2L(pData + 3));
    if(Resume == 0){//ͬ�������
      cBootDcl.Timer = C_BOOT_DCL_DOING_OV;//����
      cBootDcl.IsEncrypted = IsCrypto;//��������
      cBootDcl.Id = *pData; //��סID
      cBootDcl_cbStateNotify(1);//ͨ��
    }
    //else ;//��ͬ�����ӣ�������״
    *pData = Resume; //���ؽ��
    return 2 + 1; 
  }
  //����ͨѶ���ѽ�������ʱ�����¼�ʱ   
  else if(cBootDcl.Timer) 
    cBootDcl.Timer = C_BOOT_DCL_DOING_OV;      
  else{//����ͨѶδ��������
    *pData = (unsigned char)C_BOOT_DCL_ERR_CONNECT; //δ��������
    return 2 + 1; 
  }
  
  //=========================дFlashЭ�����==========================
  if(cBootDcl.Id != *pData){//ID��ƥ��
    *pData = (unsigned char)C_BOOT_DCL_ERR_ID_OTHER; //��������ID��
    return 2 + 1;     
  }
  //�ֱ���
  switch(FunCode){  
    case 0x5a:  //3.2�����ݶ�֡д��Э��,��ͨ����
    case 0x5c:{ //3.2�����ݶ�֡д��Э��,��������
      cBootDcl.WrCountD4 = *(pData + 3);      
      if(Len != (12 + cBootDcl.WrCountD4 * 4)){//���ݹ��̻�ƥ��
        *pData = (unsigned char)C_BOOT_DCL_ERR_LEN;
        return 2 + 1;
      }
      unsigned char IsCrypto = FunCode - _HAND_NOR;//����ͨ����־
      if(_DataIsInvalid(IsCrypto)){
        *pData = (unsigned char)C_BOOT_DCL_ERR_CH_MATCH; //����ͨ����ƥ��
        return 2 + 1;
      }
      cBootDcl.FrameNo = MsbFull2S(pData + 4);
      signed char Resume = cBootDcl_cbIsWrFlashRdy(IsCrypto, pData);
      if(Resume < 0){//������
        *pData = (unsigned char)Resume; //���ش�����
        return 2 + 1; 
      }
      if(cBootDcl_cbFlashWr(Resume, pData + 6) == 0){//д����ȷ��
        if(Resume) cBootDcl_cbStateNotify(2);//ͨ��������
        cBootDcl_cbStateNotify(3); //д�����
      }
      else{//д�������
        *pData = (unsigned char)C_BOOT_DCL_ERR_FLASH_WR; //���ش�����
        return 2 + 1;         
      }
      break;
    }
    case 0x55: //3.3 FLASH����У��Э��
    case 0x5f:{ //3.4 FLASH�����ڲ����ݿ�У��Э��      
      unsigned char Count = *(pData + 3); 
      if(Len != (12 + Count * 4)){//���ݹ��̻�ƥ��
        *pData = (unsigned char)C_BOOT_DCL_ERR_LEN;
        return 2 + 1;
      }
      unsigned short Start = MsbFull2S(pData + 4);
      pData += 6; //Adler32λ����
      unsigned char State;
      if(FunCode == 0x55) State = 1;
      else State = 0;
      for(unsigned char Pos = 0; Pos < Count; Pos++, pData += 4){
        Resume = cBootDcl_cbFlashCheck(State, Start + Pos, MsbFull2L(pData));
        if(Resume){//��ƥ��ʱ
          if(Resume > 0) *pData = Pos; //�����ش���λ��
          else *pData = Resume;//���ش�����
          return 2 + 1;
        }//end if
      } //end for
      break;
    }
    case 0xaa:{//3.5 д������걸�������������豸
      if(Len != (5 + 20)){//���ݹ��̻�ƥ��
        *pData = (unsigned char)C_BOOT_DCL_ERR_LEN;
        return 2 + 1;        
      }
    }
    //����У��
    if(cBootDcl_cbFlashCheck(2, 0, MsbFull2L(pData + 5))){
      *pData = 1; //��ƥ����
      return 2 + 1;      
    }
    //ƥ���ˣ�д����������ļ���
    cBootDcl_cbFlashWr(2, pData + 9);
    break;
    default: //����ʶ��Ĺ�����
      *pData = (unsigned char)C_BOOT_DCL_ERR_FUN_CODE; 
      return 2 + 1;    
  }
  //������ȷʱ����
  *pData = C_BOOT_DCL_ERR_NO; 
  return 2 + 1; 
}







