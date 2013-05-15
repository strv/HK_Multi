
#ifndef HK_MULTICONFIG_H_
#define HK_MULTICONFIG_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "system.h"

//�|�[�g��`
#define PIN_LED PB6
#define PIN_AIL PD1	//PCINT17
#define PIN_ELE PD2	//INT0
#define PIN_THR PD3	//INT1
#define PIN_RUD PB7	//PCINT7
#define PIN_M6  PD5
#define PIN_M5  PD6
#define PIN_M4  PD7
#define PIN_M3  PB0
#define PIN_M2  PB1
#define PIN_M1  PB2
#define PIN_GYR_ROL PC2
#define PIN_GYR_PIT PC1
#define PIN_GYR_YAW PC0
#define PIN_POT_YAW PC5
#define PIN_POT_PIT PC4
#define PIN_POT_ROL PC3

//IO�𑽏��֗��ɂ���
#define LED_ON() (PORTB |= (1 << PIN_LED))
#define LED_OFF() (PORTB &= ~(1 << PIN_LED))
#define PORTB_MASK ((1 << PIN_M1) | (1 << PIN_M2) | (1 << PIN_M3))
#define PORTD_MASK ((1 << PIN_M4) | (1 << PIN_M5) | (1 << PIN_M6))

//���W�R��PWM�̒��S�l[us]
#define RX_CENTER (1520)
//���W�R��PWM�̍ŏ��l[us]
#define RX_MIN (1020)

//�X���b�g�����ŏ��ɂȂ��Ă���Ɣ��f����X���b�V�����h
#define RX_THR_THR (180)
//���_�[���ő�ɂȂ��Ă���Ɣ��f����X���b�V�����h
#define RX_YAW_THR (350)

//���o�͂̃C�j�V�����C�Y
void initBoardIo(void);

//�\�t�g�E�F�APWM�֌W�̃C�j�V�����C�Y
void initSoftPwm(void);

//��M�@����̐M�����󂯎��֌W�̃C�j�V�����C�Y
void initRxInt(void);

//���ʑ҂�[ms]
volatile void wait(uint16_t);

//PWM�o�͊J�n�@���̊֐������s���Ă���1[ms]�ȓ���endPwm�����s����K�v������
volatile void startPwm(void);

//PWM�o�͏I���@startPwm�����s���Ă���1[ms]�ȓ��Ɏ��s����K�v������
//�����Ŗ�1[ms]���炢���ʑ҂�����
//���g�̓A�Z���u���֐� softPwm.s
volatile void endPwm(uint8_t,uint8_t,uint8_t,uint8_t);

//�o�͂ł���PWM��0�`256�ŁA����������0�`1023�Ȃ̂ł����Ȃ�悤�ɃX�P�[�����O����
volatile void scalePwm(int16_t*,int16_t*,int16_t*,int16_t*);

//��M�@����̐M�����󂯎��[us]
volatile void getRx(uint16_t*,uint16_t*P,uint16_t*,uint16_t*);

#endif /* HK_MULTICONFIG_H_ */