#include "hx711.h"
#include "filter.h"
#include "delay.h"
#include "sys.h"

//定义量程系数
#define RATIO    0.00480    //这是一个修正系数，例如100g砝码称出来是238g，则RATIO为100/238=0.420。这里取0.00420是为了把重量缩小100倍;

//称重用变量
unsigned long FullScale; //满量程AD值/1000

void HX711_GPIO_Init(void)
{

 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能PB端口时钟
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 ;				 // 端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);		

 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;				 // 端口配置
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 GPIO_Init(GPIOB, &GPIO_InitStructure);			
}

unsigned long ReadCount(void)
{
   u32 Count = 0;
   u8 i;
   ADSK=0;
   while(ADDO);
   for (i=0;i<24;i++)
   {
      ADSK=1;
      Count=Count<<1;
		  delay_us(1);
      ADSK=0;
      if(ADDO) 
				Count++;
			delay_us(1);
   }
   ADSK=1;
   Count=Count^0x800000;
	 delay_us(1);
   ADSK=0;
	 delay_us(1);
   return(Count);
}

//重新找回零点，每次测量前调用
void To_Zero(void)
{
    FullScale=ReadCount();//读取HX711的AD值
}

unsigned long  get_weight(void)				    //获得称重
{
    volatile unsigned long cur_adc_value = 0;
    unsigned long weight;    //重量值，单位g
	
    cur_adc_value = ReadCount();//读取HX711的AD值

    if (cur_adc_value <= FullScale)
    {
        weight = 0;
    }
    else
    {
        cur_adc_value = cur_adc_value - FullScale;//获取净重
			
			  ///最终目的是把cur_adc_value最后两位取整
        if (cur_adc_value > 55)//55只是一个微动值的判定，因为HX711是一款24位高精度的AD芯片，一般很微小的值也能感应到
        {
            if (cur_adc_value % 100 >= 55)//余数大于55
            {
							   //////////把cur_adc_value值以55为基准进行取舍，假如cur_adc_value=257，则这里我们取整为300。再假如cur_adc_value=240，则我们取整200
                cur_adc_value = cur_adc_value + (100 - (cur_adc_value % 100));
            }
            else
            {
							   //////////把cur_adc_value值以55为基准进行取舍，假如cur_adc_value=89，则这里我们取整为100。再假如cur_adc_value=40，则我们取整0
                cur_adc_value = cur_adc_value - (cur_adc_value % 100);
            }
        }
        else
        {
            cur_adc_value = 0;
        }
        weight = cur_adc_value * RATIO;//乘上RATIO系数，得到正确的重量值
    }
    weight = weight_value_filter(weight);//重量值滤波,让重量值更稳定
		
		return(weight);
}
