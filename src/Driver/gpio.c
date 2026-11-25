#include "gpio.h"

//////////////////////////////////////////////////////////////////////////////////	 
//LED,按键的GPIO设置								  
////////////////////////////////////////////////////////////////////////////////// 	   

void KEY_AND_BEEP_GPIO_Init(void)
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
		
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);	 //使能PB端口时钟
	 GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);  //关闭JTAG模式 使PB3，PB4变成普通IO口
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //推挽输出
	 GPIO_Init(GPIOB, &GPIO_InitStructure);			
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOB, &GPIO_InitStructure);			
	 GPIO_ResetBits(GPIOB,GPIO_Pin_6);	//输出低电平
	
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;				 // 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //推挽输出
	 GPIO_Init(GPIOA, &GPIO_InitStructure);		
}



