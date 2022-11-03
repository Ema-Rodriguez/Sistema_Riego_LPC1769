/*
===============================================================================
 Name        : TP_Final_ED3.c
 Author      : $Brian Gerard, Emanuel Rodriguez
 Version     : 1
 Copyright   :
 Description : Control de riego de una planta mediante sensor de humedad,
 manejo de bomba hidraulica y sensor ultrasonico para medir nivel de agua
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

/*********************************************************************************/
//Declaracion de funciones
/*********************************************************************************/

void setPins();//Configuracion de los pines
void setAdc();//ADC para sensor de humedad
void setTimer0Adc();//timer que utiliza el ADC
void onAdc(); //prende ADC
void offAdc(); // apaga ADC
void setTimer2Riego();//timer para activar bomba de riego
void onTimer2(); //prende timer2
void offTimer2(); //apaga timer2

void setDMA_DAC(); //configuracion  LLI DMA
void setDac(); //configuracion DAC para sonido
void onDac(int); //prende DAC
void offDac();  // apaga DAC
void setRiegoMan(); //Boton para regar manualmente

void confUart(); //configuracion comunicacion UART
void confDMA_UART(); //config DMA-UART
void actualizarMensaje(); //actualiza mensaje UART

void controlarNivel(); //controla nivel de agua de la bomba

uint_fast16_t potencia(uint8_t , uint_fast8_t );

/*********************************************************************************/
//Valores de la funcion seno, estos valores ya se encuentran desplazados 6 lugares
//a la derecha para cargarlos en DACR
uint32_t sinu[60]={32768, 36160, 39552, 42880, 46080, 49152, 51968, 54656, 57088, 59264
, 61120, 62656, 63872, 64768, 65344, 65472, 65344, 64768, 63872, 62656, 61120, 59264
, 57088, 54656, 51968, 49152, 46080, 42880, 39552, 36160, 32768, 29376, 25984, 22656
, 19456, 16384, 13568, 10880, 8448, 6272, 4416, 2880, 1664, 768, 192, 0, 192, 768, 1664
, 2880, 4416, 6272, 8448, 10880, 13568, 16384, 19456, 22656, 25984, 29376};
/*********************************************************************************/
//Variables Globales
/*********************************************************************************/

GPDMA_Channel_CFG_Type GPDMACfg_DAC;
GPDMA_LLI_Type DMA_LLI_Struct_DAC;

GPDMA_LLI_Type DMA_LLI_Struct_UART;
GPDMA_Channel_CFG_Type GPDMACfg_UART;

uint8_t mensaje[30] = {"PORCENTAJE DE HUMEDAD: \r  \n"};

int flag=0;
uint8_t humedad=0;
int cont=0;

int main(void) {
	setPins();
	setTimer0Adc();
	setTimer2Riego();
	confUart();
	GPDMA_Init();
	setDac();
	setAdc();
	setRiegoMan();
	onAdc();
    while(1){}
    return 0 ;
}

/*********************************************************************************/
//Funciones de configuracion
/*********************************************************************************/

//Seteamos los pines para el adc,dac y pin para control de encendido de la bomba
void setPins(){
	PINSEL_CFG_Type pinAdc;
	PINSEL_CFG_Type pinDac;
	PINSEL_CFG_Type pinArd;

	pinAdc.Portnum= PINSEL_PORT_0;
	pinAdc.Pinnum= PINSEL_PIN_23;
	pinAdc.Funcnum= PINSEL_FUNC_1;
	pinAdc.Pinmode= PINSEL_PINMODE_TRISTATE;
	pinAdc.OpenDrain= PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinAdc);//Configuramos AD0.0

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
	PINSEL_ConfigPin(&pinArd);//Configuramos P0.10
	pinArd.Pinnum= PINSEL_PIN_11;
	PINSEL_ConfigPin(&pinArd);//Configuramos P0.11

	LPC_GPIO0->FIODIR &= ~(1<<10);//Seteo como entrada P0.10 para señal
	LPC_GPIO0->FIOCLR |= (1<<10);//Seteo en 0
	LPC_GPIO0->FIODIR &= ~(1<<11);//Seteo como entrada P0.10 para señal
	LPC_GPIO0->FIOCLR |= (1<<11);//Seteo en 0

	//configuramos el p0.0 como salida para riego
	LPC_PINCON->PINMODE0 |= (1<<1);
	GPIO_SetDir(0, 0x1, 1);//Configuramos p0.0 como salida
	LPC_GPIO0->FIOCLR |=(1<<0); //Seteo en 0 la salida
	LPC_GPIO0->FIODIR |=(1<<1);
	LPC_GPIO0->FIOSET |=(1<<1);

	//configuramos el p0.21 Para activar riego manual
	LPC_GPIO0->FIODIR &= ~(1<<21);//P0.21 como entrada
	LPC_GPIO0->FIOCLR |= (1<<21);//Seteo en 0 la salida.

}


//Seteamos el timer0 que utilizara el adc
void setTimer0Adc(){
	TIM_TIMERCFG_Type timStruc;
	timStruc.PrescaleOption = TIM_PRESCALE_USVAL;
	timStruc.PrescaleValue = 3000000;//para hacer 30 seg 30000000

	TIM_MATCHCFG_Type matchConf;
	matchConf.MatchChannel = 1; //para MAT0.1
	matchConf.MatchValue = 1;
	matchConf.IntOnMatch = DISABLE;//no interrumpe
	matchConf.ResetOnMatch = ENABLE;//se resetea en el match
	matchConf.StopOnMatch = DISABLE;
	matchConf.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;//hace un toggle cada vez que matchea

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timStruc);//configuramos timer
	TIM_ConfigMatch(LPC_TIM0, &matchConf);//configuramos el match

}
//seteamos el adc para utilizar con el tim0 a una ft=200khz
void setAdc(){
	ADC_Init(LPC_ADC, 200000);//seteamos a una frec de 200k
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE); //configuramos CH0
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE); //habilitamos interrupcion en CH0
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING); //ADC empieza en flanco descendente
}
//funcion que enciende el adc
void onAdc(){
	TIM_ResetCounter(LPC_TIM0);//reseteamos el tim0
	TIM_Cmd(LPC_TIM0, ENABLE);//Prendemos el tim0
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);//empieza en MAT0.1
	NVIC_EnableIRQ(ADC_IRQn); //habilitamos interrupcion
}
//funcion que apaga el adc
void offAdc(){
	TIM_Cmd(LPC_TIM0, DISABLE);//Apagamos el tim0 por lo que el adc no convierte.
	NVIC_DisableIRQ(ADC_IRQn);//deshabilitamos interrupcion

}
//funciones riego
//timer para control de riego el cual tiene 2 match, 1 que nos controla
//el tiempo el cual va a estar encendida la bomba hidraulica y el otro
//que despues de un determinado tiempo enciendo el adc
void setTimer2Riego(){
	TIM_TIMERCFG_Type timStruc;
	timStruc.PrescaleOption = TIM_PRESCALE_USVAL;
	timStruc.PrescaleValue = 5000000;//para hacer 5 seg

	TIM_MATCHCFG_Type match0;
	match0.MatchChannel = 0;//para MAT2.0
	match0.MatchValue = 1;//para que sea 5 seg de riego
	match0.IntOnMatch = ENABLE;//interrumpe en match
	match0.ResetOnMatch = DISABLE;//no se resetea
	match0.StopOnMatch = DISABLE;
	match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

	TIM_MATCHCFG_Type match1;
	match1.MatchChannel = 1;//para MAT2.1
	match1.MatchValue = 3;//para que sea 20 min sin tomar medicion 240
	match1.IntOnMatch = ENABLE;//interrumpe en match
	match1.ResetOnMatch = ENABLE;//resetea TIMER2
	match1.StopOnMatch = ENABLE;
	match1.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

	TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timStruc);
	TIM_ConfigMatch(LPC_TIM2, &match0);//configuramos el MAT2.0
	TIM_ConfigMatch(LPC_TIM2, &match1);//configuramos el MAT2.1

}
//Funcion para prender TIMER2
void onTimer2(){
	TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);//limpia flags de interrupcion
	TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);//limpia flags de interrupcion
	NVIC_EnableIRQ(TIMER2_IRQn);//habilitamos interrupcion
	TIM_ResetCounter(LPC_TIM2);//reseteamos el TIM2
	TIM_Cmd(LPC_TIM2, ENABLE);//Prendemos el TIM2
}
//Funcion para apagar el TIM2
void offTimer2(){
	TIM_Cmd(LPC_TIM2, DISABLE);//deshabilita TIM2
	NVIC_DisableIRQ(TIMER2_IRQn);//deshabilita interrupcion
}
//Funcion para configurar LLI DMA
void setDMA_DAC(){
	DMA_LLI_Struct_DAC.SrcAddr= (uint32_t)sinu; //el source va a ser el arreglo de la funcion seno
	DMA_LLI_Struct_DAC.DstAddr= (uint32_t)&(LPC_DAC->DACR);//el destination es el registro del DAC
	DMA_LLI_Struct_DAC.NextLLI= (uint32_t)&DMA_LLI_Struct_DAC;
	DMA_LLI_Struct_DAC.Control= 60
			| (2<<18)//Fuente: 32bits
			| (2<<21)//Destino: 32bit
			| (1<<26)//Incremento automático de la fuente
			;

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
//Funcion para configurar el DAC
void setDac(){
	DAC_CONVERTER_CFG_Type dacStruc;
	dacStruc.CNT_ENA = 1;
	dacStruc.DMA_ENA = 1;

	DAC_Init(LPC_DAC);// Inicializo controlador de GPDMA
	DAC_ConfigDAConverterControl(LPC_DAC, &dacStruc);

}
//Funcion para prender el DAC
void onDac(int frecuencia){ //hay que pasarle la frec que queremos que suene
	uint32_t time_out=25000000/(frecuencia*60);//4500 frec de la onda
	setDMA_DAC();
	DAC_SetDMATimeOut(LPC_DAC,time_out);
	GPDMA_ChannelCmd(0, ENABLE); //Enciende el modulo DMA channel0
}
//Funcion para apagar DAC
void offDac(){
	GPDMA_ChannelCmd(0, DISABLE); //Apaga el modulo DMA channel0
}
//Funcion para regar manualmente
void setRiegoMan(){
	LPC_GPIOINT->IO0IntEnF |= (1<<21); //interrumpe flanco descendente
	LPC_GPIOINT->IO0IntClr |= (1<<21); //limpio flags interrupcion
	NVIC_EnableIRQ(EINT3_IRQn); //habilita interrupcion
}
//Funcion para configurar UART
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
//Funcion para configurar DMA para la comunicacion UART
void confDMA_UART(void)
{
	DMA_LLI_Struct_UART.SrcAddr= (uint32_t)mensaje;//el source va a ser el mensaje
	DMA_LLI_Struct_UART.DstAddr= (uint32_t)&LPC_UART0->THR;//el destination UART
	DMA_LLI_Struct_UART.NextLLI= 0;//(uint32_t)&DMA_LLI_Struct_UART;
	DMA_LLI_Struct_UART.Control= sizeof(mensaje)
		| 	(2<<12)
		| 	(1<<26) //source increment
		;

	NVIC_DisableIRQ(DMA_IRQn);// Desabilito la interrupcion de GPDMA
	//GPDMA_Init();// Inicializo controlador de GPDMA

	GPDMACfg_UART.ChannelNum = 1;
	GPDMACfg_UART.SrcMemAddr = (uint32_t)mensaje;
	GPDMACfg_UART.DstMemAddr = 0;
	GPDMACfg_UART.TransferSize = sizeof(mensaje);
	GPDMACfg_UART.TransferWidth = 0;
	GPDMACfg_UART.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg_UART.SrcConn = 0;
	GPDMACfg_UART.DstConn = GPDMA_CONN_UART0_Tx;
	GPDMACfg_UART.DMALLI = (uint32_t)&DMA_LLI_Struct_UART;

	GPDMA_Setup(&GPDMACfg_UART);
	GPDMA_ChannelCmd(1, ENABLE);//Enciende el modulo DMA channel1
}
//Funcion para actualizar mensaje
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
//Funcion para controlar nivel de agua de la bomba
void controlarNivel(){
	mensaje[0]= 'Q';//mensaje para diferenciar en arduino
	flag=0; //variable para que se mantenga en un bucle hasta que llegue la señal de medicion
	confDMA_UART();
	while(flag==0){
		if(LPC_GPIO0->FIOPIN & (1<<10)){
			flag=1;
		}if(LPC_GPIO0->FIOPIN & (1<<11)){
			flag=2;
		}
	}
	mensaje[0]= 'P';
	confDMA_UART();
}

/*********************************************************************************/
//handlers de interrupciones
/*********************************************************************************/

void ADC_IRQHandler(){
	humedad = 100-((ADC_ChannelGetData(LPC_ADC, 0)*100)/4095);//convierto nivel de humedad
	offDac();//apaga el dac
	controlarNivel();//controla nivel de agua de la bomba
	actualizarMensaje();//actualiza mensaje UART
	int val= (LPC_ADC->ADDR0>>4 & (0xfff));//valor leido ADC

	//if((val>3800)){
	if(humedad<40){
		if(flag==1){//puedo regar
		GPIO_SetValue(0, 0x1);//enciendo la bomba
		LPC_GPIO0->FIOCLR |= (1<<1);
		}
	}
	if(flag==2){//significa que no tengo agua suficiente
		onDac(4500);//prendo sonido para avisar que no hay agua
	}
	onTimer2();//prendo timer2
	offAdc();//apago ADC
}

void TIMER2_IRQHandler(){
	if(TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)==SET){//pregunto si interrumpio MAT2.0
		GPIO_ClearValue(0, 0x1);//Apago la bomba
		LPC_GPIO0->FIOSET |= (1<<1);
	}else if(TIM_GetIntStatus(LPC_TIM2, TIM_MR1_INT)==SET){//pregunto si interrumpio MAT2.1
		offTimer2();//apago TIMER2
		onAdc();//prendo ADC
	}
	TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);//limpio flags interrupcion
	TIM_ClearIntPending(LPC_TIM2, TIM_MR1_INT);//limpio flags interrupcion
}

void EINT3_IRQHandler(){
	onTimer2();//prendo TIMER2
	GPIO_SetValue(0, 0x1);//prendo la bomba
	LPC_GPIO0->FIOCLR |= (1<<1);
	LPC_GPIOINT->IO0IntClr |= (1<<21);//limpio flags de interrupcion
}

/*********************************************************************************/
