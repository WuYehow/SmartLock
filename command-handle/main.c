//此两个函数用于智能门锁的串口命令处理，命令和参数均需小于9个字符，如果命令超过9个字符将会舍弃多余字符和参数。如果参数多于9个字符，将舍弃多余部分。
#include<stdio.h>
#include<string.h>
// 命令部分的相关定义
int cmd_start=0;
int cmd_parastart=0;
char cmd_input[10];
char cmd_parainput[10];  //一个命令只可附加一个参数
int cmd_paracount=0;
int cmd_count=0;  //命令计数
//指针数组用于存放命令，注意此处顺序将影响do_command函数。
char *command[]=
{
    "unlock",   //开锁
    "beepon",   //报警
};

void do_command(int cmd_len)
{
    int cmd_do=0;
    int cmd_order;
    //打印命令数组和参数数组记录的值
    for(cmd_order=0;cmd_order<10;cmd_order++)
        printf("%c",cmd_input[cmd_order]);
    for(cmd_order=0;cmd_order<10;cmd_order++)
        printf("%c",cmd_parainput[cmd_order]);
    for(cmd_order=0;cmd_order<cmd_len;cmd_order++)
        if(strcmp(command[cmd_order],cmd_input)==0)
        {
                switch(cmd_order)
                    {
                        case 0: /*opendoor()*/printf("open");break;
                        case 1: /*error()*/printf("deep");break;
                    }
                cmd_do=1;
        }
    if(cmd_do==0)
        printf("The command does not exist.");
    //清除数组内容
    cmd_input[0]='\0';
    cmd_parainput[0]='\0';
}

void handle_command(char receive_data)
{
    if(receive_data=='#')  //命令开始符
    {

        cmd_start=1;
        cmd_parastart=0;
    }
    else if(receive_data==' '&&cmd_start==1)
    {
            cmd_parastart=1;
            cmd_input[cmd_count]='\0';
    }
    else if((receive_data=='*'||cmd_count==9||cmd_paracount==9)&&cmd_start==1)  //命令结束符；防止命令溢出
    {
        if(cmd_parastart==1)
            cmd_parainput[cmd_paracount]='\0';
        else
            cmd_input[cmd_count]='\0';
        //return 0;  //返回值表示命令、参数记录成功
        //命令计数器归位
        cmd_count=0;
        cmd_start=0;
        cmd_paracount=0;
        cmd_parastart=0;
        //转到执行命令函数
        do_command(sizeof(command)/sizeof(char *));
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

int main(void)
{
    char in[20]={'#','d','e','8',' ','o','5','5','1','5','7','1','7','6','4','*'};
    int i;
    for(i=0;i<20;i++)
    {
        handle_command(in[i]);
    }
    return 0;
}

//write by Yehow at 2017.3.14
//revise by Yehow at 2017.3.17
