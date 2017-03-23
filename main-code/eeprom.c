#include "eeprom.h"  

void ISP_IAP_disable()                  //关闭ISP_IAP  
{     
    EA=1;                                   //恢复中断  
    ISP_CONTR = 0x00;       
    ISP_CMD = 0x00;        
    ISP_TRIG = 0x00;                                               
}  
  
void ISP_IAP_trigger()                       //触发   
{     
    EA=0;                                    //下面的2条指令必须连续执行,故关中断             
    ISP_TRIG = 0x46;                         //送触发命令字0x46       
    ISP_TRIG = 0xB9;                         //送触发命令字0xB9       
}  
         
  
void ISP_IAP_readData(unsigned int beginAddr,unsigned char* pBuf, unsigned int dataSize) //读取数据  
{  
    ISP_DATA=0;                               //清零,不清也可以
    ISP_CMD = read_cmd;                       //指令:读取
    ISP_CONTR = enable_waitTime;              //开启ISP_IAP，并送等待时间 
    while(dataSize--)                         //循环读取
    {  
        ISP_ADDRH = (unsigned char)(beginAddr >> 8);     //送地址高字节   
        ISP_ADDRL = (unsigned char)(beginAddr & 0x00ff); //送地址低字节
        ISP_IAP_trigger();                  //触发
        beginAddr++;                        //地址++ 
        *pBuf++ = ISP_DATA;                 //将数据保存到接收缓冲区
    }  
    ISP_IAP_disable();                      //关闭ISP_IAP功能  
}         
     
void ISP_IAP_writeData(unsigned int beginAddr,unsigned char* pDat,unsigned int dataSize) //写数据      
{             
    ISP_CONTR = enable_waitTime;                //开启ISP_IAP，并送等待时间         
    ISP_CMD = wirte_cmd;                        //送字节编程命令字  
    while(dataSize--)  
    {  
        ISP_ADDRH = (unsigned char)(beginAddr >> 8);       //送地址高字节         
        ISP_ADDRL = (unsigned char)(beginAddr & 0x00ff);   //送地址低字节             
        ISP_DATA = *pDat++;                     //送数据         
        beginAddr++;   
        ISP_IAP_trigger();                      //触发  
    }            
    ISP_IAP_disable();   //关闭                                        
}      
          
void ISP_IAP_sectorErase(unsigned int sectorAddr)        //扇区擦除  
{             
    ISP_CONTR = enable_waitTime;                //开启ISP_IAP;并送等待时间         
    ISP_CMD = erase_cmd;                        //送扇区擦除命令字           
    ISP_ADDRH = (unsigned char)(sectorAddr >> 8);            //送地址高字节         
    ISP_ADDRL = (unsigned char)(sectorAddr & 0X00FF);      //送地址低字节             
    ISP_IAP_trigger();                          //触发  
    ISP_IAP_disable();                          //关闭ISP_IAP功能           
}  
