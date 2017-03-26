/*
版权所有 © 吴艺豪 2017
此为成都理工大学智能门锁项目的单片机部分代码。
更新历史:
时间                                    版本号                                    修改记录
2017.3.26                               1.0                                       代码重构，修正格式中不规范的地方。
*/

#include "reg52.h"
#include "string.h"
#include "eeprom.h"
#include "lcd.h"

typedef unsigned int uint;
typedef unsigned char uchar;

#define GPIO_KEY P1 //矩阵键盘的io
#define pwlen 4     //密码位数

//定义特殊io
sbit beep = P1^5;   //蜂鸣器io
sbit door = P2^0;   //门锁io
sbit light = P2^1;  //光警告io

//其他定义
uint countkey;           //用来存放读取到的键值序号
uchar adminpwon;         //控制管理员密码是否生效
uchar inkey[pwlen+1] = {'\0'};   //存储获取的密码
uchar adminpw[5];        //管理员密码

//命令部分的相关定义
uint cmd_parastart = 0, cmd_start = 0;
uchar cmd_input[5], cmd_parainput[5];        //命令长度4个字符
uint cmd_paracount = 0, cmd_count = 0;       //命令计数
//指针数组用于存放命令，注意此处顺序将影响do_command函数。
uchar *command[] =
{
  "0x00",   //开锁 无参数
  "0x01",   //报警 无参数
  "0x02",   //更改管理员密码，需附带欲改密码参数
  "0x03"    //启用或禁止管理员密码，参数on为打开，参数off为关闭
};

uchar *lcd[] =
{
  "adminpw on ",
  "adminpw off",
  "password: ",
  "No adminpw "
};

//初始化串口
void UsartInit()
{
  SCON=0X50;            //设置为工作方式1
  TMOD=0X20;            //设置计数器工作方式2
  PCON=0X80;            //波特率加倍
  TH1=0XF3;             //计数器初始值设置，注意波特率是4800的
  TL1=0XF3;
  ES=1;                 //打开接收中断
  EA=1;                 //打开总中断
  TR1=1;                //打开计数器
}

//串口发送（中断）
void sendstr(uchar *str)
{
  EA = 0;                 //关闭总中断
  //添加头部
  SBUF='#';
  while(TI != 1);
    TI=0;
  while(*str != '\0')
  {
    SBUF=*str++;          //送入缓冲区
    while(TI != 1);       //等待发送完毕
    TI = 0;               //软件清零
  }
  //添加尾部
  SBUF='*';
  while(TI! = 1);
    TI = 0;
  EA = 1;                 //打开总中断
}

//延时函数
void delay(uint i)
{
  while (i--);
}

//声光警告，参数为1时为按键结束提示，其他值时为持续时间
void warn(uint beepdelay)
{
  if(beepdelay == 1)
  {
    beepdelay = 2000;
    while(beepdelay--)
    {
      beep = ~beep;
      delay(25);
    }
  }
  else
  {
    light = 0;          //光警告开
    while(beepdelay--)
    {
      beep = ~beep;
      delay(15);
    }
    light = 1;          //光警告关
  }
}

//按键读取
void KeyDown(void)
{
  uint KeyValue;
  uint a = 0;
  GPIO_KEY = 0x0f;
  if (GPIO_KEY != 0x0f)        //读取按键是否按下
  {
    delay(1000);               //延时10ms进行消抖
    if (GPIO_KEY != 0x0f)      //再次检测键盘是否按下
    {
      //测试列
      GPIO_KEY = 0X0F;
      switch (GPIO_KEY)
      {
        case(0X07): KeyValue = 1;break;
        case(0X0b): KeyValue = 2;break;
        case(0X0d): KeyValue = 3;break;
        //case(0X0e): KeyValue = 3;break;
      }
      //测试行
      GPIO_KEY = 0XF0;
      switch (GPIO_KEY)
      {
        case(0X70): KeyValue = KeyValue;break;
        case(0Xb0): KeyValue = KeyValue + 3;break;
        case(0Xd0): KeyValue = KeyValue + 6;break;
        case(0Xe0): KeyValue = KeyValue + 9;break;
      }
      while ((a < 50) && (GPIO_KEY != 0xf0))    //检测按键松手检测
      {
        delay(1000);
        a++;
      }
      countkey++;
      if (KeyValue > 0&&KeyValue <= 9)
        inkey[countkey - 1] = KeyValue + '0';
      else if (KeyValue == 10 && countkey > 1)
        countkey=countkey - 2;   //清除功能
      else if (KeyValue == 11)
        inkey[countkey - 1] = '0';
      else if (KeyValue == 12 && countkey>1)
        countkey = 0;            //重输功能
      else
        countkey--;
      LcdWriteCom(0x80 + 0x4a);
      for (i = 0; i < pwlen + 1; i++)
        LcdWriteData(' ');
      LcdWriteCom(0x80 + 0x4a);
      for (i = 0; i < countkey; i++)
      { 
        //LcdWriteData(' ');
        LcdWriteData(inkey[i]);
      }
    }
  }
}

//命令出错时清除计数器，当参数为1时将清空命令数组和参数数组
void error_command(uint clear)
{
  if (clear == 1)
  {
    cmd_input[0] = '\0';
    cmd_parainput[0] = '\0';
  }
    cmd_paracount = 0;
    cmd_count = 0;
    cmd_start = 0;
    cmd_parastart = 0;
}

//命令执行
void do_command(uint cmd_len)
{
  uint cmd_do = 0;
  uint loop_count;
  uint cmd_order;
  /*
  sendstr(cmd_input);                //将接收到的命令发回
  sendstr(cmd_parainput);            //将接受到的参数发回
  */
  for (cmd_order = 0; cmd_order < cmd_len; cmd_order++)
    if (strcmp(command[cmd_order], cmd_input) == 0)
    {
      switch (cmd_order)
      {
        //开门      
        case 0: if (strlen(cmd_parainput) == 0)
                {
                  door = 0;           //开门
                  delay(10000);
                  door = 1;           //关门
                  sendstr("0x50");
                }
                else
                  sendstr("0x54");
                break;
        //报警
        case 1: if (strlen(cmd_parainput) == 0)
                {
                  sendstr("0x53");
                  warn(20000);    //警告
                }
                else
                  sendstr("0x54");
                break;
        //更改密码
        case 2: if (strlen(cmd_parainput) == 4)
                {
                  for (loop_count = 0; loop_count < 4; loop_count++)
                    if (cmd_parainput[loop_count] <= '9' && cmd_parainput[loop_count] >= '0');   //判断是否为数字
                    else
                      loop_count = 5;
                    if (loop_count == 4)
                    {
                      strcpy(adminpw, cmd_parainput);
                      ISP_IAP_sectorErase(0x2200);
                      ISP_IAP_writeData(0x2200, adminpw, sizeof(adminpw)); 
                      sendstr("0x53");
                      if (adminpwon == '1')
                      {
                        LcdWriteCom(0x80 + 0x00);
                        lcdstr(lcd[0], strlen(lcd[0]));
                      }
                      else
                      {
                        LcdWriteCom(0x80 + 0x00);
                        lcdstr(lcd[1], strlen(lcd[1]));
                      }
                    }
                    else
                      sendstr("0x54");
                }
                else
                  sendstr("0x54");
                break;
        //开启或关闭管理员密码
        case 3: if(strcmp(cmd_parainput, "on")==0)
                {
                  adminpwon='1';
                  ISP_IAP_sectorErase(0x2000);
                  ISP_IAP_writeData(0x2000, &adminpwon, sizeof(adminpwon));
                  if (adminpw[0] == 0xff);
                  {
                    LcdWriteCom(0x80 + 0x00);
                    lcdstr(lcd[0], strlen(lcd[0]));
                  }
                  sendstr("0x53");
                }
                else if (strcmp(cmd_parainput, "off") == 0)
                {
                  adminpwon = '0';
                  ISP_IAP_sectorErase(0x2000);
                  ISP_IAP_writeData(0x2000, &adminpwon, sizeof(adminpwon));
                  if (adminpw[0] == 0xff);
                  {
                    LcdWriteCom(0x80 + 0x00);
                    lcdstr(lcd[1], strlen(lcd[1]));
                  }
                  sendstr("0x53");
                }
                else
                  sendstr("0x54");
                break;
      }
      cmd_do = 1;
    }
  if (cmd_do == 0)
    sendstr("0x52");
  //清除数组内容
  cmd_input[0] = '\0';
  cmd_parainput[0] = '\0';
}

//命令处理
void handle_command(uchar receive_data)
{
  if (receive_data == '#')   //命令开始符
  {
  cmd_count = 0;
  cmd_start = 1;
  cmd_parastart = 0;
  cmd_paracount = 0;
  }
  else if (receive_data == ' '&&cmd_start == 1)
  {
    if (cmd_parastart == 1)
    {
      error_command(1);
      sendstr("0x54");
    }
    else if (cmd_count != 4)
    {
      error_command(1);
      sendstr("0x52");
    }
    cmd_parastart = 1;
  }
  else if (receive_data == '*' && cmd_start == 1)    //命令结束符
  {
    if (cmd_parastart == 1)
      cmd_parainput[cmd_paracount] = '\0';
    else
      cmd_input[cmd_count] = '\0';
    error_command(0);
    //转到执行命令函数
    do_command(sizeof(command) / sizeof(char *));
  }
  else if ((cmd_count == 4 && cmd_parastart == 0) || cmd_paracount == 4)
  {
    if (cmd_paracount == 4)
      sendstr("0x54");
    else
      sendstr("0x52");
    error_command(1);
  }
  else if (cmd_start == 1)
  {
    switch (cmd_parastart)
    {
      case 0: cmd_input[cmd_count++]=receive_data;
              break;
      case 1: cmd_parainput[cmd_paracount++]=receive_data;
              break;
    }
  }
}

//串口发送（中断外使用）
void sendstring(uchar *str)
{
  ES = 0;               //关闭接收中断
  SBUF = '#';           //将接收到的数据放入到发送寄存器
  while (!TI);          //等待发送数据完成
    TI = 0;             //清除发送完成标志位
  while (*str != '\0')
  {
    SBUF = *str++;      //将接收到的数据放入到发送寄存器
    while (!TI);        //等待发送数据完成
      TI = 0;           //清除发送完成标志位
  }
  SBUF = '*';           //将接收到的数据放入到发送寄存器
  while (!TI);          //等待发送数据完成
    TI = 0;             //清除发送完成标志位
  ES = 1;               //打开接收中断
}

//中断（串口接受指令）
void Usart() interrupt 4
{
  uint receiveData;
  receiveData=SBUF;       //记录接收到的数据
  RI = 0;                 //清除接收中断标志位
  handle_command(receiveData);
}


//主函数
void main(void)
{
  UsartInit();          //串口初始化
  LcdInit();            //LCD初始化
  uint loop_count;
  ISP_IAP_readData(0x2000, &adminpwon, sizeof(adminpwon));
  ISP_IAP_readData(0x2200, adminpw, sizeof(adminpw));
  if (adminpwon == 0xff)
    adminpwon = '0';
  if (adminpw[0] == 0xff)
  {
    sendstring("0x55");
    lcdstr(lcd[3], strlen(lcd[3]));
  }
  else if (adminpwon == '1')
    lcdstr(lcd[0], strlen(lcd[0]));
  else if (adminpwon == '0')
    lcdstr(lcd[1], strlen(lcd[1]));
  LcdWriteCom(0x80 + 0x40);
  lcdstr(lcd[2], strlen(lcd[2]));
  while (1)
  {
    countkey = 0;
    while (countkey < pwlen)
      KeyDown();        //按键读取
    warn(1);            //按键结束提示
    if (strcmp(adminpw, inkey) == 0 && adminpwon == '1')
    {
      door = 0;         //开门
      delay(10000);
      door = 1;
      sendstring("0x51");
    }
    else
      sendstring(inkey);
    LcdWriteCom(0x80 + 0x4a);
    for (loop_count = 0; loop_count < pwlen + 1; loop_count++)
      LcdWriteData(' ');
  }
}
