#include "AD.h"


static volatile uint16_t ad_result_raw[AD_CH_OREDER*AD_SUM_ORDER];
static volatile uint8_t ad_ch_count = 0;
static volatile uint8_t ad_sum_count = 0;

/////	AD初期化
void initAd(void){
	ADMUX = (0x1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADIE) | (0x7 << ADPS0);
	ADCSRB = 0x00;
	DIDR0 = 0x3F;

	ADCSRA |= (1 << ADSC);
}

volatile int16_t getAd(uint8_t ch){
	uint8_t i;
	uint16_t temp=0;
	if(ch >= AD_CH_OREDER)return 0;
	for(i = 0; i< AD_SUM_ORDER;i++){
		temp += ad_result_raw[ch*AD_SUM_ORDER+i];
	}
	return (int16_t)(temp / AD_SUM_ORDER);
}

//	ADコンバート割り込み
ISR(ADC_vect){
	ADCSRA &= ~(1 << ADEN);
	ad_result_raw[ad_ch_count*AD_SUM_ORDER+ad_sum_count] = ADC;
	ad_ch_count++;

	if(ad_ch_count >= AD_CH_OREDER){
		ad_ch_count = 0;
		ad_sum_count++;
		if(ad_sum_count >= AD_SUM_ORDER){
			ad_sum_count = 0;
		}
	}

	ADMUX &= 0xF0;
	ADMUX |= ad_ch_count;
	ADCSRA |= (1 << ADEN) | (1 << ADIF);
	ADCSRA |= (1 << ADSC);
}
