### 总体说明:
  cBootDcl用以为小型嵌入式系统提供boot与App更新，以及Dcl(Dynamic callback Library 动态回调库)功能。
  分为boot引导程序与app支持端两部分
#### boot引导程序：
  设计为提供下述功能:
  * 常规BootLoader功能，暂仅实现通过USART下载应用程序，采用私有协议，再配合自设计的PC端软件进行。
  * Dcl(Dynamic callback Library 动态回调库)功能: 为应用程序提供可随时替换的回调函数
    如：对接相同功能(如消防中的CRT)，不同厂家通讯协议的数据编码与解码函数。
  * 单应用多资源更新功能：FLASH可能分为多个区域,如：app(唯一的应用程序)，EEPOM，顺序记录区，字库，图片资源等
    可通过电脑端下载更新定好的不同的Flash区域(电脑与boot区的“资源包描述”需匹配)。
#### app支持端：
  设计为提供下述功能:
  * 安全授权功能：防止黑客入仅，以及通讯异常时进入,可通过
    + 人工授权(如通过菜单)，或
    + 自动授权(如用户提供UID后通过远程通讯)方式进入此部分程序。
  * 反向更新boot程序：即通过app端通讯，将boot引导程序更新。
  * 授权进入boot程序：进入boot以更新app及其它程序。
  * 替换Dcl功能(可选功能)：在不更新boot情况下，通过app支持端单独更新Dcl。

### 资源包描述
  * 资源包描述为一组常量(0组固定为app)，为每个”单应用多资源“定义管理的FLASH区域，并为boot下载提供握手校验信息：
    + Flash区起始地址：4Byte
    + Flash区容量：4Byte
    + 资源文件幻数：2Byte, boot与PC端握手用。
    + Flash扇区总数：2Byte
    + Flash每个扇区的大小，2Byte为一组，256为单位(如0x0100，则表示扇区大小0x010000)
  * 电脑端软件通过提供前10Byte信息，boot端识别并比较后，即可握手进一步开始下载FLASH数据。

### 程序完备描述
  程序完备描述用于标识自身程序是完整的，主要针对boot程序与app程序。
  程序完备描述位于程序端首个扇区的约定位置(默认App时为其基址+0x400, boot时约定)：
    + 由“4字节起始地址” + “16字节文件名”， 共20字节组成（若Flash写入以8字节对齐，则为24）
      如app时:0x00002000, “applacationV230.bin”
    + 程序端需定义：位于约定位置的，值必须为FLASH擦除后后值(一搬为0xff)。
    + 此部分程序在烧写结束并确认无误后，将此部分内容改为需要的值。
### app引导过程
    + 开机启动boot时，检测某个IO口位以判断是否驻留在boot中。
    + 若无需驻留,则识别app中"程序完备描述"中的起始地址，若与“app资源包描述”的Flash起始对应,
      表示有完整app程序可直接进入，否则可能中途掉电丢失，保持boot在下载程序等待状态。
    + 若需要驻留,则识别boo中的"程序完备描述",若不符合，则回到上一步继续，直接进入APP以防死机。
    + app与boot程序均不完备，板砖了。用编程器下载吧！

### 关于数据加密
    + 数据加密由用户端实现，建议采用数据流式加密。并与复杂度间取得平衡。
    + 建议采用UID或随机码方式关联密匙，以保证每次数据不一致以提高解密难度

### DCL功能 （因使用MODBUS协议报文限制，暂不支持此功能）
#### DCL描述
  DCL描述用于app进入函数及供替换DCL用，以每个函数为一组，8字节为单位:
  * 2Byte：函数幻数。       供与PC端握手用
  * 2Byte：函数预留空间大小。供下载程序校验用
  * 4Byte：函数入口:        供下载程序及app入口用。
  每个组的下标为函数id, 需与app双方约定以实现快速定义

#### (可选功能)替换Dcl要求及流程:
  * Dcl要求: 为每个回调函数留足可能的最大实现空间，如：预计A函数最大复杂度下需要256字节，
    但实现1中，仅占用了100字节，则在此函数尾部应留156字节空间，以防止更新至实现2时，超100时程序出错。
  * 替换要求：替换程序在app区实现。
  * 替换流程：因DCL与boot程序位于同一区域，故按下述进行：
    + app进入boot下载功能：此时除需要的通讯口外，应停止所有其它程序或中断的运行。
    + 将boot区FLASH copy入写缓冲区
    + 与电脑端建立联系后，根据DCL描述及通讯端传输的函数bin数据，替换掉该函数,直到需要替换的函数全部完成。
    + 将新的boot区FLASH缓冲区数据，替换掉原boot区FLASH
    + 校验boot区FLASH后，重启。
    


