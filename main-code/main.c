#include "reg52.h"			 //此文件中定义了单片机的一些特殊功能寄存器
#include "string.h"

typedef unsigned int u16;	  //对数据类型进行声明定义
typedef unsigned char u8;

#define GPIO_KEY P1  //矩阵键盘的io
#define pwlen 4  //密码位数

//定义特殊io
sbit beep=P1^5;  //蜂鸣器的io口
sbit door=P2^0;  //门锁的io口
sbit light=P2^1; //光警告io口

//其他定义
u16 countkey;	      //用来存放读取到的键值序号
u16 adminpwon;      //控制管理员密码是否生效
u8 inkey[pwlen+1]={'\0'};			  //存储获取的密码
u8 adminpw[]="0123";//管理员密码
u16 receiveData;

//命令部分的相关定义
u16 cmd_parastart=0;
u16 cmd_start=0;
u8 cmd_input[5];  //命令长度4个字符
u8 cmd_parainput[5];  //一个命令只可附加一个参数
u16 cmd_paracount=0;
u16 cmd_count=0;      //命令计数
u8 *command[]=        //指针数组用于存放命令，注意此处顺序将影响do_command函数。
{
  "0x00",   //开锁 无参数
  "0x01",   //报警 无参数
  "0x02",  //更改管理员密码，需附带欲改密码参数
  "0x03"	 //启用或禁止管理员密码，参数on为打开，参数off为关闭
};

//初始化串口
void UsartInit()
{
	SCON=0X50;			//设置为工作方式1
	TMOD=0X20;			//设置计数器工作方式2
	PCON=0X80;			//波特率加倍
	TH1=0XF3;				//计数器初始值设置，注意波特率是4800的
	TL1=0XF3;
	ES=1;						//打开接收中断
	EA=1;						//打开总中断
	TR1=1;					//打开计数器
}

//串口发送字符串
void sendChar(u8 ch)
{
   SBUF=ch;       // 送入缓冲区
   while(TI!=1);  // 等待发送完毕
   TI=0;          // 软件清零
}

//串口发送
void sendstr(u8 *str)
{
  //添加头部
  SBUF='#';
  while(TI!=1);
  TI=0;
  while(*str!='\0')
  sendChar(*str++);
  //添加尾部
  SBUF='*';
  while(TI!=1);
  TI=0;
}

//延时函数
void delay(u16 i)
{
	while(i--);
}

//声光警告，参数为1时为按键结束提示，其他值时为持续时间
void warn(u16 beepdelay)
{
	if(beepdelay==1)
	{
		beepdelay=2000;
	  while(beepdelay--)
	  {
			beep=~beep;
			delay(25);
		}
	}
	else
	{
		light=0;		     //光警告开
		while(beepdelay--)
		{
			beep=~beep;
			delay(15);
		}
		light=1;		     //光警告关
	}
}

//密码开门,参数为1时为管理员密码开门
void opendoor(int i)
{
	if(i==1)
		sendstr("0x51");
	else
		sendstr("0x50");
	door=0;		        //开门
	delay(10000);
	door=1;				    //关门
}

//按键读取
void KeyDown(void)
{
	u16 KeyValue;
	u16 a=0;
	GPIO_KEY=0x0f;
	if(GPIO_KEY!=0x0f)  //读取按键是否按下
	{
		delay(1000);      //延时10ms进行消抖
		if(GPIO_KEY!=0x0f&&countkey!=4)  //再次检测键盘是否按下
		{
			//测试列
			GPIO_KEY=0X0F;
			switch(GPIO_KEY)
			{
				case(0X07):	KeyValue=0;break;
				case(0X0b):	KeyValue=1;break;
				case(0X0d):	KeyValue=2;break;
				case(0X0e):	KeyValue=3;break;
		    }
			//测试行
			GPIO_KEY=0XF0;
			switch(GPIO_KEY)
			{
				case(0X70):	KeyValue=KeyValue;break;
				case(0Xb0):	KeyValue=KeyValue+4;break;
				case(0Xd0): KeyValue=KeyValue+8;break;
				case(0Xe0):	KeyValue=KeyValue+12;break;
			}
			while((a<50)&&(GPIO_KEY!=0xf0))	 //检测按键松手检测
			{
				delay(1000);
				a++;
			}
			countkey++;
			if(KeyValue>=0&&KeyValue<=9)
				inkey[countkey-1]=KeyValue+'0';
			else if(KeyValue==10)
				countkey=countkey-2;  //清除功能
			else if(KeyValue==12)
				countkey=0;           //重输功能
		}
	}
}

//命令出错时清楚计数器，当参数为1时将清空命令数组和参数数组
void error_command(u16 clear)
{
	if(clear==1)
	{
		cmd_input[0]='\0';
		cmd_parainput[0]='\0';
	}
		cmd_paracount=0;
		cmd_count=0;
		cmd_start=0;
		cmd_parastart=0;
}

//命令执行
void do_command(u16 cmd_len)
{
  u16 cmd_do=0;
  u16 i;                     //多用变量
  u16 cmd_order;
	/*
	sendstr(cmd_input);      //将接收到的命令发回
	sendstr(cmd_parainput);  //将接受到的参数发回
	*/
  for(cmd_order=0;cmd_order<cmd_len;cmd_order++)
		if(strcmp(command[cmd_order],cmd_input)==0)
		{
			switch(cmd_order)
            {
				//开门			
                case 0: if(strlen(cmd_parainput)==0)
                            opendoor(0);  //开门
                        else
                            sendstr("0x54");
                        break;
				//报警
                case 1: if(strlen(cmd_parainput)==0)
                        {
                            sendstr("0x53");
                            warn(20000); //警告
                        }
                        else
                            sendstr("0x54");
                        break;
				//更改密码
                case 2: if(strlen(cmd_parainput)==4)
                        {
                            for(i=0;i<4;i++)
                                if(cmd_parainput[i]<='9'&&cmd_parainput[i]>='0');  //判断是否为数字
																else
																	i=5;
                            if(i==4)
                            {
                                strcpy(adminpw,cmd_parainput);
                                sendstr("0x53");
                            }
                            else
                                sendstr("0x54");
                        }
                        else
                            sendstr("0x54");
                        break;
				//开启或关闭管理员密码
                case 3: if(strcmp(cmd_parainput,"on")==0)
                        {
                            adminpwon=1;
                            sendstr("0x53");
                        }
                        else if(strcmp(cmd_parainput,"off")==0)
                        {
                            adminpwon=0;
                            sendstr("0x53");
                        }
                        else
                            sendstr("0x54");
                        break;
            }
						cmd_do=1;
		}
    if(cmd_do==0)
    sendstr("0x52");
    //清除数组内容
    cmd_input[0]='\0';
    cmd_parainput[0]='\0';
}

//命令处理
void handle_command(u8 receive_data)
{
		if(receive_data=='#')  //命令开始符
		{
		cmd_count=0;
    cmd_start=1;
    cmd_parastart=0;
		cmd_paracount=0;
		}
		else if(receive_data==' '&&cmd_start==1)
		{
			if(cmd_parastart==1)
			{
				error_command(1);
				sendstr("0x54");
			}
			else if(cmd_count!=4)
			{
				error_command(1);
				sendstr("0x52");
			}
			cmd_parastart=1;
    }
    else if(receive_data=='*'&&cmd_start==1)  //命令结束符
    {
			if(cmd_parastart==1)
				cmd_parainput[cmd_paracount]='\0';
			else
				cmd_input[cmd_count]='\0';
			error_command(0);
      //转到执行命令函数
			do_command(sizeof(command)/sizeof(char *));
    }
		else if((cmd_count==4&&cmd_parastart==0)||cmd_paracount==4)
		{
			if(cmd_paracount==4)
				sendstr("0x54");
			else
				sendstr("0x52");
			error_command(1);
		}
		else if(cmd_start==1)
    {
			switch(cmd_parastart)
      {
				case 0: cmd_input[cmd_count++]=receive_data;
                break;
        case 1: cmd_parainput[cmd_paracount++]=receive_data;
                break;
      }
    }
}

/*
//串口发送密码
void sendpw(void)
{
	ES=0;  //关闭接收中断
  SBUF='#';                      //将接收到的数据放入到发送寄存器
	while(!TI);			         //等待发送数据完成
	TI=0;				         //清除发送完成标志位
  for(countkey=0;countkey<4;countkey++)
	{
		SBUF=inkey[countkey];    //将接收到的数据放入到发送寄存器
		while(!TI);			     //等待发送数据完成
		TI=0;				     //清除发送完成标志位
	}
	SBUF='*';                    //将接收到的数据放入到发送寄存器
	while(!TI);			         //等待发送数据完成
	TI=0;				         //清除发送完成标志位
	ES=1;                        //打开接收中断
}
*/

//中断（串口接受指令）
void Usart() interrupt 4
{
	receiveData=SBUF;	      //记录接收到的数据
	RI = 0;			          //清除接收中断标志位
	handle_command(receiveData);
}

//主函数
void main(void)
{
	UsartInit();           //串口初始化
	adminpwon=0;
	while(1)
	{
		countkey=0;
		while(countkey<pwlen)
		{
			KeyDown();		  //按键读取
		}
		warn(1);              //按键结束提示
		if(strcmp(adminpw,inkey)==0&&adminpwon==1)
			opendoor(1);
			else
			sendstr(inkey);
	}
}
