#ifndef _HX711_H_
#define _HX711_H_ 

#define ADSK   PBout(13)	
#define ADDO   PBin(12)	

void HX711_GPIO_Init(void);
unsigned long ReadCount(void);
void To_Zero(void);
unsigned long  get_weight(void);

#endif

