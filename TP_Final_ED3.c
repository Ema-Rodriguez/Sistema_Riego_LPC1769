/*
===============================================================================
 Name        : TP_Final_ED3.c
 Author      : $Brian Gerard, Emanuel Rodriguez
 Version     : 1
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_uart.h"
#include <string.h>

//Declaracion de funciones
void setPins();//Configuracion de los pines
void setAdc();//adc para sensor de humedad
void setTimer0Adc();//timer que utiliza el adc
void onAdc();
void offAdc();
void setTimer2Riego();//timer para activar bomba de riego
void onTimer2();
void offTimer2();

void setDac();
void onDac(int);
void offDac();
void setRiegoMan();

void confUart();
void confDMA_UART();
void actualizarMensaje();

void setDMA_DAC();

uint_fast16_t potencia(uint8_t , uint_fast8_t );
//
GPDMA_Channel_CFG_Type GPDMACfg_DAC;
GPDMA_LLI_Type DMA_LLI_Struct_DAC;

GPDMA_LLI_Type DMA_LLI_Struct_UART;
GPDMA_Channel_CFG_Type GPDMACfg_UART;

uint8_t mensaje[30] = {"PORCENTAJE DE HUMEDAD: \r  \n"};

//Estos valores ya se encuentran desplazados 6 lugares a la derecha para cargarlos en DACR
uint32_t sinu[60]={32768, 36160, 39552, 42880, 46080, 49152, 51968, 54656, 57088, 59264
, 61120, 62656, 63872, 64768, 65344, 65472, 65344, 64768, 63872, 62656, 61120, 59264
, 57088, 54656, 51968, 49152, 46080, 42880, 39552, 36160, 32768, 29376, 25984, 22656
, 19456, 16384, 13568, 10880, 8448, 6272, 4416, 2880, 1664, 768, 192, 0, 192, 768, 1664
, 2880, 4416, 6272, 8448, 10880, 13568, 16384, 19456, 22656, 25984, 29376};

int flag=0;
uint8_t humedad=0;
int cont=0;

int main(void) {
	setPins();
	setTimer0Adc();
	setTimer2Riego();
	confUart();
	confDMA_UART();
	setDMA_DAC();
	setDac();
	setAdc();
	setRiegoMan();
	onAdc();
	//onDac(4500);
    while(1){}
    return 0 ;
}

//Funciones de configuracion
void setPins(){
	PINSEL_CFG_Type pinAdc;
	PINSEL_CFG_Type pinDac;
	PINSEL_CFG_Type pinCap;
	PINSEL_CFG_Type pinArd;

	pinAdc.Portnum= PINSEL_PORT_0;
	pinAdc.Pinnum= PINSEL_PIN_23;
	pinAdc.Funcnum= PINSEL_FUNC_1;
	pinAdc.Pinmode= PINSEL_PINMODE_TRISTATE;
	pinAdc.OpenDrain= PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinAdc);//Configuramos ad0.0

	pinDac.Portnum= PINSEL_PORT_0;
	pinDac.Pinnum= PINSEL_PIN_26;
	pinDac.Funcnum= PINSEL_FUNC_2;
	pinDac.Pinmode= PINSEL_PINMODE_TRISTATE;
	pinDac.OpenDrain= PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinDac);//Configuramos AOUT


	pinArd.Portnum= PINSEL_PORT_0;
	pinArd.Pinnum= PINSEL_PIN_10;
	pinArd.Funcnum= 0;
	pinArd.Pinmode= PINSEL_PINMODE_TRISTATE;
	pinArd.OpenDrain= PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinArd);//Configuramos p0.10
	pinArd.Pinnum= PINSEL_PIN_11;
	PINSEL_ConfigPin(&pinArd);//Configuramos p0.11

	LPC_GPIO0->FIODIR &= ~(1<<10);//Seteo como salida P0.10 para señal
	LPC_GPIO0->FIOCLR |= (1<<10);//Seteo en 0
	LPC_GPIO0->FIODIR &= ~(1<<11);//Seteo como salida P0.10 para señal
	LPC_GPIO0->FIOCLR |= (1<<11);//Seteo en 0

	//configuramos el p0.0 como salida para riego
	GPIO_SetDir(0, 0x1, 1);//Configuramos p0.0 como salida
	GPIO_ClearValue(0, 0x1);//Seteo en 0 la salida, //CAMBIAR

	//configuramos el p0.21 Para activar riego manual
	//GPIO_SetDir(0, 0x1, 0);//Configuramos p0.21 como salida
	LPC_GPIO0->FIODIR |= (1<<21);//0,21 como salida
	LPC_GPIO0->FIOSET |= (1<<21);//Seteo en 1 la entrada.

}

//funciones adc
void setTimer0Adc(){
	TIM_TIMERCFG_Type timStruc;
	timStruc.PrescaleOption = TIM_PRESCALE_USVAL;
	timStruc.PrescaleValue = 3000000;//para hacer 30 seg 30000000

	TIM_MATCHCFG_Type matchConf;
	matchConf.MatchChannel = 1;//para mat0.1
	matchConf.MatchValue = 1;
	matchConf.IntOnMatch = DISABLE;
	matchConf.ResetOnMatch = ENABLE;
	matchConf.StopOnMatch = DISABLE;
	matchConf.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timStruc);
	TIM_ConfigMatch(LPC_TIM0, &matchConf);//configuramos el match

}
void setAdc(){
	ADC_Init(LPC_ADC, 200000);//seteamos a una frec de 200k
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING);
	//NVIC_EnableIRQ(ADC_IRQn);
}
void onAdc(){
	TIM_ResetCounter(LPC_TIM0);//reseteamos el tim0
	TIM_Cmd(LPC_TIM0, ENABLE);//Prendemos el tim0
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
	NVIC_EnableIRQ(ADC_IRQn);
}
void offAdc(){
	TIM_Cmd(LPC_TIM0, DISABLE);//Apagamos el tim0 por lo que el adc no convierte.
	NVIC_DisableIRQ(ADC_IRQn);

}
//funciones riego
void setTimer2Riego(){
	TIM_TIMERCFG_Type timStruc;
	timStruc.PrescaleOption = TIM_PRESCALE_USVAL;
	timStruc.PrescaleValue = 5000000;//para hacer 5 seg

	TIM_MATCHCFG_Type match0;
	match0.MatchChannel = 0;//para match0
	match0.MatchValue = 1;//para que sea 5 seg de riego
	match0.IntOnMatch = ENABLE;
	match0.ResetOnMatch = DISABLE;
	match0.StopOnMatch = DISABLE;
	match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

	TIM_MATCHCFG_Type match1;
	match1.MatchChannel = 1;//para match0
	match1.MatchValue = 3;//para que sea 20 min sin tomar medicion 240
	match1.IntOnMatch = ENABLE;
	match1.ResetOnMatch = ENABLE;
	match1.StopOnMatch = ENABLE;
	match1.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;


	TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timStruc);
	TIM_ConfigMatch(LPC_TIM2, &match0);//configuramos el match0
	TIM_ConfigMatch(LPC_TIM2, &match1);//configuramos el match1

}
void onTimer2(){
	TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
	TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);
	NVIC_EnableIRQ(TIMER2_IRQn);
	TIM_ResetCounter(LPC_TIM2);//reseteamos el tim2
	TIM_Cmd(LPC_TIM2, ENABLE);//Prendemos el tim2
}
void offTimer2(){
	TIM_Cmd(LPC_TIM2, DISABLE);
	NVIC_DisableIRQ(TIMER2_IRQn);
}
void setDMA_DAC(){
	DMA_LLI_Struct_DAC.SrcAddr= (uint32_t)sinu;
	DMA_LLI_Struct_DAC.DstAddr= (uint32_t)&(LPC_DAC->DACR);
	DMA_LLI_Struct_DAC.NextLLI= (uint32_t)&DMA_LLI_Struct_DAC;
	DMA_LLI_Struct_DAC.Control= 60
			| (2<<18)//Fuente: 32bits
			| (2<<21)//Destino: 32bit
			| (1<<26)//Incremento automático de la fuente
			;
	//GPDMA_Init();
	GPDMACfg_DAC.ChannelNum = 0;
	GPDMACfg_DAC.SrcMemAddr = (uint32_t)sinu;
	GPDMACfg_DAC.DstMemAddr = 0;
	GPDMACfg_DAC.TransferSize = 60;
	GPDMACfg_DAC.TransferWidth = 0;
	GPDMACfg_DAC.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg_DAC.SrcConn = 0;
	GPDMACfg_DAC.DstConn = GPDMA_CONN_DAC;
	GPDMACfg_DAC.DMALLI= (uint32_t)&DMA_LLI_Struct_DAC;
	GPDMA_Setup(&GPDMACfg_DAC);
}
void setDac(){
	DAC_CONVERTER_CFG_Type dacStruc;
	dacStruc.CNT_ENA = 1;
	dacStruc.DMA_ENA = 1;

	DAC_Init(LPC_DAC);
	DAC_ConfigDAConverterControl(LPC_DAC, &dacStruc);

}
void onDac(int frecuencia){
	uint32_t time_out=25000000/(frecuencia*60);//4500 frec de la onda
	DAC_SetDMATimeOut(LPC_DAC,time_out);
	GPDMA_ChannelCmd(0, ENABLE); //Enciende el modulo DMA
}
void offDac(){
	GPDMA_ChannelCmd(0, DISABLE); //Enciende el modulo DMA
}

void setRiegoMan(){
	LPC_GPIOINT->IO0IntEnF |= (1<<21);
	LPC_GPIOINT->IO0IntClr |= (1<<21);
	NVIC_EnableIRQ(EINT3_IRQn);
	//NVIC_SetPriority(EINT3_IRQn, priority);
}


void confUart(void)
{
	PINSEL_CFG_Type PinCfg;
	//configuracion pin de Tx y Rx
	PinCfg.Funcnum = PINSEL_FUNC_1;
	PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
	PinCfg.Pinnum = PINSEL_PIN_2;
	PinCfg.Portnum = PINSEL_PORT_0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = PINSEL_PIN_3;
	PINSEL_ConfigPin(&PinCfg);

	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;

	//configuracion por defecto:
	UART_ConfigStructInit(&UARTConfigStruct);
	//inicializa periferico
	UART_Init(LPC_UART0, &UARTConfigStruct);

	//configuro la FIFO para DMA
	UARTFIFOConfigStruct.FIFO_DMAMode = ENABLE;
	UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV0;
	UARTFIFOConfigStruct.FIFO_ResetRxBuf = ENABLE;
	UARTFIFOConfigStruct.FIFO_ResetTxBuf = ENABLE;

	//Inicializa FIFO
	UART_FIFOConfig(LPC_UART0, &UARTFIFOConfigStruct);
	//Habilita transmision
	UART_TxCmd(LPC_UART0, ENABLE);
}
void confDMA_UART(void)
{
	//Preparo la LLI del DMA
	DMA_LLI_Struct_UART.SrcAddr= (uint32_t)mensaje;
	DMA_LLI_Struct_UART.DstAddr= (uint32_t)&LPC_UART0->THR;
	DMA_LLI_Struct_UART.NextLLI= (uint32_t)&DMA_LLI_Struct_UART;
	DMA_LLI_Struct_UART.Control= sizeof(mensaje)
		| 	(2<<12)
		| 	(1<<26) //source increment
		;

	// Desabilito la interrupcion de GDMA
	NVIC_DisableIRQ(DMA_IRQn);
	// Inicializo controlador de GPDMA
	GPDMA_Init();
	//Configuracion del canal 0
	// CANAL 0
	GPDMACfg_UART.ChannelNum = 1;
	// Fuente de memoria
	GPDMACfg_UART.SrcMemAddr = (uint32_t)mensaje;
	// Destino de memoria, al ser periferico es 0
	GPDMACfg_UART.DstMemAddr = 0;
	// tamaño de trasnferencia
	GPDMACfg_UART.TransferSize = sizeof(mensaje);
	// Ancho de trasnferencia
	GPDMACfg_UART.TransferWidth = 0;
	// Tipo de memoria
	GPDMACfg_UART.TransferType = GPDMA_TRANSFERTYPE_M2P;
	// Coneccion de la Fuente, sin usar al ser periferico
	GPDMACfg_UART.SrcConn = 0;
	// Coneccion de destino
	GPDMACfg_UART.DstConn = GPDMA_CONN_UART0_Tx;
	// LLI
	GPDMACfg_UART.DMALLI = (uint32_t)&DMA_LLI_Struct_UART;

	GPDMA_Setup(&GPDMACfg_UART);

	// Habilito canal 0
	GPDMA_ChannelCmd(1, ENABLE);
}

void actualizarMensaje(){
	uint16_t temp=humedad;
	uint8_t temp1=0;
	for(uint8_t i=2;i>0;i--)
	{
		temp1=temp/(potencia(10, i-1));

		mensaje[24-i]=temp1 + '0';

		temp-=temp1*(potencia(10, i-1));
	}
	mensaje[25]='%';
	//strcat(mensaje, char(humedad));
}

uint_fast16_t potencia(uint8_t numero, uint_fast8_t potencia)
{
	uint16_t resultado = numero;
	while (potencia > 1)
	{
		resultado = resultado * numero;
		potencia--;
	}
	if(potencia==0)
	{
		resultado=1;
	}
	return resultado;
}



//handlers de interrupciones
void ADC_IRQHandler(){
	humedad = 100-((ADC_ChannelGetData(LPC_ADC, 0)*100)/4095);
	mensaje[0]= 'Q';
	offDac();

	flag=0;
	while(flag==0){
		if(LPC_GPIO0->FIOPIN & (1<<10)){
			flag=1;
		}if(LPC_GPIO0->FIOPIN & (1<<11)){
			flag=2;
		}
	}
	mensaje[0]= 'P';
	actualizarMensaje();
	//if(ADC_ChannelGetData(LPC_ADC, 0)>650){
	//ControlarNivel();
	int val= (LPC_ADC->ADDR0>>4 & (0xfff));

	if((val>3800)){
		if(flag==1){//puedo regar
		GPIO_SetValue(0, 0x1);//enciendo la bomba
		}
	}
	if(flag==2){//recargar agua
		onDac(4500);
	}
	onTimer2();
	offAdc();
	//else{GPIO_ClearValue(0, 0x1);}//no hago nada}
}

void TIMER2_IRQHandler(){
	if(TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)==SET){
		GPIO_ClearValue(0, 0x1);//Apago la bomba
	}else if(TIM_GetIntStatus(LPC_TIM2, TIM_MR1_INT)==SET){
		offTimer2();
		onAdc();
	}
	TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
	TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);
}

void EINT3_IRQHandler(){
	onTimer2();
	GPIO_SetValue(0, 0x1);//prendo la bomba
	LPC_GPIOINT->IO0IntClr |= (1<<21);
}
