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
#include "spi.h" 


void OtherIOInit(void);

const uint8_t TEXT_Buffer[]={"W25Q16 Write And Read Test OK"};
#define W25_SIZE sizeof(TEXT_Buffer)


const uint16_t NTCTemp1039[30]=
{
/*10℃-19℃*/	2174 ,2139 ,2104 ,2068 ,2032 ,1996 ,1960 ,1924 ,1887 ,1851 ,
/*20℃-29℃*/	1814 ,1778 ,1741 ,1705 ,1669 ,1633 ,1597 ,1562 ,1526 ,1491 ,
/*30℃-39℃*/	1457 ,1423 ,1389 ,1355 ,1322 ,1289 ,1257 ,1226 ,1194 ,1164 
};

#define ADValueMax 	0x7fffff	
#define ADVref 		12.75	//0.5*Vref/Gain->0.5*3266/128
#define Vref		3266
#define Scale		700 //0.1℃ AD比例
#define AD37Ref  	4191422


int main(void)
{
	uint8_t Tempi;
	uint32_t ADValue;
	uint32_t Calib25_AD=0x3fd092;
	uint32_t Calib34_AD=0x40a008;
	uint32_t Calib37_AD=0x40f274;
	uint32_t Calib42_AD=0x417114;
	uint32_t Diff25;
	uint32_t Diff34;
	uint32_t Diff37;
	uint32_t Compensate_AD;
	uint8_t SetTemp;
	uint8_t SetTempi;
	uint8_t SetTempj;
	uint32_t ADValueTemp=0;
	float Votage;
	uint16_t VotageTemp;
	uint16_t TobjTemp;
	uint16_t NTCADValue;
	uint8_t NTCTemp;
	uint16_t BATADValue;
	uint16_t NTCVotage;
	uint8_t Delay_Cnt;
	uint8_t datatemp[W25_SIZE];
	uint32_t FLASH_SIZE=2*1024*1024;
	
	uint8_t W25_SetDataTemp[30];
	uint32_t CalibAD[10];
	uint8_t CalibADStep;
	uint32_t CalibADRef;
	
	
	
	bool PowerKeyFalg;
	bool CalibFalg;
	bool KeyModeFirstFlag;
	
	OtherIOInit();
	SET_Power_H;
	delay_init();
	USART1_Init(9600);
	printf("Usart Test OK\r\n");
	
	ADC_Config();
	TIM_Config(59,47);		//0.1M计数频率，1us*60=60us
	
	
	Lcd_Init();
	LCD_Clear(BLACK);
	//显示buzzer图标
	POINT_COLOR=YELLOW;
	ShowImage(10,10,34,31,buzzer);
	//显示HOLD
	POINT_COLOR=WHITE;
	LCD_ShowChar(52,8,15,32,Charasc32,0);		//H
	LCD_ShowChar(72,8,16,32,Charasc32,0);		//O
	LCD_ShowChar(92,8,17,32,Charasc32,0);		//L
	LCD_ShowChar(112,8,18,32,Charasc32,0);		//D	
	
	
	//显示battery图标
	showbattery();
	//ShowImage(180,10,50,24,battery);

	//显示满电
	batteryupdate(1241);
	//显示实际电池电压
	BATADValue=Get_Adc_Average(ADC_Channel_1,4);
	printf("BATADValue=%d\n",BATADValue);
	batteryupdate(BATADValue);
	
	//显示大号 88.8℃
	POINT_COLOR=GREEN;
	LCD_ShowNum(0,56,88,128,Charasc128,2,0);		//88
	Draw_Circle(135,154,4);		//.
	Draw_Circle(135,154,3);		//.
	Draw_Circle(135,154,2);		//.
	Draw_Circle(135,154,1);		//.
	Draw_Circle(135,154,0);		//.
	LCD_ShowNum(140,56,8,128,Charasc128,1,0);	//8
	Draw_Circle(208,88,4);		//.
	Draw_Circle(208,88,3);		//.
	LCD_ShowNum(214,78,8,128,Charasc128,1,0);	//8
	
	//显示小号 88.8℃
	POINT_COLOR=RED;
	LCD_ShowNum(48,190,88,48,Charasc48,2,0);	//88
	Draw_Circle(102,225,3);		//.
	Draw_Circle(102,225,2);		//.
	Draw_Circle(102,225,1);		//.
	Draw_Circle(102,225,0);		//.
	LCD_ShowNum(108,190,8,48,Charasc48,1,0);	//8
	Draw_Circle(138,205,3);		//.
	Draw_Circle(138,205,2);		//.
	LCD_ShowChar(142,190,14,48,Charasc48,0);	//C
	//打开OLED背光
	GPIO_SetBits(GPIOA,LED_Pin);
	delay_ms(2000);
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
	LCD_ShowChar(210,70,14,48,Charasc48,0);	//C
	
	
	Cs1237IO_Init();
	Con_CS1237();
	Init_CS1237();
	Tempi=Read_CON();
	printf("Read_CON=%d,CS1237 Init OK\n",Tempi);
	
	
	W25QXX_Init();
	W25QXX_Write((uint8_t*)TEXT_Buffer,FLASH_SIZE-100,W25_SIZE);	//从倒数第100个地址处开始,写入SIZE长度的数据
	delay_ms(100);
	W25QXX_Read(datatemp,FLASH_SIZE-100,W25_SIZE);					//从倒数第100个地址处开始,读出SIZE个字节
	for(Tempi=0;Tempi<W25_SIZE-1;Tempi++)
	{
		printf("%c",datatemp[Tempi]);
		
	}
	printf("\n");
	
//擦出标定标志位，恢复默认值
//	W25_DataTemp[0]=0xaa;				
//	W25QXX_Write(W25_DataTemp,0,1);

	W25QXX_Read(W25_SetDataTemp,0,10);
	if(W25_SetDataTemp[0]==0x55)
	{
		printf("W25q16 Byte0=0x%x Thermometer Has Already Calib\n",W25_SetDataTemp[0]);
		//Compensate_AD校正补偿值计算：
		Compensate_AD=W25_SetDataTemp[1]*Scale;
		printf("Compensate_AD=%d\n",Compensate_AD);
	}
	else
	{
		printf("W25q16 Byte0=0x%x Thermometer Has Not Calib\n",W25_SetDataTemp[0]);
		//Compensate_AD校正补偿值计算：W25_SetDataTemp[1]是1分辨率，实际是0.1分辨率，所以除以10
		Compensate_AD=0;
		printf("Compensate_AD=%d\n",Compensate_AD);
 	}
	
	PowerKeyFalg=false;
	CalibFalg=false;
	
	while(1)
	{
		delay_ms(100);
		if(ReadKey==0)
		{
			delay_ms(50);
			if(ReadKey==0)
			{
				BuzzerCount=3;
				PoweroffCount=0;
				PowerKeyFalg=true;
				
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
				
				
				ADValue=0x00;
				ADValue=Read_CS1237();
				
				if((ADValue & 0x800000) !=0)							//负值
				{
					ADValue &= 0x007fffff;
					ADValueTemp=Code32bit_conversion(ADValue);
					ADValue=ADValueTemp;
					ADValue=0x400000-ADValue;
				}
				else													//正值
				{
					ADValue=0x400000+ADValue;
				}
				printf("-------Measure_Data-------\n");
				printf("Measure_AD_1=%d\n",ADValue);
				
				if(W25_SetDataTemp[2]>0)	//正值补偿
				{
					ADValue+=Compensate_AD;
				}	
				else						//负值补偿
				{
					ADValue-=Compensate_AD;
				}
				
				printf("Measure_AD_2=%d\n",ADValue);
				if(ADValue>=AD37Ref)															
				{
					TobjTemp=370;
					TobjTemp += (ADValue-AD37Ref)/Scale;
				}
				else
				{
					TobjTemp=370;
					TobjTemp -= (AD37Ref-ADValue)/Scale;
				}
				
				
				if(TobjTemp>420)									// Tobj>42℃ 显示HI						
				{
					LCD_Clearpart(0,56,240,184,BLACK);
					POINT_COLOR=RED;
					LCD_ShowChar(51,56,10,128,Charasc128,0);	
					LCD_ShowChar(125,56,11,128,Charasc128,0);	
				}
				else if(TobjTemp<320)								// Tobj<25℃ 显示LO										
				{
					LCD_Clearpart(0,56,240,184,BLACK);					
					POINT_COLOR=RED;
					LCD_ShowChar(51,56,12,128,Charasc128,0);	
					LCD_ShowChar(125,56,13,128,Charasc128,0);	
				}
				else												//32<=Tobj<=42℃			
				{
					tempupdate(TobjTemp,NTCTemp);
				}
			}
		}
		else if(KeyMode==0 && CalibFalg==false )
		{
			if(KeyModeFirstFlag==true)
			{
				KeyModeFirstFlag=false;
				KeyModeCount=0;
			}
			if(KeyModeCount>30)
			{
				KeyModeCount=0;
				BuzzerCount=3;
				CalibFalg=true;
				POINT_COLOR=GREEN;	
//———————————————————————设置微调±0.1℃——————————————————————————	
				LCD_Clearpart(0,56,240,184,BLACK);
				//显示F1:±0.1℃
				LCD_ShowChar(16,56,26,32,Charasc32,0);	//F
				LCD_ShowChar(36,70,1,16,Charasc16,0);	//1
				LCD_ShowChar(48,56,27,32,Charasc32,0);	//：
				ShowChinese(64,56,32,32,Chinese32,0);	//±			
				LCD_ShowChar(96,56,0,32,Charasc32,0);	//0	
				Draw_Circle(116,80,0);					//.1
				Draw_Circle(116,80,1);					//.
				LCD_ShowChar(120,56,1,32,Charasc32,0);	//1
				Draw_Circle(140,64,0);					//.
				Draw_Circle(140,64,1);					//.
				LCD_ShowChar(144,56,14,32,Charasc32,0);	//C
				
				                                                                                                                                                                                 
				/*
				W25_SetDataTemp[0] //是否校正标志位
				W25_SetDataTemp[1] //微调数
				W25_SetDataTemp[2] //微调正负标志位
				W25_SetDataTemp[3] //F/C标志位
				W25_SetDataTemp[4] //Buzzer ON/OFF标志位
				W25_SetDataTemp[5] //Body/Obj标志位 
				W25_SetDataTemp[5] 
				W25_SetDataTemp[6] 
				W25_SetDataTemp[7] 
				W25_SetDataTemp[8] 
				W25_SetDataTemp[9] 
				W25_SetDataTemp[10] 
				W25_SetDataTemp[11] 
				*/
				//显示--.-℃
				POINT_COLOR=WHITE;
				LCD_Fill(20,150,60,158,WHITE);			//-
				LCD_Fill(70,150,110,158,WHITE);			//-
				Draw_Circle(121,164,0);					//.
				Draw_Circle(121,164,1);					//.
				Draw_Circle(121,164,2);					//.
				LCD_Fill(130,150,170,158,WHITE);		//-	
				Draw_Circle(182,122,3);					//.
				Draw_Circle(182,122,2);					//.
				LCD_ShowChar(190,116,14,48,Charasc48,0);//C
				delay_ms(1000);
				CalibADStep=0;
				while(KeyMode==0)
				{
					
				}
				while(1)
				{
					if(ReadKey==0)
					{
						delay_ms(50);
						W25QXX_Read(W25_SetDataTemp,0,10);
						SetTempi=W25_SetDataTemp[1]/10;
						SetTempj=W25_SetDataTemp[1]%10;
						CalibADStep++;
						if(ReadKey==0)
						{
							BuzzerCount=3;
						}
						ADValue=0x00;
						ADValue=Read_CS1237();
						if((ADValue & 0x800000) !=0)							//负值
						{
							ADValue &= 0x007fffff;
							ADValueTemp=Code32bit_conversion(ADValue);
							ADValue=ADValueTemp;
							ADValue=0x400000-ADValue;
						}
						else													//正值
						{
							ADValue=0x400000+ADValue;
						}
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
						
						
						CalibAD[CalibADStep]=ADValue;
						if(CalibAD[CalibADStep]>=AD37Ref)
						{
							TobjTemp=370;
							TobjTemp+=(CalibAD[CalibADStep]-AD37Ref)/Scale;
						}
						else
						{
							TobjTemp=370;
							TobjTemp-=(AD37Ref-CalibAD[CalibADStep])/Scale;
						}
						tempupdatecalib(TobjTemp,NTCTemp);
						printf("CalibADNTCTemp=%d\n",NTCTemp);
						printf("CalibAD%d=%d\n",CalibADStep,CalibAD[CalibADStep]);
						delay_ms(1000);
						delay_ms(1000);
						POINT_COLOR=WHITE;
						
						LCD_Clearpart(0,96,240,240,BLACK);
						
						LCD_Fill(20,150,60,158,WHITE);			//-
						LCD_Fill(70,150,110,158,WHITE);			//-
						Draw_Circle(121,164,0);					//.
						Draw_Circle(121,164,1);					//.
						Draw_Circle(121,164,2);					//.
						LCD_Fill(130,150,170,158,WHITE);		//-	
						Draw_Circle(182,122,3);					//.
						Draw_Circle(182,122,2);					//.
						LCD_ShowChar(190,116,14,48,Charasc48,0);	//C
					}
					if(CalibADStep>2)
					{
						if(CalibAD[0]>=CalibAD[1])
						{
							CalibAD[3]=CalibAD[0]-CalibAD[1];
						}
						else
						{
							CalibAD[3]=CalibAD[1]-CalibAD[0];
						}
						
						if(CalibAD[0]>=CalibAD[2])
						{
							CalibAD[4]=CalibAD[0]-CalibAD[2];
						}
						else
						{
							CalibAD[4]=CalibAD[2]-CalibAD[0];
						}
						
						if(CalibAD[1]>=CalibAD[2])
						{
							CalibAD[5]=CalibAD[1]-CalibAD[2];
						}
						else
						{
							CalibAD[5]=CalibAD[2]-CalibAD[1];
						}
						if(CalibAD[3]<=2100 || CalibAD[4]<=2100 || CalibAD[5]<=2100)
						{
							W25_SetDataTemp[2]=1;
							SetTemp=0;
							SetTempi=0;
							SetTempj=0;
							CalibADRef=(CalibAD[0]+CalibAD[1]+CalibAD[2])/3;
							LCD_Clearpart(0,100,240,170,BLACK);
							break;
						}
						else
						{
							CalibADStep=0;
						}
					}
				}
				
				
				//显示±0.1℃
				if(W25_SetDataTemp[2]>0)
				{
					LCD_ShowChar(28,120,11,64,Charasc64,0);		//+		
				}
				else
				{
					LCD_ShowChar(28,120,12,64,Charasc64,0);		//-	
				}
				LCD_ShowNum(64,120,0,64,Charasc64,1,0);			//0
				Draw_Circle(104,168,0);							//.
				Draw_Circle(104,168,1);							//.
				Draw_Circle(104,168,2);							//.
				Draw_Circle(104,168,3);							//.
				LCD_ShowNum(112,120,0,64,Charasc64,1,0);		//SetTempj
				Draw_Circle(148,134,0);							//.
				Draw_Circle(148,134,1);							//.
				Draw_Circle(148,134,2);							//.
				Draw_Circle(148,134,3);							//.
				LCD_ShowChar(152,120,10,64,Charasc64,0);		//C
				
				while(1)
				{
					if(Keyup==0)
					{
						delay_ms(200);
						if(W25_SetDataTemp[2]>0)
						{
							SetTemp++;
							if(SetTemp>99)
							{
								SetTemp=99;
							}
						}
						else
						{
							SetTemp--;
							if(SetTemp==0)
							{
								W25_SetDataTemp[2]=1;
							}
						}
						SetTempi=SetTemp/10;
						SetTempj=SetTemp%10;
						LCD_Clearpart(64,120,64+32,120+64,BLACK);
						LCD_Clearpart(112,120,112+32,120+64,BLACK);
						LCD_Clearpart(32,120,32+32,120+64,BLACK);
						if(W25_SetDataTemp[2]>0)
						{
							LCD_ShowChar(28,120,11,64,Charasc64,0);	//+		
						}
						else
						{
							LCD_ShowChar(28,120,12,64,Charasc64,0);	//-	
						}
						LCD_ShowNum(64,120,SetTempi,64,Charasc64,1,0);	//SetTempi
						LCD_ShowNum(112,120,SetTempj,64,Charasc64,1,0);	//SetTempj
					}
					else if(Keydown==0)
					{

						delay_ms(200);
						if(W25_SetDataTemp[2]>0)
						{
							SetTemp--;
							if(SetTemp==0)
							{
								W25_SetDataTemp[2]=0;
								
							}
						}
						else
						{
							SetTemp++;
							if(SetTemp>99)
							{
								SetTemp=99;
							}
						}
						SetTempi=SetTemp/10;
						SetTempj=SetTemp%10;
						LCD_Clearpart(64,120,64+32,120+64,BLACK);
						LCD_Clearpart(112,120,112+32,120+64,BLACK);
						LCD_Clearpart(32,120,32+32,120+64,BLACK);
						if(W25_SetDataTemp[2]>0)
						{
							LCD_ShowChar(28,120,11,64,Charasc64,0);	//+		
						}
						else
						{
							LCD_ShowChar(28,120,12,64,Charasc64,0);	//-	
						}
						LCD_ShowNum(64,120,SetTempi,64,Charasc64,1,0);	//SetTempi
						LCD_ShowNum(112,120,SetTempj,64,Charasc64,1,0);	//SetTempj
					}
					else if(KeyMode==0)
					{
						delay_ms(500);
						
						while(KeyMode==0)
						{
							
						}
						W25_SetDataTemp[1]=SetTempi*10+SetTempj;
							
						break;
					}
				}
//———————————————————————设置F/℃———————————————————————————	
				LCD_Clearpart(0,56,240,184,BLACK);
				//显示F2:F/℃
				LCD_ShowChar(16,56,26,32,Charasc32,0);	//F
				LCD_ShowChar(36,70,2,16,Charasc16,0);	//1
				LCD_ShowChar(48,56,27,32,Charasc32,0);	//：
				LCD_ShowChar(64,56,26,32,Charasc32,0);	//F	
				LCD_ShowChar(80,56,33,32,Charasc32,0);	///
				Draw_Circle(100,64,0);					//.
				Draw_Circle(100,64,1);					//.
				LCD_ShowChar(104,56,14,32,Charasc32,0);	//C
				//显示F或者℃
				if(W25_SetDataTemp[3]>0)
				{
					LCD_ShowChar(104,120,13,64,Charasc64,0);//F
				}
				else
				{
					Draw_Circle(100,134,0);					//.
					Draw_Circle(100,134,1);					//.
					Draw_Circle(100,134,2);					//.
					Draw_Circle(100,134,3);					//.
					LCD_ShowChar(104,120,10,64,Charasc64,0);//C
				}
				while(1)
				{
					if(Keyup==0||Keydown==0)
					{
						delay_ms(300);
						while(Keyup==0||Keydown==0)
						{
							
						}
						if(W25_SetDataTemp[3]>0)
						{
							W25_SetDataTemp[3]=0;
							LCD_Clearpart(100,120,240,184,BLACK);
							Draw_Circle(100,134,0);					//.
							Draw_Circle(100,134,1);					//.
							Draw_Circle(100,134,2);					//.
							Draw_Circle(100,134,3);					//.
							LCD_ShowChar(104,120,10,64,Charasc64,0);//C
						}
						else
						{
							W25_SetDataTemp[3]=1;
							LCD_Clearpart(100,120,240,184,BLACK);
							LCD_ShowChar(104,120,13,64,Charasc64,0);//F
						}
					}
					else if(KeyMode==0)
					{
						delay_ms(500);
						while(KeyMode==0)
						{
							
						}
						break;
					}
				}
//———————————————————————设置buzzer ON/OFF———————————————————————————				
				LCD_Clearpart(0,56,240,184,BLACK);
				//显示Buzzer
				LCD_ShowChar(16,56,26,32,Charasc32,0);	//F
				LCD_ShowChar(36,70,3,16,Charasc16,0);	//3
				LCD_ShowChar(48,56,27,32,Charasc32,0);	//：
				
				ShowImage(68,60,34,31,buzzer);

				//显示ON或者OFF
				if(W25_SetDataTemp[4]>0)
				{
					LCD_ShowChar(72,120,15,64,Charasc64,0);		//ON
					LCD_ShowChar(104,120,16,64,Charasc64,0);	//ON
				}
				else
				{
					LCD_ShowChar(72,120,15,64,Charasc64,0);		//OFF
					LCD_ShowChar(104,120,17,64,Charasc64,0);	//OFF
					LCD_ShowChar(136,120,17,64,Charasc64,0);	//OFF
				}
				while(1)
				{
					if(Keyup==0||Keydown==0)
					{
						delay_ms(300);
						while(Keyup==0||Keydown==0)
						{
							
						}
						if(W25_SetDataTemp[4]>0)
						{
							W25_SetDataTemp[4]=0;
							LCD_Clearpart(72,120,240,184,BLACK);
							LCD_ShowChar(72,120,15,64,Charasc64,0);		//OFF
							LCD_ShowChar(104,120,17,64,Charasc64,0);	//OFF
							LCD_ShowChar(136,120,17,64,Charasc64,0);	//OFF
						}
						else
						{
							
							W25_SetDataTemp[4]=1;
							LCD_Clearpart(72,120,240,184,BLACK);
							LCD_ShowChar(72,120,15,64,Charasc64,0);		//ON
							LCD_ShowChar(104,120,16,64,Charasc64,0);	//ON
						}
					}
					else if(KeyMode==0)
					{
						delay_ms(500);
						while(KeyMode==0)
						{
							
						}
						break;
					}
				}
//———————————————————————设置Body/Obj———————————————————————————	
				LCD_Clearpart(0,56,240,184,BLACK);
				//显示Body/Obj
				LCD_ShowChar(16,56,26,32,Charasc32,0);	//F
				LCD_ShowChar(36,70,4,16,Charasc16,0);	//4
				LCD_ShowChar(48,56,27,32,Charasc32,0);	//：
				LCD_ShowChar(64,56,29,32,Charasc32,0);	//Body
				LCD_ShowChar(80,56,30,32,Charasc32,0);	//Body
				LCD_ShowChar(96,56,31,32,Charasc32,0);	//Body
				LCD_ShowChar(112,56,32,32,Charasc32,0);	//Body
				LCD_ShowChar(128,56,33,32,Charasc32,0);	///
				LCD_ShowChar(144,56,34,32,Charasc32,0);	//Obj
				LCD_ShowChar(160,56,35,32,Charasc32,0);	//Obj
				LCD_ShowChar(176,56,36,32,Charasc32,0);	//Obj
				
				
				//显示Body或者Obj
				if(W25_SetDataTemp[5]>0)
				{
					LCD_ShowChar(72,120,18,64,Charasc64,0);		//Body
					LCD_ShowChar(104,120,19,64,Charasc64,0);	//Body
					LCD_ShowChar(136,120,20,64,Charasc64,0);	//Body
					LCD_ShowChar(168,120,21,64,Charasc64,0);	//Body
				}
				else
				{
					LCD_ShowChar(72,120,15,64,Charasc64,0);		//Obj
					LCD_ShowChar(104,120,22,64,Charasc64,0);	//Obj
					LCD_ShowChar(136,120,23,64,Charasc64,0);	//Obj
				}
				while(1)
				{
					if(Keyup==0||Keydown==0)
					{
						delay_ms(300);
						while(Keyup==0||Keydown==0)
						{
							
						}
						if(W25_SetDataTemp[5]>0)
						{
							W25_SetDataTemp[5]=0;
							LCD_Clearpart(72,120,240,184,BLACK);
							LCD_ShowChar(72,120,15,64,Charasc64,0);		//Obj
							LCD_ShowChar(104,120,22,64,Charasc64,0);	//Obj
							LCD_ShowChar(136,120,23,64,Charasc64,0);	//Obj
						}
						else
						{
							W25_SetDataTemp[5]=1;
							LCD_Clearpart(72,120,240,184,BLACK);
							LCD_ShowChar(72,120,18,64,Charasc64,0);		//Body
							LCD_ShowChar(104,120,19,64,Charasc64,0);	//Body
							LCD_ShowChar(136,120,20,64,Charasc64,0);	//Body
							LCD_ShowChar(168,120,21,64,Charasc64,0);	//Body
						}
					}
					else if(KeyMode==0)
					{
						delay_ms(500);
						while(KeyMode==0)
						{
							
						}
						break;
					}
				}
//———————————————————————设置Save———————————————————————————	
				LCD_Clearpart(0,56,240,184,BLACK);
				
				//显示Body或者Obj
				LCD_ShowChar(62,100,24,64,Charasc64,0);		//Save
				LCD_ShowChar(94,100,25,64,Charasc64,0);		//Save
				LCD_ShowChar(126,100,26,64,Charasc64,0);	//Save
				LCD_ShowChar(158,100,27,64,Charasc64,0);	//Save
				
				W25_SetDataTemp[0]=0x55;
				W25QXX_Write(W25_SetDataTemp,0,10);
				CalibFalg=false;
				delay_ms(2000);
				SET_Power_L;
			}
		}
		else
		{
			KeyModeFirstFlag=true;
			KeyModeCount=0;
			//显示温度持续时间30*100ms=3s
			if(PowerKeyFalg==true && PoweroffCount>30 )
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
				LCD_ShowChar(210,70,14,48,Charasc48,0);	//C
			}
			//无按键关机时间 150*	100ms=15s
			else if(PoweroffCount>150)
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
	
	RCC_AHBPeriphClockCmd(	RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOF, ENABLE );	
	GPIO_InitStructure.GPIO_Pin = POWER_Pin;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  			
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEYRead|KEYUp;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEYMode|KEYDown;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; 
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	
	
	GPIO_InitStructure.GPIO_Pin = Buzzer_Pin;  				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  			
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,Buzzer_Pin);
	
}








