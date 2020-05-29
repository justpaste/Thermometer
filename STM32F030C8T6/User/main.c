#include "stm32f0xx.h"
#include "delay.h"
#include "usart1.h"
#include "stm32f0xx_gpio.h"
#include "adc.h"
#include "stm32f0xx_adc.h"
#include "tim3.h"
#include "cs1237.h"
#include "w25qxx.h" 
#include "oled.h" 
#include "stdbool.h" 
//#include "st7789v.h" 


void OtherIOInit(void);


#define FLASH_SIZE 16*1024*1024

uint8_t TEXT_Buffer[]={"w25q16 test"};
const uint16_t NTCTemp1039[30]=
{
/*10℃-19℃*/	2174 ,2139 ,2104 ,2068 ,2032 ,1996 ,1960 ,1924 ,1887 ,1851 ,
/*20℃-29℃*/	1814 ,1778 ,1741 ,1705 ,1669 ,1633 ,1597 ,1562 ,1526 ,1491 ,
/*30℃-39℃*/	1457 ,1423 ,1389 ,1355 ,1322 ,1289 ,1257 ,1226 ,1194 ,1164 
};

const uint32_t Tobj18[111]=
{
	
//32.0-43.0 
43423,44081,44739,45397,46713,48029,48687,49345,50003,50661,
51318,51976,52634,53292,53950,55266,55924,56582,57240,57898,
58556,59214,59872,60529,61187,61845,62503,63161,63819,65135,
65793,66451,67109,67767,68425,69083,69740,70398,71056,71714,
72372,73030,73688,74346,75004,75662,76320,76978,77636,78294,
78951,79609,80267,80925,81583,82241,82899,83557,84215,84873,
85531,86189,86847,87505,88162,88820,89478,90136,90794,91452,
92110,92768,93426,94084,94742,95400,96058,96716,97373,98031,
98689,99347,100005,100663,101321,101979,102637,103295,103953,104611,
105269,105927,106584,107242,107900,108558,109216,109874,110532,111190,
111848,112506,113164,113822,114480,115138,115796,116453,117111,117769,
118427 

	
					
	
};

#define ADValueMax 	0x7fffff	
#define ADVref 		12.75	//0.5*Vref/Gain->0.5*3266/128
#define Vref		3266



int main(void)
{
	uint8_t Tempi;
	uint32_t ADValue;
	uint32_t ADValueTemp=0;
	float Votage;
	uint16_t VotageTemp;
	uint16_t TobjTemp;
	uint16_t NTCADValue;
	uint8_t NTCTemp;
	uint16_t BATADValue;
	uint16_t NTCVotage;
	bool PowerKeyFalg;
	
	OtherIOInit();
	SET_Power_H;
	delay_init();
	delay_ms(200);
	USART1_Init(9600);
	printf("Usart Test OK\r\n");
	ADC_Config();
	TIM_Config(59,47);		//0.1M计数频率，1us*60=50us
	Cs1237IO_Init();
//	W25QXX_Init();
	

	Lcd_Init();
	LCD_Clear(BLACK);
	//显示buzzer图标
	POINT_COLOR=YELLOW;
	showbuzzer();
	//显示HOLD
	POINT_COLOR=WHITE;
	LCD_ShowChar44(52,8,0,32,0);		//H
	LCD_ShowChar44(72,8,1,32,0);		//O
	LCD_ShowChar44(92,8,2,32,0);		//L
	LCD_ShowChar44(112,8,3,32,0);		//D
	//显示battery图标
	showbattery();
	//显示满电
	batteryupdate(1241);
	//显示实际电池电压
	BATADValue=Get_Adc_Average(ADC_Channel_1,4);
	printf("BATADValue=%d\n",BATADValue);
	batteryupdate(BATADValue);
	
	//显示大号 88.8℃
	POINT_COLOR=GREEN;
	LCD_ShowNum22(0,56,88,128,2);	//88
	Draw_Circle(135,154,4);		//.
	Draw_Circle(135,154,3);		//.
	Draw_Circle(135,154,2);		//.
	Draw_Circle(135,154,1);		//.
	Draw_Circle(135,154,0);		//.
	LCD_ShowNum22(140,56,8,128,1);	//8
	Draw_Circle(208,88,4);		//.
	Draw_Circle(208,88,3);		//.
	LCD_ShowChar33(214,78,0,48,0);		//C
	
	//显示小号 88.8℃
	POINT_COLOR=RED;
	LCD_ShowNum33(48,190,55,48,2);	//88
	Draw_Circle(102,225,3);		//.
	Draw_Circle(102,225,2);		//.
	Draw_Circle(102,225,1);		//.
	Draw_Circle(102,225,0);		//.
	LCD_ShowNum33(108,190,5,48,1);		//8
	Draw_Circle(138,205,3);		//.
	Draw_Circle(138,205,2);		//.
	LCD_ShowChar33(142,190,0,48,0);		//C
	//打开OLED背光
	GPIO_SetBits(GPIOA,LED_Pin);
	delay_ms(1000);
	//清除小号 88.8℃
	LCD_Clearpart(0,190,240,240,BLACK);
	//清除大号 88.8℃
	LCD_Clearpart(0,56,240,184,BLACK);
	//显示大号 --.-℃
	POINT_COLOR=WHITE;
	LCD_Fill(10,130,60,142,WHITE);			//-
	LCD_Fill(70,130,120,142,WHITE);			//-
	Draw_Circle(137,152,0);					//.
	Draw_Circle(137,152,1);					//.
	Draw_Circle(137,152,2);					//.
	Draw_Circle(137,152,3);					//.
	Draw_Circle(137,152,4);					//.
	Draw_Circle(137,152,5);					//.
	LCD_Fill(150,130,200,142,WHITE);
	Draw_Circle(205,70,4);					//.
	Draw_Circle(205,70,3);					//.
	LCD_ShowChar33(210,70,0,48,0);			//C
	
	
//	showimage();
//	W25QXX_Write((uint8_t*)TEXT_Buffer,FLASH_SIZE-100,SIZE);	//从倒数第100个地址处开始,写入SIZE长度的数据
//	delay_ms(100);
//	W25QXX_Read(datatemp,FLASH_SIZE-100,SIZE);					//从倒数第100个地址处开始,读出SIZE个字节
//	printf("datatemp=%s\n",datatemp);
	

	PowerKeyFalg=false;
	Con_CS1237();
	Init_CS1237();
	Tempi=Read_CON();
	printf("Read_CON=%d\n",Tempi);
	while(1)
	{
		delay_ms(2000);
		printf("--------------------\n");
		if(ReadKey==0)
		{
			delay_ms(50);
			if(ReadKey==0)
			{
				PoweroffCount=0;
				PowerKeyFalg=true;
				ADValue=Read_CS1237();
				printf("ADValue=%d\n",ADValue);
				if((ADValue & 0x800000) !=0)
				{
					ADValue &= 0x007fffff;
//					printf("ADValue=%d\n",ADValue);
					ADValueTemp=Code32bit_conversion(ADValue);
					printf("ADValueTemp=%d\n",ADValueTemp);
					ADValue=ADValueTemp;
					ADValue=0x400000-ADValue;
				}
				else
				{
					printf("ADValue=%d\n",ADValue);
					ADValue=0x400000+ADValue;
				}
				
//				Read_CS1237数据是24位有符号数据，
//				Bit24=0表示正，1表示负
//				Votage=((float)ADValue/(float)ADValueMax)*ADVref;
//				Votage=Votage*1000;				//实际输入电压(uV)
//				printf("Votage=%d\n",(uint16_t)Votage);
				printf("ADValue=%x\n",ADValue);
				
//				for(Tempi=0;Tempi<111;Tempi++)
//				{
//					if(ADValue<Tobj18[Tempi])
//					{
//						break;
//					}
//				}
//				TobjTemp=Tempi+319;
//				printf("TobjTemp=%d\n",TobjTemp);
//				if(Tempi==0)								//显示LO
//				{
//					BuzzerCount=3;
//					LCD_Clearpart(0,56,240,184,BLACK);
//					POINT_COLOR=RED;
//					LCD_ShowChar22(51,56,13,128,0);		//L
//					LCD_ShowChar22(125,56,14,128,0);	//O
//				}
//				else if(Tempi>=111 || Votage>1000)		//显示HI
//				{
//					BuzzerCount=3;
//					LCD_Clearpart(0,56,240,184,BLACK);
//					POINT_COLOR=RED;
//					LCD_ShowChar22(51,56,11,128,0);		//H
//					LCD_ShowChar22(125,56,12,128,0);	//I
//				}
//				else
//				{
//					tempupdate(TobjTemp);
//				}
//				tempupdate(1234);
				
				//检测NTC温度
				NTCADValue=Get_Adc_Average(ADC_Channel_2,4);
				NTCVotage=NTCADValue*Vref/4096;
				for(Tempi=0;Tempi<30;Tempi++)
				{
					if(NTCVotage>NTCTemp1039[Tempi])
					{
						break;
					}
				}
				NTCTemp=Tempi+10-1;
				printf("NTCTemp=%d\n",NTCTemp);
			}
		}
		else
		{
			if(PowerKeyFalg==true && PoweroffCount>20 )
			{
				PoweroffCount=0;
				PowerKeyFalg=false;
				LCD_Clearpart(0,56,240,184,BLACK);
				//显示大号 88.8℃
				POINT_COLOR=WHITE;
				LCD_Fill(10,130,60,142,WHITE);			//-
				LCD_Fill(70,130,120,142,WHITE);			//-
				Draw_Circle(137,152,0);					//.
				Draw_Circle(137,152,1);					//.
				Draw_Circle(137,152,2);					//.
				Draw_Circle(137,152,3);					//.
				Draw_Circle(137,152,4);					//.
				Draw_Circle(137,152,5);					//.
				LCD_Fill(150,130,200,142,WHITE);
				Draw_Circle(205,70,4);					//.
				Draw_Circle(205,70,3);					//.
				LCD_ShowChar33(210,70,0,48,1);			//C
			}
			else if(PoweroffCount>200)
			{
				PoweroffCount=0;
				SET_Power_L;
			}
		}
		
	}
	
}

void OtherIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHBPeriphClockCmd(	RCC_AHBPeriph_GPIOA, ENABLE );	
	GPIO_InitStructure.GPIO_Pin = POWER_Pin;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  			
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEY_Pin;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;  			
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = Buzzer_Pin;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  			
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,Buzzer_Pin);
}








