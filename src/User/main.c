#include "sys.h"
#include "delay.h"
#include "lcd1602.h"
#include "timer.h"
#include "gpio.h"
#include "dht11.h"
#include "ds1302.h"
#include "usart1.h"
#include "esp8266.h"
#include "hx711.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"

#define FLASH_SAVE_ADDR  ((u32)0x0800F000)  				//设置FLASH 保存地址(必须为偶数)

#define ON   1
#define OFF  0

typedef struct {
unsigned char  shi;	  //时
unsigned char	 fen;		//分
unsigned char	 state; //状态
unsigned char  temporary;
}user;

user  Remtime1={8 ,0,1,0};   //提醒时间1
user  Remtime2={12,0,1,0};   //提醒时间2
user  Remtime3={19,0,1,0};   //提醒时间3

unsigned char  lcd_char_buf[]={0x00,0x10,0x07,0x08,0x08,0x08,0x07,0x00};
unsigned char  setn = 0;     //记录设置按键按下的次数
unsigned char display[16];
unsigned char temp,humi;  //温度湿度
bool shuaxin = 0;  
bool shanshuo = 0; 
bool tixing = 0;
bool beepflag=0;
bool SendFlag = 0;
bool SendSwitchState = 0;
bool p_r = 0;	                  //平年/润年  =0表示平年，=1表示润年
unsigned char A=1,B=2,C=3,D=4;   //4种药物数量
unsigned long weight;            //重量值

void STM32_FlashWriteData(void)  //STM32 Flash写入数据
{
u16 temp_buf[13];
u8 add = 0;

temp_buf[add ++] = Remtime1.shi;
temp_buf[add ++] = Remtime1.fen;
temp_buf[add ++] = Remtime1.state;

temp_buf[add ++] = Remtime2.shi;
temp_buf[add ++] = Remtime2.fen;
temp_buf[add ++] = Remtime2.state;

temp_buf[add ++] = Remtime3.shi;
temp_buf[add ++] = Remtime3.fen;
temp_buf[add ++] = Remtime3.state;

temp_buf[add ++] = A;
temp_buf[add ++] = B;
temp_buf[add ++] = C;
temp_buf[add ++] = D;

STMFLASH_Write(FLASH_SAVE_ADDR + 0x60,temp_buf,13); //存入数据
delay_ms(100);
}

void STM32_FlashReadData(void)  //STM32 Flash读出数据
{
u16 temp_buf[13];
u8 add = 0;

STM32F10x_Read(FLASH_SAVE_ADDR + 0x60,temp_buf,13); //读出数据

Remtime1.shi = temp_buf[add ++];
Remtime1.fen = temp_buf[add ++];
Remtime1.state = temp_buf[add ++];

Remtime2.shi = temp_buf[add ++];
Remtime2.fen = temp_buf[add ++];
Remtime2.state = temp_buf[add ++];

Remtime3.shi = temp_buf[add ++];
Remtime3.fen = temp_buf[add ++];
Remtime3.state = temp_buf[add ++];

A = temp_buf[add ++];
B = temp_buf[add ++];
C = temp_buf[add ++];
D = temp_buf[add ++];

delay_ms(100);
}

void STM32_FlashCheck(void)  // 检查是否是新的单片机，是的话清空存储区，否则保留
{
u8 comper_str[6];

STM32F10x_Read(FLASH_SAVE_ADDR + 0x10,(u16*)comper_str,5);
comper_str[5] = '\0';
if(strstr((char *)comper_str,"FDYDZ") == NULL)  //新的单片机
{
STMFLASH_Write(FLASH_SAVE_ADDR + 0x10,(u16*)"FDYDZ",5); //写入“FDYDZ”，方便下次校验
delay_ms(50);
STM32_FlashWriteData();
}
STM32_FlashReadData();

delay_ms(100);
}

void displayInitChar(void)   //显示初始界面
{
LCD_Write_String(0,0,"  /  /         g");//显示字符串
LCD_Write_String(0,1,"  :  :         %");
Lcd_Custom_Char(1,11,0,lcd_char_buf);
}

void write_week(u8 week)//写星期函数
{
LCD_Write_Com(0x80+9);//星期字符的显示位置
switch(week)
{
case 1:
LCD_Write_Data('M');//星期数为1时，显示
LCD_Write_Data('O');
break;

case 2:
LCD_Write_Data('T');//星期数据为2时显示
LCD_Write_Data('U');
break;

case 3:
LCD_Write_Data('W');//星期数据为3时显示
LCD_Write_Data('E');
break;

case 4:
LCD_Write_Data('T');//星期数据为4是显示
LCD_Write_Data('H');
break;

case 5:
LCD_Write_Data('F');//星期数据为5时显示
LCD_Write_Data('R');
break;

case 6:
LCD_Write_Data('S');//星期数据为6时显示
LCD_Write_Data('A');
break;

case 0:
LCD_Write_Data('S');//星期数据为7时显示
LCD_Write_Data('U');
break;
}

}

void displayTime(void) //显示时间
{
//显示时间
LCD_Write_Com(0x80);
LCD_Write_Data(SysDate.year/10+0x30);
LCD_Write_Data(SysDate.year%10+0x30);
LCD_Write_Data('/');
LCD_Write_Data(SysDate.mon/10+0x30);
LCD_Write_Data(SysDate.mon%10+0x30);
LCD_Write_Data('/');
LCD_Write_Data(SysDate.day/10+0x30);
LCD_Write_Data(SysDate.day%10+0x30);

SysDate.week=Conver_week(SysDate.year,SysDate.mon,SysDate.day);
write_week(SysDate.week);

LCD_Write_Com(0x80+0x40);
LCD_Write_Data(SysDate.hour/10+0x30);
LCD_Write_Data(SysDate.hour%10+0x30);
LCD_Write_Data(':');
LCD_Write_Data(SysDate.min/10+0x30);
LCD_Write_Data(SysDate.min%10+0x30);
LCD_Write_Data(':');
LCD_Write_Data(SysDate.sec/10+0x30);
LCD_Write_Data(SysDate.sec%10+0x30);
}

void Get_Ds1302_Time(void)//获取DS1302时间
{
DS1302_DateRead(&SysDate);//读时间
if(setn == 0 && tixing==0){
displayTime();
}
}

void displaySetTime(const user * const time)   //显示提醒时间
{
if(time->state == 1)LCD_Write_String(6,1," ON");else LCD_Write_String(6,1,"OFF");

LCD_Write_Char(11,1,time->shi/10+'0');
LCD_Write_Char(12,1,time->shi%10+'0');

LCD_Write_Char(14,1,time->fen/10+'0');
LCD_Write_Char(15,1,time->fen%10+'0');
}

void SetReminderTime(bool mode,u8 index ,user * const time)  //设置提醒时间
{
if(mode == 0)
{
switch(index)
{
case(1):
time->state = ON; LCD_Write_String(6,1," ON");
LCD_Write_Com(0x80+0x40+8);
LCD_Write_Com(0x0f);//开启光标     
break;
case(2):  
time->shi ++;
if(time->shi == 24)time->shi = 0;
LCD_Write_Char(11,1,time->shi/10+'0');
LCD_Write_Char(12,1,time->shi%10+'0');
LCD_Write_Com(0x80+0x40+12);
LCD_Write_Com(0x0f);//开启光标     
break;
case(3): 
time->fen ++;
if(time->fen == 60)time->fen = 0;
LCD_Write_Char(14,1,time->fen/10+'0');
LCD_Write_Char(15,1,time->fen%10+'0');
LCD_Write_Com(0x80+0x40+15);
LCD_Write_Com(0x0f);//开启光标     
break;

default:break;
}
}
else
{
switch(index)
{
case(1): 
time->state = OFF; LCD_Write_String(6,1,"OFF");
LCD_Write_Com(0x80+0x40+8);
LCD_Write_Com(0x0f);//开启光标     
break;
case(2):
if(time->shi == 0)time->shi = 24;
time->shi --;
LCD_Write_Char(11,1,time->shi/10+'0');
LCD_Write_Char(12,1,time->shi%10+'0');
LCD_Write_Com(0x80+0x40+12);
LCD_Write_Com(0x0f);//开启光标     
break;
case(3): 
if(time->fen == 0)time->fen = 60;
time->fen --;
LCD_Write_Char(14,1,time->fen/10+'0');
LCD_Write_Char(15,1,time->fen%10+'0');
LCD_Write_Com(0x80+0x40+15);
LCD_Write_Com(0x0f);//开启光标     
break;

default:break;
}
}
}

void displayMedicineNum(void)      //显示药物数量
{
if(setn==0 && tixing && shanshuo && A!=0)
{
LCD_Write_Char(5,0,' ');
LCD_Write_Char(6,0,' ');
}
else
{
if(A==0)
{
LCD_Write_Char(5,0,'-');
LCD_Write_Char(6,0,'-');
}
else
{
LCD_Write_Char(5,0,A/10+'0');
LCD_Write_Char(6,0,A%10+'0');
}
}
if(setn==0 && tixing && shanshuo && B!=0)
{
LCD_Write_Char(14,0,' ');
LCD_Write_Char(15,0,' ');
}
else
{
if(B==0)
{
LCD_Write_Char(14,0,'-');
LCD_Write_Char(15,0,'-');
}
else
{
LCD_Write_Char(14,0,B/10+'0');
LCD_Write_Char(15,0,B%10+'0');
}
}

if(setn==0 && tixing && shanshuo && C!=0)
{
LCD_Write_Char(5,1,' ');
LCD_Write_Char(6,1,' ');
}
else
{
if(C==0)
{
LCD_Write_Char(5,1,'-');
LCD_Write_Char(6,1,'-');
}
else
{
LCD_Write_Char(5,1,C/10+'0');
LCD_Write_Char(6,1,C%10+'0');
}
}

if(setn==0 && tixing && shanshuo && D!=0)
{
LCD_Write_Char(14,1,' ');
LCD_Write_Char(15,1,' ');
}
else
{
if(D==0)
{
LCD_Write_Char(14,1,'-');
LCD_Write_Char(15,1,'-');
}
else
{
LCD_Write_Char(14,1,D/10+'0');
LCD_Write_Char(15,1,D%10+'0');
}
}
}

void SetMedicineNum(bool mode,u8 index)  //设置吃药数量
{
if(mode == 0)
{
switch(index)
{
case(1): if(A < 99)A ++; displayMedicineNum();LCD_Write_Com(0x80+6);
LCD_Write_Com(0x0f);//开启光标
break;
case(2): if(B < 99)B ++; displayMedicineNum();LCD_Write_Com(0x80+15);
LCD_Write_Com(0x0f);//开启光标
break;
case(3): if(C < 99)C ++; displayMedicineNum();LCD_Write_Com(0x80+0x40+6);
LCD_Write_Com(0x0f);//开启光标
break;
case(4): if(D < 99)D ++; displayMedicineNum();LCD_Write_Com(0x80+0x40+15);
LCD_Write_Com(0x0f);//开启光标
break;
}
}
else
{
switch(index)
{
case(1): if(A > 0)A --; displayMedicineNum();LCD_Write_Com(0x80+6);
LCD_Write_Com(0x0f);//开启光标
break;
case(2): if(B > 0)B --; displayMedicineNum();LCD_Write_Com(0x80+15);
LCD_Write_Com(0x0f);//开启光标
break;
case(3): if(C > 0)C --; displayMedicineNum();LCD_Write_Com(0x80+0x40+6);
LCD_Write_Com(0x0f);//开启光标
break;
case(4): if(D > 0)D --; displayMedicineNum();LCD_Write_Com(0x80+0x40+15);
LCD_Write_Com(0x0f);//开启光标
break;
}
}
}

bool keyLongPress=0;
unsigned int keyPressTime=0;//按键按下计数变量
unsigned int SetFlag=0;

void displayCursor1(void)
{
if(SetFlag == 1)//设置状态
{
LCD_Write_Com(0x80+0x40+8);
LCD_Write_Com(0x0f);//开启光标
}
if(SetFlag == 2)//设置时
{
LCD_Write_Com(0x80+0x40+12);
LCD_Write_Com(0x0f);
}
if(SetFlag == 3)//设置分
{
LCD_Write_Com(0x80+0x40+15);
LCD_Write_Com(0x0f);
}
}

void displayCursor2(void)
{
if(SetFlag == 1)//设置A
{
LCD_Write_Com(0x80+6);
LCD_Write_Com(0x0f);//开启光标
}
if(SetFlag == 2)//设置B
{
LCD_Write_Com(0x80+15);
LCD_Write_Com(0x0f);
}
if(SetFlag == 3)//设置C
{
LCD_Write_Com(0x80+0x40+6);
LCD_Write_Com(0x0f);
}
if(SetFlag == 4)//设置D
{
LCD_Write_Com(0x80+0x40+15);
LCD_Write_Com(0x0f);
}
}

void keyscan(void)  //按键扫描
{
u16 nian_temp;

if(KEY1 == 0)//设置按键
{
delay_ms(50);//消抖
if(KEY1 == 0)
{
while(KEY1 == 0)
{
if(keyPressTime<60000 && setn >= 7)keyPressTime++;
if(keyPressTime >= 500)   //长按
{
keyLongPress=!keyLongPress;
if(keyLongPress == 0)
{				
SetFlag=0;
LCD_Write_Com(0x0C);
}
else
{
SetFlag=1;
switch(setn)
{
case(7):LCD_Write_Com(0x80+0x40+8);LCD_Write_Com(0x0F);break;
case(8):LCD_Write_Com(0x80+0x40+8);LCD_Write_Com(0x0F);break;
case(9):LCD_Write_Com(0x80+0x40+8);LCD_Write_Com(0x0F);break;
case(10):LCD_Write_Com(0x80+6);LCD_Write_Com(0x0F);break;
case(11):LCD_Write_Com(0x80+0x40+2);LCD_Write_Com(0x0F);break;
default:break;
}
}
beepflag=1;
BEEP = 1;
delay_ms(300);
BEEP = 0;
beepflag=0;
break;
}
delay_ms(1);
}
if(keyPressTime < 500)  //短按
{
if(keyLongPress == 0)
{
setn++;

nian_temp=2000+SysDate.year;
if((nian_temp%400==0)||((nian_temp%100!=0)&&(nian_temp%4==0)))  //判断是否为闰年
p_r=1;
else
p_r=0;
if(setn == 1)//设0置年
{
tixing = 0;
LCD_Clear();
displayInitChar();

sprintf((char *)display,"%03dg",(int)weight);
LCD_Write_String(12,0,display);

LCD_Write_Char(9,1,temp/10+'0'); 
LCD_Write_Char(10,1,temp%10+'0'); 
LCD_Write_Char(13,1,humi/10+'0'); 
LCD_Write_Char(14,1,humi%10+'0');  

displayTime();

LCD_Write_Com(0x80+1);
LCD_Write_Com(0x0f);//开启光标
}
if(setn == 2)//设置月
{
LCD_Write_Com(0x80+4);
LCD_Write_Com(0x0f);
}
if(setn == 3)//日
{
LCD_Write_Com(0x80+7);
LCD_Write_Com(0x0f);
}
if(setn == 4)//时
{
LCD_Write_Com(0x80+0x40+1);
LCD_Write_Com(0x0f);
}
if(setn == 5)//分
{
LCD_Write_Com(0x80+0x40+4);
LCD_Write_Com(0x0f);
}
if(setn == 6)//秒
{
LCD_Write_Com(0x80+0x40+7);
LCD_Write_Com(0x0f);
}
if(setn == 7)//设置吃药时间1
{
LCD_Write_Com(0x0C);
LCD_Write_String(0,0,"   Set Time 1   ");
LCD_Write_String(0,1,"State:     00:00");

displaySetTime(&Remtime1);
}
if(setn == 8)//设置吃药时间2
{
LCD_Write_String(0,0,"   Set Time 2   ");
LCD_Write_String(0,1,"State:     00:00");

displaySetTime(&Remtime2);
}
if(setn == 9)//设置吃药时间3
{
LCD_Write_String(0,0,"   Set Time 3   ");
LCD_Write_String(0,1,"State:     00:00");

displaySetTime(&Remtime3);
}
if(setn == 10)//设置吃药药量
{
LCD_Write_String(0,0,"Anum:    Bnum:  ");
LCD_Write_String(0,1,"Cnum:    Dnum:  ");

displayMedicineNum();
}
if(setn >= 11)
{
setn = 0;
LCD_Write_Com(0x0C);
LCD_Clear();
displayInitChar();
STM32_FlashWriteData();   //退出设置之后保存一下设置值
}
}
else
{
SetFlag ++; 
if(setn>=7 && setn<=9)
{
if(SetFlag>3)SetFlag=1;

displayCursor1();
}
if(setn==10)
{
if(SetFlag>4)SetFlag=1;

displayCursor2();
}
}
}
keyPressTime = 0;
while(KEY1 == 0);
}
}
if(KEY2 == 0)//加键
{
delay_ms(10);
if(KEY2 == 0)
{
while(KEY2 == 0);
if(keyLongPress == 0)
{
if(setn == 1)//设置年
{
SysDate.year++;
if(SysDate.year == 100)SysDate.year = 0;//最大到99

DS1302_DateSet(&SysDate);//设置时间

nian_temp=2000+SysDate.year;
if((nian_temp%400==0)||((nian_temp%100!=0)&&(nian_temp%4==0)))  //判断是否为闰年
p_r=1;
else
p_r=0;
displayTime();//显示

LCD_Write_Com(0x80+1);
LCD_Write_Com(0x0f);//光标闪烁
}
else if(setn == 2)//设置月
{
SysDate.mon++;
if(SysDate.mon==13)//最大12个月
SysDate.mon=1;
if((SysDate.mon==4)||(SysDate.mon==6)||(SysDate.mon==9)||(SysDate.mon==11))//4，6，9，11月只有30天
{
if(SysDate.day>30)
SysDate.day=1;
}
else
{
if(SysDate.mon==2)//2月
{
if(p_r==1)//闰年2月有29天
{
if(SysDate.day>29)
SysDate.day=1;
}
else//平年2月只有28天
{
if(SysDate.day>28)
SysDate.day=1;
}
}
}
DS1302_DateSet(&SysDate);//设置时间

displayTime();//显示

LCD_Write_Com(0x80+4);
LCD_Write_Com(0x0f);//光标闪烁
}
else if(setn == 3)//设置日
{
SysDate.day++;
if((SysDate.mon==1)||(SysDate.mon==3)||(SysDate.mon==5)||(SysDate.mon==7)||(SysDate.mon==8)||(SysDate.mon==10)||(SysDate.mon==12))//有31天的月份
{
if(SysDate.day==32)
SysDate.day=1;
}
else
{
if(SysDate.mon==2)
{
if(p_r==1)
{
if(SysDate.day==30)//闰年2月有29天
SysDate.day=1;
}
else
{
if(SysDate.day==29)//平年2月只有28天
SysDate.day=1;
}
}
else
{
if(SysDate.day==31)//最大到30 
SysDate.day=1;
}
}
DS1302_DateSet(&SysDate);//设置时间

displayTime();//显示

LCD_Write_Com(0x80+7);
LCD_Write_Com(0x0f);//光标闪烁
}
else if(setn == 4)//设置时
{
SysDate.hour++;
if(SysDate.hour == 24)//到0点
SysDate.hour = 0;
DS1302_DateSet(&SysDate);//设置时间

displayTime();//显示

LCD_Write_Com(0x80+0x40+1);
LCD_Write_Com(0x0f);//光标闪烁
}
else  if(setn == 5)//设置分
{
SysDate.min++;
if(SysDate.min == 60)//60分钟
SysDate.min = 0;
DS1302_DateSet(&SysDate);//设置时间

displayTime();//显示

LCD_Write_Com(0x80+0x40+4);
LCD_Write_Com(0x0f);//光标闪烁
}
else  if(setn == 6)//设置秒
{
SysDate.sec++;
if(SysDate.sec == 60)//60秒
SysDate.sec = 0;
DS1302_DateSet(&SysDate);//设置时间

displayTime();//显示

LCD_Write_Com(0x80+0x40+7);
LCD_Write_Com(0x0f);//光标闪烁
}
}else
{
if(setn == 7)         //设置第一个吃药时间
{
SetReminderTime(0,SetFlag,&Remtime1);
}
if(setn == 8)        //设置第二个吃药时间
{ 
SetReminderTime(0,SetFlag,&Remtime2);
}
if(setn == 9)       //设置第三个吃药时间
{
SetReminderTime(0,SetFlag,&Remtime3);
}
if(setn==10)       //设置吃药数量
{
SetMedicineNum(0,SetFlag);
}
}
}
}

if(KEY3 == 0)//减键
{
delay_ms(10);
if(KEY3 == 0 )
{
while(KEY3 == 0);
if(keyLongPress == 0)
{  
if(setn == 0)
{
beepflag=1;
BEEP = 1;
delay_ms(300);
BEEP = 0;
beepflag=0;
To_Zero();
}

if(setn == 1)//设置年
{
if(SysDate.year == 0)SysDate.year = 100;
SysDate.year--;
DS1302_DateSet(&SysDate);//设置时间

nian_temp=2000+SysDate.year;
if((nian_temp%400==0)||((nian_temp%100!=0)&&(nian_temp%4==0)))  //判断是否为闰年
p_r=1;
else
p_r=0;

displayTime();

LCD_Write_Com(0x80+1);
LCD_Write_Com(0x0f);
}
else if(setn == 2)//设置月
{
SysDate.mon--;
if(SysDate.mon==0)SysDate.mon=12;
if((SysDate.mon==4)||(SysDate.mon==6)||(SysDate.mon==9)||(SysDate.mon==11))
{
if(SysDate.day>30)SysDate.day=1;
}
else
{
if(SysDate.mon==2)
{
if(p_r==1)
{
if(SysDate.day>29)SysDate.day=1;
}
else
{
if(SysDate.day>28)SysDate.day=1;
}
}
}
DS1302_DateSet(&SysDate);//设置时间

displayTime();

LCD_Write_Com(0x80+4);
LCD_Write_Com(0x0f);
}
else if(setn == 3)//设置日
{
SysDate.day--;
if((SysDate.mon==1)||(SysDate.mon==3)||(SysDate.mon==5)||(SysDate.mon==7)||(SysDate.mon==8)||(SysDate.mon==10)||(SysDate.mon==12))
{
if(SysDate.day==0)SysDate.day=31;
}
else
{
if(SysDate.mon==2)
{
if(p_r==1)
{
if(SysDate.day==0)SysDate.day=29;
}
else
{
if(SysDate.day==0)SysDate.day=28;
}
}
else
{
if(SysDate.day==0)
SysDate.day=30;
}
}
DS1302_DateSet(&SysDate);//设置时间

displayTime();

LCD_Write_Com(0x80+7);
LCD_Write_Com(0x0f);
}
else	if(setn == 4)//设置时
{
if(SysDate.hour == 0)SysDate.hour = 24;
SysDate.hour--;
DS1302_DateSet(&SysDate);//设置时间

displayTime();

LCD_Write_Com(0x80+0x40+1);
LCD_Write_Com(0x0f);
}
else  if(setn == 5)//设置分
{
if(SysDate.min == 0)SysDate.min = 60;
SysDate.min--;
DS1302_DateSet(&SysDate);//设置时间

displayTime();

LCD_Write_Com(0x80+0x40+4);
LCD_Write_Com(0x0f);
}
else  if(setn == 6)//设置秒
{
if(SysDate.sec == 0)SysDate.sec = 60;
SysDate.sec--;
DS1302_DateSet(&SysDate);//设置时间

displayTime();

LCD_Write_Com(0x80+0x40+7);
LCD_Write_Com(0x0f);
}
}else
{
if(setn == 7)   //设置第一个吃药时间
{
SetReminderTime(1,SetFlag,&Remtime1);
}
if(setn == 8)  //设置第二个吃药时间
{
SetReminderTime(1,SetFlag,&Remtime2);
}
if(setn == 9) //设置第三个吃药时间
{
SetReminderTime(1,SetFlag,&Remtime3);
}
if(setn==10)  //设置吃药数量
{
SetMedicineNum(1,SetFlag);
}
}
}
}
}

void UsartSendReceiveData(void)
{
unsigned char *dataPtr = NULL;
char *str1=0,*str2=0,i;
int  setValue=0;
char setvalue[5]={0};
char SEND_BUF[150];

dataPtr = ESP8266_GetIPD(0);   //接收数据
if(dataPtr != NULL)
{
if(strstr((char *)dataPtr,"CONNECT")!=NULL)
{
SendSwitchState = 1;
}
if(strstr((char *)dataPtr,"one:")!=NULL)  //接收到设置吃药时间的指令
{
beepflag=1;
BEEP = 1;
delay_ms(100);
BEEP = 0;
beepflag=0;					
str1 = strstr((char *)dataPtr,"one:");

while(*str1 < '0' || *str1 > '9')        //判断是不是0到9有效数字
{
str1 = str1 + 1;
delay_ms(10);
}
i = 0;
while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str1;
i ++; str1 ++;
if(*str1 == ':')break;         
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=23)
{
Remtime1.shi = setValue;   //吃药时间的小时
if(setn == 7)//设置吃药时间1
{
displaySetTime(&Remtime1);
displayCursor1();
}
STM32_FlashWriteData();   //保存一下设置值
}
str1 = str1 + 1;
i = 0;
while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str1;
i ++; str1 ++;
if(*str1 == 't')break;         
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符

setValue = atoi(setvalue);
if(setValue>=0 && setValue<=59)
{
Remtime1.fen = setValue;   //吃药时间的小时
if(setn == 7)//设置吃药时间1
{
displaySetTime(&Remtime1);
displayCursor1();
}
STM32_FlashWriteData();   //保存一下设置值
}
}

if(strstr((char *)dataPtr,"two:")!=NULL)  //接收到设置吃药时间的指令
{
str1 = strstr((char *)dataPtr,"two:");

while(*str1 < '0' || *str1 > '9')        //判断是不是0到9有效数字
{
str1 = str1 + 1;
delay_ms(10);
}
i = 0;
while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str1;
i ++; str1 ++;
if(*str1 == ':')break;         
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=23)
{
Remtime2.shi = setValue;   //吃药时间的小时
if(setn == 8)//设置吃药时间2
{
displaySetTime(&Remtime2);
displayCursor1();
}
STM32_FlashWriteData();   //保存一下设置值
}
str1 = str1 + 1;
i = 0;
while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str1;
i ++; str1 ++;
if(*str1 == 't')break;         
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符

setValue = atoi(setvalue);
if(setValue>=0 && setValue<=59)
{
Remtime2.fen = setValue;   //吃药时间的小时
if(setn == 8)//设置吃药时间2
{
displaySetTime(&Remtime2);
displayCursor1();
}
STM32_FlashWriteData();   //保存一下设置值
}
}

if(strstr((char *)dataPtr,"three:")!=NULL)  //接收到设置吃药时间的指令
{
str1 = strstr((char *)dataPtr,"three:");

while(*str1 < '0' || *str1 > '9')        //判断是不是0到9有效数字
{
str1 = str1 + 1;
delay_ms(10);
}
i = 0;
while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str1;
i ++; str1 ++;
if(*str1 == ':')break;         
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=23)
{
Remtime3.shi = setValue;   //吃药时间的小时
if(setn == 9)//设置吃药时间3
{
displaySetTime(&Remtime3);
displayCursor1();
}
STM32_FlashWriteData();   //保存一下设置值
}
str1 = str1 + 1;
i = 0;
while(*str1 >= '0' && *str1 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str1;
i ++; str1 ++;
if(*str1 == ',')break;         
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符

setValue = atoi(setvalue);
if(setValue>=0 && setValue<=59)
{
Remtime3.fen = setValue;   //吃药时间的小时
if(setn == 9)//设置吃药时间3
{
displaySetTime(&Remtime3);
displayCursor1();
}
STM32_FlashWriteData();   //保存一下设置值
}
}

if(strstr((char *)dataPtr,"MedA")!=NULL)
{
str2 = strstr((char *)dataPtr,"MedA");

while(*str2 < '0' || *str2 > '9')        //判断是不是0到9有效数字
{
str2 = str2 + 1;
delay_ms(10);
}
i = 0;
while(*str2 >= '0' && *str2 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str2;
i ++; str2 ++;
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=99)
{
A = setValue;    //设置的药物数量
if(setn == 10)//设置吃药药量
{
displayMedicineNum();
displayCursor2();
}
STM32_FlashWriteData();   //保存一下设置值
}
}	
if(strstr((char *)dataPtr,"MedB")!=NULL)
{
str2 = strstr((char *)dataPtr,"MedB");

while(*str2 < '0' || *str2 > '9')        //判断是不是0到9有效数字
{
str2 = str2 + 1;
delay_ms(10);
}
i = 0;
while(*str2 >= '0' && *str2 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str2;
i ++; str2 ++;
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=99)
{
B = setValue;    //设置的药物数量
if(setn == 10)//设置吃药药量
{
displayMedicineNum();
displayCursor2();
}
STM32_FlashWriteData();   //保存一下设置值
}
}
if(strstr((char *)dataPtr,"MedC")!=NULL)
{
str2 = strstr((char *)dataPtr,"MedC");

while(*str2 < '0' || *str2 > '9')        //判断是不是0到9有效数字
{
str2 = str2 + 1;
delay_ms(10);
}
i = 0;
while(*str2 >= '0' && *str2 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str2;
i ++; str2 ++;
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=99)
{
C = setValue;    //设置的药物数量
if(setn == 10)//设置吃药药量
{
displayMedicineNum();
displayCursor2();
}
STM32_FlashWriteData();   //保存一下设置值
}
}
if(strstr((char *)dataPtr,"MedD")!=NULL)
{
str2 = strstr((char *)dataPtr,"MedD");

while(*str2 < '0' || *str2 > '9')        //判断是不是0到9有效数字
{
str2 = str2 + 1;
delay_ms(10);
}
i = 0;
while(*str2 >= '0' && *str2 <= '9')        //判断是不是0到9有效数字
{
setvalue[i] = *str2;
i ++; str2 ++;
delay_ms(10);
}
setvalue[i] = '\0';            //加上结尾符
setValue = atoi(setvalue);
if(setValue>=0 && setValue<=99)
{
D = setValue;    //设置的药物数量
if(setn == 10)//设置吃药药量
{
displayMedicineNum();
displayCursor2();
}
STM32_FlashWriteData();   //保存一下设置值
}
}
if(strstr((char *)dataPtr,"time1Switch_on")!=NULL)
{
Remtime1.state = 1;

if(setn == 7)//显示
{
displaySetTime(&Remtime1);
displayCursor1();
}
}
if(strstr((char *)dataPtr,"time1Switch_off")!=NULL)
{
Remtime1.state = 0;

if(setn == 7)//显示
{
displaySetTime(&Remtime1);
displayCursor1();
}
}
if(strstr((char *)dataPtr,"time2Switch_on")!=NULL)
{
Remtime2.state = 1;

if(setn == 8)//显示
{
displaySetTime(&Remtime2);
displayCursor1();
}
}
if(strstr((char *)dataPtr,"time2Switch_off")!=NULL)
{
Remtime2.state = 0;

if(setn == 8)//显示
{
displaySetTime(&Remtime2);
displayCursor1();
}
}
if(strstr((char *)dataPtr,"time3Switch_on")!=NULL)
{
Remtime3.state = 1;

if(setn == 9)//显示
{
displaySetTime(&Remtime3);
displayCursor1();
}
}
if(strstr((char *)dataPtr,"time3Switch_off")!=NULL)
{
Remtime3.state = 0;

if(setn == 9)//显示
{
displaySetTime(&Remtime3);
displayCursor1();
}
}
if(strstr((char *)dataPtr,"closeRemind")!=NULL)
{
tixing = 0;       //清除提醒标志
displayInitChar();
}
ESP8266_Clear();									//清空缓存
}
if(SendFlag==1)    //1秒钟上传一次数据
{
SendFlag = 0;		

memset(SEND_BUF,0,sizeof(SEND_BUF));   			//清空缓冲区
sprintf(SEND_BUF,"$temp:%d#,$humi:%d#,$weight:%d#",temp,humi,(int)weight);

if(SendSwitchState)  //当状态发生变化的时候，上报一次状态
{
if(Remtime1.state==1)strcat(SEND_BUF,",time1_on");else strcat(SEND_BUF,",time1_off");
if(Remtime2.state==1)strcat(SEND_BUF,",time2_on");else strcat(SEND_BUF,",time2_off");
if(Remtime3.state==1)strcat(SEND_BUF,",time3_on");else strcat(SEND_BUF,",time3_off");

SendSwitchState = 0;
}

if(tixing==1)strcat(SEND_BUF,",play"); else strcat(SEND_BUF,",stop");

ESP8266_SendData((u8 *)SEND_BUF, strlen(SEND_BUF));
ESP8266_Clear();
}
}

int main(void)
{	
unsigned char timeCount=0;

delay_init();	    //延时函数初始化	  
delay_ms(500);       //上电瞬间加入一定延时在初始化
LCD_Init();         //屏幕初始化
KEY_AND_BEEP_GPIO_Init();  //按键及蜂鸣器引脚初始化
HX711_GPIO_Init();     
To_Zero();
while(DHT11_Init()==1)
{
LCD_Write_String(0,0,"   DHT11 Init!  ");
LCD_Write_String(0,1,"                ");
}
LCD_Write_String(0,0,"   loading...   ");//显示字符串
LCD_Write_String(0,1,"                ");
ESP8266_Init();
To_Zero();
STM32_FlashCheck();       
To_Zero();
displayInitChar();
DS1302_Init(&SysDate);    //DS1302时钟初始化
To_Zero();
TIM3_Init(499,7199);   //定时器初始化，定时50ms
//Tout = ((arr+1)*(psc+1))/Tclk ; 
//Tclk:定时器输入频率(单位MHZ)
//Tout:定时器溢出时间(单位us)
while(1)
{  
keyscan();//按键扫描
if(setn == 0)
{
Get_Ds1302_Time();   //显示时间

//吃药时间到了
if((SysDate.hour==Remtime1.shi && SysDate.min==Remtime1.fen && SysDate.sec==0  && Remtime1.state==1)||
(SysDate.hour==Remtime2.shi && SysDate.min==Remtime2.fen && SysDate.sec==0 && Remtime2.state==1)||
(SysDate.hour==Remtime3.shi && SysDate.min==Remtime3.fen && SysDate.sec==0 && Remtime3.state==1)
)
{
if((tixing==0)&&(A!=0 || B!=0 || C!=0 || D!=0)) //必须要有吃药量，才能提醒
{
LCD_Write_String(0,0,"Anum:    Bnum:  ");
LCD_Write_String(0,1,"Cnum:    Dnum:  ");
tixing = 1;
}
}
if(HW == 0)  //红外遮挡，说明已经吃过药了
{
if(tixing==1)
{
tixing = 0;       //清除提醒标志
displayInitChar();
}
}
if(tixing == 0)
{
timeCount ++;
if(timeCount >= 10)
{
timeCount = 0;
weight = get_weight();   //获取重量
if(weight > 999)weight=999;
sprintf((char *)display,"%03dg",(int)weight);
if(weight == 0 && shanshuo)
{
LCD_Write_String(12,0,"    ");
}
else LCD_Write_String(12,0,display);   //显示重量
}
if(shuaxin == 1)
{
shuaxin = 0;
DHT11_Read_Data(&temp,&humi);     //读取温湿度
LCD_Write_Char(9,1,temp/10+'0'); 
LCD_Write_Char(10,1,temp%10+'0'); 
LCD_Write_Char(13,1,humi/10+'0'); 
LCD_Write_Char(14,1,humi%10+'0');   
}
}
else
{
displayMedicineNum();   //吃药时间到了，显示吃药的数量
}
}
UsartSendReceiveData();
delay_ms(10);
}	
}

void TIM3_IRQHandler(void)//定时器3中断服务程序，用于记录时间
{ 
static u16 timeCount1=0;
static u16 timeCount2=0;
static u16 timeCount3=0;

if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
{ 
TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //清除中断标志位  

timeCount1 ++;
if(timeCount1 >= 10)
{
timeCount1 = 0;
shanshuo = !shanshuo; 
shuaxin = 1; 
if(Remtime1.temporary!=Remtime1.state)
{
Remtime1.temporary = Remtime1.state;
SendSwitchState = 1;
}
if(Remtime2.temporary!=Remtime2.state)
{
Remtime2.temporary = Remtime2.state;
SendSwitchState = 1;
}
if(Remtime3.temporary!=Remtime3.state)
{
Remtime3.temporary = Remtime3.state;
SendSwitchState = 1;
}
}

timeCount2 ++;

if(tixing == 1)   //吃药时间到了，蜂鸣器提醒
{
if(timeCount2==3)BEEP=1;
if(timeCount2==6)BEEP=0; 
if(timeCount2==9)BEEP=1;
if(timeCount2==12)BEEP=0;
if(timeCount2==15)BEEP=1;
if(timeCount2==18)BEEP=0; 

if(timeCount2>=40)
{
BEEP=0;
timeCount2=0;
}
}
else
{
if(timeCount2 >= 2)
{
timeCount2 = 0;
if(weight == 0 && setn==0)   //重量是0，蜂鸣器响
{
BEEP = ~BEEP;
}
else
{
if(beepflag==0)BEEP = 0;
}
}
}

timeCount3 ++;
if(timeCount3 >= 20)
{
timeCount3 = 0;
SendFlag = 1;		
}
}
}
