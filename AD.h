#ifndef AD_H
#define AD_H

#include <avr/io.h>
#include <avr/interrupt.h>

//AD�l�̈ړ����ω�
#define AD_SUM_ORDER (1<<6)

//AD�ϊ�����ch�܂ł���?
#define AD_CH_OREDER (6)

/////�������֐��@�K�����Z�b�g��Ɉ����s���邱��
void initAd(void);

//�ŋ߂̈ړ����σf�[�^��Ԃ��@�m�[���ʑ҂�
volatile int16_t getAd(uint8_t ch);

#endif
