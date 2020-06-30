#include "stm32f0xx.h"
#include "delay.h"
#include "usart1.h"
#include "stm32f0xx_gpio.h"
#include "adc.h"
#include "stm32f0xx_adc.h"
#include "tim3.h"
#include "cs1237.h"
#include "w25qxx.h" 
#include "oled.h" -



#include "stdbool.h" 
//#include "st7789v.h" 
#include "spi.h" 

//标定温度25的AD，
void OtherIOInit(void);
void ShowPass(void);
void ShowF25(uint8_t Ftemp);


const uint8_t TEXT_Buffer[]={"W25Q16 Write And Read Test OK"};
#define W25_SIZE sizeof(TEXT_Buffer)

uint16_t ColorTable1[7]={WHITE,GREEN,PURPLE,BLUE,YELLOW,RED,BRRED};

uint16_t ColorTable2[7]={LGREEN,PURPLE,LBLUE,LWHITE,BRRED,BLUE,GREEN};


const uint16_t NTCTemp1039[30]=
{
/*10℃-19℃*/	2174 ,2139 ,2104 ,2068 ,2032 ,1996 ,1960 ,1924 ,1887 ,1851 ,
/*20℃-29℃*/	1814 ,1778 ,1741 ,1705 ,1669 ,1633 ,1597 ,1562 ,1526 ,1491 ,
/*30℃-39℃*/	1457 ,1423 ,1389 ,1355 ,1322 ,1289 ,1257 ,1226 ,1194 ,1164 
};

#define ADValueMax 			0x7fffff	
#define ADVref 				12.75		//0.5*Vref/Gain->0.5*3266/128
#define Vref				3266
#define ObjtoBody			0.3			//物温模式转体温模式系数

int main(void)
{
	uint8_t Tempi;
	uint32_t ADValue;
	uint32_t Calib25_AD;
	uint32_t Calib34_AD;
	uint32_t Calib37_AD;
	uint32_t Calib42_AD;
	uint32_t Diff25;
	uint32_t Diff34;
	uint32_t Diff37;
	uint8_t NTCTemp;
	uint8_t SetTemp;
	uint8_t SetTempi;
	uint8_t SetTempj;
	uint32_t ADValueTemp=0;
	uint16_t TobjTemp;
	uint16_t NTCVotage;
	uint8_t Delay_Cnt;
	uint32_t FLASH_SIZE=2*1024*1024;
	
	uint8_t datatemp[W25_SIZE];
	uint8_t W25_SetDataTemp[10];
	uint8_t W25_CalibData[20];
	uint8_t ColorNum;
	
	uint16_t NTC25AD;
	uint16_t NTCMeasureAD;
	uint16_t BATADValue;
	
	uint16_t NTCAD_Compensate;
	
	bool PowerKeyFalg;
	bool SetFalg;
	bool KeyModeFirstFlag;
	bool CalibFalg;
	bool KeyupFirstFlag;
	
	OtherIOInit();
	SET_Power_H;
	delay_init();
	USART1_Init(9600);
	printf("Usart Test OK\r\n");
	
	ADC_Config();
	TIM_Config(59,47);		//0.1M计数频率，1us*60=60us
	
	W25QXX_Init();
	W25QXX_Write((uint8_t*)TEXT_Buffer,FLASH_SIZE-100,W25_SIZE);	//从倒数第100个地址处开始,写入SIZE长度的数据
	delay_ms(100);
	W25QXX_Read(datatemp,FLASH_SIZE-100,W25_SIZE);					//
	for(Tempi=0;Tempi<W25_SIZE-1;Tempi++)
	{
		printf("%c",datatemp[Tempi]);
	}
	printf("\n");
	
	//擦出标定标志位，恢复默认值
	//W25_DataTemp[0]=0xaa;				
	//W25QXX_Write(W25_DataTemp,0,1);

	W25QXX_Read(W25_SetDataTemp,0,10);		//设置数据读取
	W25QXX_Read(W25_CalibData,30,20);		//标定数据读取
		
	UnitFlag=W25_SetDataTemp[3];
	
	if(W25_CalibData[18]==0x55)
	{
		printf("W25q16 Byte18=0x%x Thermometer Has Already Calib\n",W25_CalibData[18]);
		//Compensate_AD校正补偿值计算：
		SetTemp=W25_SetDataTemp[1];
		Calib25_AD=W25_CalibData[0]*1000000+W25_CalibData[1]*10000+W25_CalibData[2]*100+W25_CalibData[3];
		Calib34_AD=W25_CalibData[4]*1000000+W25_CalibData[5]*10000+W25_CalibData[6]*100+W25_CalibData[7];
		Calib37_AD=W25_CalibData[8]*1000000+W25_CalibData[9]*10000+W25_CalibData[10]*100+W25_CalibData[11];
		Calib42_AD=W25_CalibData[12]*1000000+W25_CalibData[13]*10000+W25_CalibData[14]*100+W25_CalibData[15];
		NTC25AD=W25_CalibData[16]*100+W25_CalibData[17];
		
		Diff25=(Calib34_AD-Calib25_AD)/90;
		Diff34=(Calib37_AD-Calib34_AD)/30;
		Diff37=(Calib42_AD-Calib37_AD)/50;
	}
	else
	{
		printf("W25q18 Byte0=0x%x Thermometer Has Not Calib\n",W25_CalibData[18]);
		SetTemp=W25_SetDataTemp[1];
		Calib25_AD=0x00;
		Calib34_AD=0x00;
		Calib37_AD=0x00;
		Calib42_AD=0x00;
		NTC25AD=1633;
		
		Diff25=(Calib34_AD-Calib25_AD)/90;
		Diff34=(Calib37_AD-Calib34_AD)/30;
		Diff37=(Calib42_AD-Calib37_AD)/50;
		SetTemp=0;
 	}
	printf("Calib25_AD=%d\n",Calib25_AD);
	printf("Calib34_AD=%d\n",Calib34_AD);
	printf("Calib37_AD=%d\n",Calib37_AD);
	printf("Calib42_AD=%d\n",Calib42_AD);
	printf("NTC25AD=%d\n",NTC25AD);
	printf("Diff25=%d\n",Diff25);
	printf("Diff34=%d\n",Diff34);
	printf("Diff37=%d\n",Diff37);

	
	
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
	
	ColorNum=W25_SetDataTemp[6];
	ColorNum++;
	if(ColorNum>6)
	{
		ColorNum=0;
	}
	//显示大号 88.8℃
	POINT_COLOR=ColorTable1[ColorNum];
	
	LCD_ShowNum(0,56,88,128,Charasc128,2,0);	//88
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
	POINT_COLOR=ColorTable2[ColorNum];
	
	W25_SetDataTemp[6]=ColorNum;
	W25QXX_Write(W25_SetDataTemp,0,10);
	
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
	
	//显示F或者℃
	POINT_COLOR=WHITE;
	if(UnitFlag>0)
	{
		LCD_ShowChar(214,78,15,48,Charasc48,0);		//F
	}
	else
	{
		Draw_Circle(208,88,4);						//.
		Draw_Circle(208,88,3);						//.
		LCD_ShowChar(214,78,14,48,Charasc48,0);		//C
	}
	
	//显示Obj或者Body
	if(W25_SetDataTemp[5]>0)
	{
		LCD_ShowChar(40-10,180,29,32,Charasc32,0);		//B
		LCD_ShowChar(56-10,180,30,32,Charasc32,0);		//o
		LCD_ShowChar(72-10,180,31,32,Charasc32,0);		//d
		LCD_ShowChar(88-10,180,32,32,Charasc32,0);		//y
	}
	else
	{
		LCD_ShowChar(40-10,180,34,32,Charasc32,0);		//O
		LCD_ShowChar(56-10,180,35,32,Charasc32,0);		//b
		LCD_ShowChar(72-10,180,36,32,Charasc32,0);		//j
		LCD_ShowChar(88-10,180,37,32,Charasc32,0);		//e
		LCD_ShowChar(104-10,180,38,32,Charasc32,0);		//c
		LCD_ShowChar(120-10,180,39,32,Charasc32,0);		//t
	}
	
	
	//显示Buzzer
	if(W25_SetDataTemp[4]>0)
	{
		BuzzerFalg=false;
		LCD_Clearpart(10,10,44,41,BLACK);
	}
	else
	{
		BuzzerFalg=true;
	}
	
	
	
	
	Cs1237IO_Init();
	Con_CS1237();
	Init_CS1237();
	Tempi=Read_CON();
	printf("Read_CON=%d,CS1237 Init OK\n",Tempi);
	
	
	
	PowerKeyFalg=false;
	SetFalg=false;
	CalibFalg=false;
	KeyupFirstFlag=true;
	
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
				NTCMeasureAD=Get_Adc_Average(ADC_Channel_2,4);
				NTCVotage=NTCMeasureAD*Vref/4096;
				for(Tempi=0;Tempi<30;Tempi++)
				{
					if(NTCVotage>NTCTemp1039[Tempi])
					{
						break;
					}
				}
				NTCTemp=Tempi+10-1;
				printf("NTCTemp=%d	",NTCTemp);
				printf("NTCMeasureAD=%d	",NTCMeasureAD);
				
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
				
				printf("Measure_AD=%d	",ADValue);
				
				//环境温度补偿，同一目标温度，环境温度越高AD越低，被测目标AD减小
				if(ADValue<Calib34_AD)
				{
					NTCAD_Compensate=130;
				}
				else if(ADValue<Calib37_AD)
				{
					NTCAD_Compensate=150;
				}
				else if(ADValue<Calib42_AD)
				{
					NTCAD_Compensate=165;
				}
				else
				{
					NTCAD_Compensate=170;
				}
				
				
				
				if(NTCMeasureAD<=NTC25AD)			//NTCMeasureTemp >=25℃
				{
					ADValue += (NTCAD_Compensate*(NTC25AD - NTCMeasureAD));
					printf("NTCAD_Compensate=+%d	",NTCAD_Compensate*(NTC25AD - NTCMeasureAD));
				}
				else
				{
					ADValue -= (NTCAD_Compensate*(NTCMeasureAD-NTC25AD));
					printf("NTCAD_Compensate=-%d	",NTCAD_Compensate*(NTCMeasureAD-NTC25AD));
				}
				//计算目标温度
				if(ADValue>Calib42_AD)									// Tobj>42℃ 显示HI						
				{
					ADValue=ADValue-Calib42_AD;
					TobjTemp=ADValue/Diff37;
					TobjTemp=420+TobjTemp;
				}
				else if(ADValue>Calib37_AD)								//37<Tobj<=42℃			
				{
					ADValue=ADValue-Calib37_AD;
					TobjTemp=ADValue/Diff37;
					TobjTemp=370+TobjTemp;
				}
				else if(ADValue>Calib34_AD)								//34<Tobj<=37℃			
				{
					ADValue=ADValue-Calib34_AD;
					TobjTemp=ADValue/Diff34;
					TobjTemp=340+TobjTemp;
				}
				else if(ADValue>Calib25_AD)								//25<Tobj<=34℃			
				{
					ADValue=ADValue-Calib25_AD;
					TobjTemp=ADValue/Diff25;
					TobjTemp=250+TobjTemp;
				}
				else
				{
					ADValue=Calib25_AD-ADValue;
					TobjTemp=ADValue/Diff25;
					TobjTemp=250-TobjTemp;
				}
				
				printf("TobjTemp=%d	 ",TobjTemp);
				
				//Obj与Body转换，默认是Obj的值
				if(W25_SetDataTemp[5]>0)												//转换Body的值
				{
					TobjTemp+=(float)((420-TobjTemp)*ObjtoBody);
					printf("TobjTemp_Body=%d	 ",TobjTemp);
				}
				
				//微调补偿
				if(W25_SetDataTemp[2]>0)
				{
					TobjTemp=TobjTemp+W25_SetDataTemp[1];
					printf("CompensateTemp=+%d\n",W25_SetDataTemp[1]);
				}
				else
				{
					TobjTemp=TobjTemp-W25_SetDataTemp[1];
					printf("CompensateTemp=-%d\n",W25_SetDataTemp[1]);
				}
				
				if(TobjTemp>420&&W25_SetDataTemp[5]>0)									// Tobj>42℃且是obj模式 显示HI						
				{
					LCD_Clearpart(0,56,240,184,BLACK);
					POINT_COLOR=RED;
					LCD_ShowChar(51,56,10,128,Charasc128,0);	
					LCD_ShowChar(125,56,11,128,Charasc128,0);	
				}
				else if(TobjTemp<250&&W25_SetDataTemp[5]>0)								// Tobj<25℃且是obj模式 显示LO										
				{
					LCD_Clearpart(0,56,240,184,BLACK);					
					POINT_COLOR=RED;
					LCD_ShowChar(51,56,12,128,Charasc128,0);	
					LCD_ShowChar(125,56,13,128,Charasc128,0);	
				}
				else																	//25<=Tobj<=42℃			
				{
					tempupdate(TobjTemp,NTCTemp);
				}
			}
		}
		else if(KeyMode==0 && SetFalg==false )
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
				SetFalg=true;
				POINT_COLOR=GREEN;
				
				
				
//———————————————————————设置微调±0.1℃——————————————————————————	
				LCD_Clearpart(0,56,240,240,BLACK);
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
				
				
				LCD_Clearpart(0,100,240,180,BLACK);
				//显示±0.1℃
				if(W25_SetDataTemp[2]>0)
				{
					LCD_ShowChar(28,120,11,64,Charasc64,0);		//+		
				}
				else
				{
					LCD_ShowChar(28,120,12,64,Charasc64,0);		//-	
				}
				
				SetTempi=SetTemp/10;
				SetTempj=SetTemp%10;
				LCD_ShowNum(64,120,SetTempi,64,Charasc64,1,0);	//SetTempi
				Draw_Circle(104,168,0);							//.
				Draw_Circle(104,168,1);							//.
				Draw_Circle(104,168,2);							//.
				Draw_Circle(104,168,3);							//.
				LCD_ShowNum(112,120,SetTempj,64,Charasc64,1,0);	//SetTempj
				Draw_Circle(148,134,0);							//.
				Draw_Circle(148,134,1);							//.
				Draw_Circle(148,134,2);							//.
				Draw_Circle(148,134,3);							//.
				LCD_ShowChar(152,120,10,64,Charasc64,0);		//C
				delay_ms(500);
				while(KeyMode==0)
				{
				
				}
				
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
							if(SetTemp>0)
							{
								SetTemp--;
							}
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
							LCD_ShowChar(28,120,11,64,Charasc64,0);			//+		
						}
						else
						{
							LCD_ShowChar(28,120,12,64,Charasc64,0);			//-	
						}
						LCD_ShowNum(64,120,SetTempi,64,Charasc64,1,0);		//SetTempi
						LCD_ShowNum(112,120,SetTempj,64,Charasc64,1,0);		//SetTempj
					}
					else if(Keydown==0)
					{

						delay_ms(200);
						if(W25_SetDataTemp[2]>0)
						{
							if(SetTemp>0)
							{
								SetTemp--;
							}
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
						
						W25_SetDataTemp[1]=SetTempi*10+SetTempj;
							
						break;
					}
				}
//———————————————————————设置F/℃———————————————————————————	
				LCD_Clearpart(0,56,240,184,BLACK);
				//显示F2:F/℃
				LCD_ShowChar(16,56,26,32,Charasc32,0);	//F
				LCD_ShowChar(36,70,2,16,Charasc16,0);	//2
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
				while(KeyMode==0)
				{
					
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
							LCD_Clearpart(95,120,136,184,BLACK);
							Draw_Circle(100,134,0);					//.
							Draw_Circle(100,134,1);					//.
							Draw_Circle(100,134,2);					//.
							Draw_Circle(100,134,3);					//.
							LCD_ShowChar(104,120,10,64,Charasc64,0);//C
						}
						else
						{
							LCD_Clearpart(95,120,136,184,BLACK);
							W25_SetDataTemp[3]=1;
							LCD_Clearpart(100,120,240,184,BLACK);
							LCD_ShowChar(104,120,13,64,Charasc64,0);//F
						}
					}
					else if(KeyMode==0)
					{
						delay_ms(500);
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
					LCD_ShowChar(72,120,15,64,Charasc64,0);		//OFF
					LCD_ShowChar(104,120,17,64,Charasc64,0);	//OFF
					LCD_ShowChar(136,120,17,64,Charasc64,0);	//OFF
				}
				else
				{
					LCD_ShowChar(72,120,15,64,Charasc64,0);		//ON
					LCD_ShowChar(104,120,16,64,Charasc64,0);	//ON
				}
				while(KeyMode==0)
				{
					
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
							LCD_ShowChar(72,120,15,64,Charasc64,0);		//ON
							LCD_ShowChar(104,120,16,64,Charasc64,0);	//ON
						}
						else
						{
							W25_SetDataTemp[4]=1;
							LCD_Clearpart(72,120,240,184,BLACK);
							LCD_ShowChar(72,120,15,64,Charasc64,0);		//OFF
							LCD_ShowChar(104,120,17,64,Charasc64,0);	//OFF
							LCD_ShowChar(136,120,17,64,Charasc64,0);	//OFF
						}
					}
					else if(KeyMode==0)
					{
						delay_ms(500);
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
				while(KeyMode==0)
				{
					
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
//						while(KeyMode==0)
//						{
//							
//						}
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
				SetFalg=false;
				delay_ms(2000);
				SET_Power_L;
			}
		}
		else if(Keyup==0 && CalibFalg==false )
		{
			if(KeyupFirstFlag==true)
			{
				KeyupFirstFlag=false;
				KeyupCount=0;
			}
			if(KeyupCount>30)
			 {
				KeyupCount=0;
				BuzzerCount=3;
				CalibFalg=true;
//———————————————————————标定25，34，37，42————————————————————————
				ShowF25(25);			
				NTC25AD=0x00;
				 
				while(Keyup==0)
				{
					
				}
				while(1)
				{
					if(ReadKey==0)
					{
						delay_ms(50);
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
						
						Calib25_AD=ADValue;										//25℃标定
						NTC25AD+=Get_Adc_Average(ADC_Channel_2,4);
						printf("NTC25AD1=%d\n",NTC25AD);
						printf("Calib25_AD=%d\n",Calib25_AD);
						ShowPass();						
						break;
					}
				}
				
				ShowF25(34);			

				while(1)
				{
					if(ReadKey==0)
					{
						delay_ms(50);
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
						
						Calib34_AD=ADValue;										//34℃标定
						NTC25AD+=Get_Adc_Average(ADC_Channel_2,4);
						printf("NTC25AD2=%d\n",NTC25AD);
						printf("CalibAD34=%d\n",Calib34_AD);
						ShowPass();						
						break;
					}
					
				}
				
				ShowF25(37);			
				while(1)
				{
					if(ReadKey==0)
					{
						delay_ms(50);
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
						
						Calib37_AD=ADValue;										//37℃标定
						NTC25AD+=Get_Adc_Average(ADC_Channel_2,4);
						printf("NTC25AD3=%d\n",NTC25AD);
						printf("CalibAD37=%d\n",Calib37_AD);
						ShowPass();						
						break;
					}
				}
				
				ShowF25(42);			
				while(1)
				{
					if(ReadKey==0)
					{
						delay_ms(50);
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
						
						Calib42_AD=ADValue;										//42℃标定
						NTC25AD+=Get_Adc_Average(ADC_Channel_2,4);
						printf("NTC25AD4=%d\n",NTC25AD);
						printf("CalibAD42=%d\n",Calib42_AD);
						break;
					}
				}
				NTC25AD=NTC25AD/4;
				W25_CalibData[0]=Calib25_AD/1000000;
				W25_CalibData[1]=(Calib25_AD%1000000)/10000;
				W25_CalibData[2]=(Calib25_AD%10000)/100;
				W25_CalibData[3]=Calib25_AD%100;
				W25_CalibData[4]=Calib34_AD/1000000;
				W25_CalibData[5]=(Calib34_AD%1000000)/10000;
				W25_CalibData[6]=(Calib34_AD%10000)/100;
				W25_CalibData[7]=Calib34_AD%100;
				W25_CalibData[8]=Calib37_AD/1000000;
				W25_CalibData[9]=(Calib37_AD%1000000)/10000;
				W25_CalibData[10]=(Calib37_AD%10000)/100;
				W25_CalibData[11]=Calib37_AD%100;
				W25_CalibData[12]=Calib42_AD/1000000;
				W25_CalibData[13]=(Calib42_AD%1000000)/10000;
				W25_CalibData[14]=(Calib42_AD%10000)/100;
				W25_CalibData[15]=Calib42_AD%100;
				W25_CalibData[16]=NTC25AD/100;
				W25_CalibData[17]=NTC25AD%100;
				W25_CalibData[18]=0x55;				//数据是否标定标志位

				Diff25=(Calib34_AD-Calib25_AD)/90;
				Diff34=(Calib37_AD-Calib34_AD)/30;
				Diff37=(Calib42_AD-Calib37_AD)/50;
				
				printf("---------Calib_Data---------");
				printf("Calib25_AD=%d\n",Calib25_AD);
				printf("Calib34_AD=%d\n",Calib34_AD);
				printf("Calib37_AD=%d\n",Calib37_AD);
				printf("Calib42_AD=%d\n",Calib42_AD);
				printf("NTC25AD=%d\n",NTC25AD);
				printf("Diff25=%d\n",Diff25);
				printf("Diff34=%d\n",Diff34);
				printf("Diff37=%d\n",Diff37);
				
				//标定数据错误，动态显示Fail
				if(Diff25>1000 || Diff34>1000 || Diff37>1000 || Diff25<200 ||Diff34<200 || Diff37<200 )
				{
					for(Tempi=0;Tempi<3;Tempi++)
					{
						LCD_Clearpart(0,60,240,240,BLACK);
						LCD_ShowChar(24,90,10,96,Charasc96,0);		//Fail
						LCD_ShowChar(72,90,13,96,Charasc96,0);		//Fail
						LCD_ShowChar(120,90,15,96,Charasc96,0);		//Fail
						LCD_ShowChar(168,90,16,96,Charasc96,0);		//Fail
						delay_ms(500);
						LCD_Clearpart(0,60,240,186,BLACK);
						delay_ms(200);
					}
				}
				else
				{
					ShowPass();						
					LCD_Clearpart(0,60,240,200,BLACK);
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
					W25QXX_Write(W25_CalibData,30,20);
					CalibFalg=false;
				}
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
//				LCD_Clearpart(0,56,240,184,BLACK);
//				//清楚显示HOLD
//				LCD_Clearpart(52,8,128,40,BLACK);
//				//显示大号 88.8℃
//				POINT_COLOR=WHITE;
//				LCD_Fill(10,130,60,142,WHITE);			//-
//				LCD_Fill(70,130,120,142,WHITE);			//-
//				Draw_Circle(137,152,0);					//.
//				Draw_Circle(137,152,1);					//.
//				Draw_Circle(137,152,2);					//.
//				Draw_Circle(137,152,3);					//.
//				Draw_Circle(137,152,4);					//.
//				Draw_Circle(137,152,5);					//.
//				LCD_Fill(150,130,200,142,WHITE);
//				Draw_Circle(205,70,4);					//.
//				Draw_Circle(205,70,3);					//.
//				LCD_ShowChar(210,70,14,48,Charasc48,0);	//C
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

void ShowPass(void)
{
	POINT_COLOR=WHITE;

	LCD_Clearpart(0,60,240,160,BLACK);
	LCD_ShowChar(24,90,12,96,Charasc96,0);		//Pass
	LCD_ShowChar(72,90,13,96,Charasc96,0);		//Pass
	LCD_ShowChar(120,90,14,96,Charasc96,0);		//Pass
	LCD_ShowChar(168,90,14,96,Charasc96,0);		//Pass
	delay_ms(500);
}
void ShowF25(uint8_t Ftemp)
{
	uint8_t ii;
	uint8_t jj;
	ii=Ftemp/10;
	jj=Ftemp%10;
	LCD_Clearpart(0,60,240,160,BLACK);
	//显示F-25
	POINT_COLOR=WHITE;
	LCD_ShowChar(24,90,10,96,Charasc96,0);		
	LCD_ShowChar(72,90,11,96,Charasc96,0);		
	LCD_ShowChar(120,90,ii,96,Charasc96,0);		
	LCD_ShowChar(168,90,jj,96,Charasc96,0);		
	delay_ms(300);
}








