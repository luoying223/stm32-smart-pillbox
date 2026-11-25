#ifndef __GPIO_H
#define __GPIO_H	 
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 


#define KEY1 PBin(3)
#define KEY2 PBin(4)
#define KEY3 PBin(5)

#define BEEP PBout(6)

#define HW PAin(12)

void KEY_AND_BEEP_GPIO_Init(void);//≥ı ºªØ

#endif
