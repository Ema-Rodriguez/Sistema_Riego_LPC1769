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
#include "lpc17xx_timer.h"

//Declaracion de funciones
void setPins();
void setTimer0Adc();//timer que utiliza el adc
void setTimer1Riego();//timer para activar bomba de riego
void onTimer1();
void offTimer1();
void setAdc();//adc para sensor de humedad
void onAdc();
void offAdc();


int main(void) {
	setPins();
	setTimer0Adc();
	setTimer1Riego();
	setAdc();
	onAdc();
    while(1){}
    return 0 ;
}

//Funciones de configuracion
void setPins(){
	PINSEL_CFG_Type pinAdc;
	pinAdc.Portnum= PINSEL_PORT_0;
	pinAdc.Pinnum= PINSEL_PIN_23;
	pinAdc.Funcnum= PINSEL_FUNC_1;
	pinAdc.Pinmode= PINSEL_PINMODE_TRISTATE;
	pinAdc.OpenDrain= PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&pinAdc);//Configuramos AD0.0

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

	//TIM_ResetCounter(LPC_TIM0);//reseteamos el tim0
	//TIM_Cmd(LPC_TIM0, ENABLE);//Prendemos el tim0
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

//handlers de interrupciones
void ADC_IRQHandler(){
	if(1/*ADC_ChannelGetData>550*/){
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


