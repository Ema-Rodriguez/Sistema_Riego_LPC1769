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

//Declaracion de funciones
void setPins();//Configuracion de los pines
void setAdc();//adc para sensor de humedad
void setTimer0Adc();//timer que utiliza el adc
void onAdc();
void offAdc();
void setTimer1Riego();//timer para activar bomba de riego
void onTimer1();
void offTimer1();

void setDac();
void onDac();
void offDac();

void setDMA();
//
GPDMA_Channel_CFG_Type GPDMACfg;
GPDMA_LLI_Type DMA_LLI_Struct;

//Estos valores ya se encuentran desplazados 6 lugares a la derecha para cargarlos en DACR
uint32_t sinu[60]={32768, 36160, 39552, 42880, 46080, 49152, 51968, 54656, 57088, 59264
, 61120, 62656, 63872, 64768, 65344, 65472, 65344, 64768, 63872, 62656, 61120, 59264
, 57088, 54656, 51968, 49152, 46080, 42880, 39552, 36160, 32768, 29376, 25984, 22656
, 19456, 16384, 13568, 10880, 8448, 6272, 4416, 2880, 1664, 768, 192, 0, 192, 768, 1664
, 2880, 4416, 6272, 8448, 10880, 13568, 16384, 19456, 22656, 25984, 29376};

int main(void) {
	setPins();
	setTimer0Adc();
	setTimer1Riego();
	setDMA();
	setAdc();
	onAdc();
    while(1){}
    return 0 ;
}

//Funciones de configuracion
void setPins(){
	PINSEL_CFG_Type pinAdc;
	PINSEL_CFG_Type pinDac;

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

	//configuramos el p0.0 como salida para riego
	GPIO_SetDir(0, 0x1, 1);//Configuramos p0.0 como salida
	GPIO_ClearValue(0, 0x1);//Seteo en 0 la salida

}

//funciones adc
void setTimer0Adc(){
	TIM_TIMERCFG_Type timStruc;
	timStruc.PrescaleOption = TIM_PRESCALE_USVAL;
	timStruc.PrescaleValue = 30000000;//para hacer 30 seg

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
	NVIC_EnableIRQ(ADC_IRQn);
}
void onAdc(){
	TIM_ResetCounter(LPC_TIM0);//reseteamos el tim0
	TIM_Cmd(LPC_TIM0, ENABLE);//Prendemos el tim0
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
}
void offAdc(){
	TIM_Cmd(LPC_TIM0, DISABLE);//Apagamos el tim0 por lo que el adc no convierte.
}
//funciones riego
void setTimer1Riego(){
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
	match1.MatchValue = 240;//para que sea 5 seg de riego
	match1.IntOnMatch = ENABLE;
	match1.ResetOnMatch = ENABLE;
	match1.StopOnMatch = ENABLE;
	match1.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;


	TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timStruc);
	TIM_ConfigMatch(LPC_TIM1, &match0);//configuramos el match0
	TIM_ConfigMatch(LPC_TIM1, &match1);//configuramos el match1

}
void onTimer1(){
	TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
	TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
	NVIC_EnableIRQ(TIMER1_IRQn);
	TIM_ResetCounter(LPC_TIM1);//reseteamos el tim1
	TIM_Cmd(LPC_TIM1, ENABLE);//Prendemos el tim1
}
void offTimer1(){
	TIM_Cmd(LPC_TIM1, DISABLE);
	NVIC_DisableIRQ(TIMER1_IRQn);
}
void setDMA(){
	DMA_LLI_Struct.SrcAddr= (uint32_t)sinu;
	DMA_LLI_Struct.DstAddr= (uint32_t)&(LPC_DAC->DACR);
	DMA_LLI_Struct.NextLLI= (uint32_t)&DMA_LLI_Struct;
	DMA_LLI_Struct.Control= 60
			| (2<<18)//Fuente: 32bits
			| (2<<21)//Destino: 32bit
			| (1<<26)//Incremento automático de la fuente
			;
	GPDMA_Init();
	GPDMACfg.ChannelNum = 0;
	GPDMACfg.SrcMemAddr = (uint32_t)sinu;
	GPDMACfg.DstMemAddr = 0;
	GPDMACfg.TransferSize = 60;
	GPDMACfg.TransferWidth = 0;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_DAC;
	GPDMACfg.DMALLI= (uint32_t)&DMA_LLI_Struct;
	GPDMA_Setup(&GPDMACfg);
}
void setDac(){
	DAC_CONVERTER_CFG_Type dacStruc;
	dacStruc.CNT_ENA = 1;
	dacStruc.DMA_ENA = 1;

	DAC_Init(LPC_DAC);
	DAC_ConfigDAConverterControl(LPC_DAC, &dacStruc);

}
void onDac(){
	uint32_t time_out=25000000/(4500*60);//4500 frec de la onda
	DAC_SetDMATimeOut(LPC_DAC,time_out);
	GPDMA_ChannelCmd(0, ENABLE); //Enciende el modulo DMA
}
void offDac(){}

//handlers de interrupciones
void ADC_IRQHandler(){
	if(ADC_ChannelGetData(LPC_ADC, 0)>550){
		GPIO_SetValue(0, 0x1);//enciendo la bomba
		onTimer1();
		offAdc();
	}
}

void TIMER1_IRQHandler(){
	if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)){
		GPIO_ClearValue(0, 0x1);//Apago la bomba
	}else if(TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT)){
		offTimer1();
		onAdc();
	}
	TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
	TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
}
