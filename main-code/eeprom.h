#ifndef __EEPROM_H_
#define __EEPROM_H_

#include <reg52.h>
/******************定义命令字节******************/         
#define read_cmd     0x01                   //字节读数据命令       
#define wirte_cmd    0x02                   //字节编程数据命令       
#define erase_cmd    0x03                   //扇区擦除数据命令                 
    
/****************特殊功能寄存器声明****************/    
sfr ISP_DATA = 0xe2;     
sfr ISP_ADDRH = 0xe3;       
sfr ISP_ADDRL = 0xe4;     
sfr ISP_CMD = 0xe5;     
sfr ISP_TRIG = 0xe6;        
sfr ISP_CONTR = 0xe7;  
  
/*定义Flash 操作等待时间及允许IAP/ISP/EEPROM 操作的常数******************/  
//#define enable_waitTime 0x80 //系统工作时钟<30MHz 时，对IAP_CONTR 寄存器设置此值  
//#define enable_waitTime 0x81 //系统工作时钟<24MHz 时，对IAP_CONTR 寄存器设置此值  
//#define enable_waitTime 0x82  //系统工作时钟<20MHz 时，对IAP_CONTR 寄存器设置此值  
#define enable_waitTime 0x83 //系统工作时钟<12MHz 时，对IAP_CONTR 寄存器设置此值  
//#define enable_waitTime 0x84 //系统工作时钟<6MHz 时，对IAP_CONTR 寄存器设置此值  

void ISP_IAP_disable(void);
void ISP_IAP_trigger(void);
void ISP_IAP_readData(unsigned int beginAddr, unsigned char* pBuf, unsigned int dataSize);
void ISP_IAP_writeData(unsigned int beginAddr, unsigned char* pDat, unsigned int dataSize);
void ISP_IAP_sectorErase(unsigned int sectorAddr);

#endif
