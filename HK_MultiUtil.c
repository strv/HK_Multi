#include "HK_MultiUtil.h"
volatile uint16_t rx1_st,rx2_st,rx3_st,rx4_st;
volatile uint16_t rx1,rx2,rx3,rx4;

void initBoardIo(void){
	PORTB &= ~(1 << PIN_LED) | (1 << PIN_M3) | (1 << PIN_M2) | (1 << PIN_M1);
	DDRB &= ~(1 << PIN_RUD);
	DDRB |= (1 << PIN_LED) | (1 << PIN_M3) | (1 << PIN_M2) | (1 << PIN_M1);
	PORTC &= ~((1 << PIN_GYR_ROL) | (1 << PIN_GYR_PIT) | (1 << PIN_GYR_YAW) | (1 << PIN_POT_ROL) | (1 << PIN_POT_PIT) | (1 <<PIN_POT_YAW));
	DDRC &= ~((1 << PIN_GYR_ROL) | (1 << PIN_GYR_PIT) | (1 << PIN_GYR_YAW) | (1 << PIN_POT_ROL) | (1 << PIN_POT_PIT) | (1 <<PIN_POT_YAW));
	PORTD &= ~((1 << PIN_AIL) | (1 << PIN_ELE) | (1 << PIN_THR) | (1 << PIN_M6) | (1 << PIN_M5) | (1 << PIN_M4));
	DDRD |= (1 << PIN_M6) | (1 << PIN_M5) | (1 << PIN_M4);
	DDRD &=  ~((1 << PIN_AIL) | (1 << PIN_ELE) | (1 << PIN_THR));
}

void initSoftPwm(void){	
	TCCR2B = (0x4 << CS20);
}

void initRxInt(void){
	
	PCICR = (1 << PCIE2) | (1 << PCIE0);
	PCMSK0 = (1 << PCINT7);
	PCMSK2 = (1 << PCINT17);
	EICRA = (0x1 << ISC10) | (0x1 << ISC00);
	EIMSK = (1 << INT1) | (1 << INT0);
	
	TCCR1B = (0x2 << CS10);
}

volatile void wait(uint16_t w){
	for(;w>0;w--){
		TCNT2 = 0;
		while(TCNT2 <126);
	}
}

volatile void startPwm(){
	PORTB |= PORTB_MASK;
	PORTD |= PORTD_MASK;
	GTCCR |= (1 << PSRASY);
	TCNT2 = 0;
}

volatile void scalePwm(int16_t *pOut1,int16_t *pOut2,int16_t *pOut3,int16_t *pOut4){
	if(*pOut1 < 0)*pOut1 = 0;
	else if(*pOut1 > 1023)*pOut1 = 1023;
	if(*pOut2 < 0)*pOut2 = 0;
	else if(*pOut2 > 1023)*pOut2 = 1023;
	if(*pOut3 < 0)*pOut3 = 0;
	else if(*pOut3 > 1023)*pOut3 = 1023;
	if(*pOut4 < 0)*pOut4 = 0;
	else if(*pOut4 > 1023)*pOut4 = 1023;
	*pOut1 /= 4;
	*pOut2 /= 4;
	*pOut3 /= 4;
	*pOut4 /= 4;
}

volatile void getRx(uint16_t *pRxR,uint16_t *pRxP,uint16_t *pRxY,uint16_t *pRxT){
	if(rx1 < 0)rx1=-rx1;
	if(rx2 < 0)rx2=-rx2;
	if(rx3 < 0)rx3=-rx3;
	if(rx4 < 0)rx4=-rx4;
	*pRxR = rx1 - (RX_CENTER);
	*pRxP = rx2 - (RX_CENTER);
	*pRxY = rx4 - (RX_CENTER);
	*pRxT = rx3 - (RX_MIN);
	if(*pRxT < 0)*pRxT=0;
}

//PIN_AIL
ISR(PCINT2_vect){
	if(PIND & (1 << PIN_AIL)){
		rx1_st = TCNT1;
	}else{
		rx1 = TCNT1 - rx1_st;
	}
}

//PIN_ELE
ISR(INT0_vect){
	if(PIND & (1 << PIN_ELE)){
		rx2_st = TCNT1;
	}else{
		rx2 = TCNT1 - rx2_st;
	}
}

//PIN_THR
ISR(INT1_vect){
	if(PIND & (1 << PIN_THR)){
		rx3_st = TCNT1;
	}else{
		rx3 = TCNT1 - rx3_st;
	}
}

//PIN_RUD
ISR(PCINT0_vect){
	if(PINB & (1 << PIN_RUD)){
		rx4_st = TCNT1;
	}else{
		rx4 = TCNT1 - rx4_st;
	}
}

