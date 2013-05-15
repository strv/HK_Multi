
#ifndef HK_MULTICONFIG_H_
#define HK_MULTICONFIG_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "system.h"

//ポート定義
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

//IOを多少便利にする
#define LED_ON() (PORTB |= (1 << PIN_LED))
#define LED_OFF() (PORTB &= ~(1 << PIN_LED))
#define PORTB_MASK ((1 << PIN_M1) | (1 << PIN_M2) | (1 << PIN_M3))
#define PORTD_MASK ((1 << PIN_M4) | (1 << PIN_M5) | (1 << PIN_M6))

//ラジコンPWMの中心値[us]
#define RX_CENTER (1520)
//ラジコンPWMの最小値[us]
#define RX_MIN (1020)

//スロットルが最小になっていると判断するスレッショルド
#define RX_THR_THR (180)
//ラダーが最大になっていると判断するスレッショルド
#define RX_YAW_THR (350)

//入出力のイニシャライズ
void initBoardIo(void);

//ソフトウェアPWM関係のイニシャライズ
void initSoftPwm(void);

//受信機からの信号を受け取る関係のイニシャライズ
void initRxInt(void);

//無駄待ち[ms]
volatile void wait(uint16_t);

//PWM出力開始　この関数を実行してから1[ms]以内にendPwmを実行する必要がある
volatile void startPwm(void);

//PWM出力終了　startPwmを実行してから1[ms]以内に実行する必要がある
//内部で約1[ms]くらい無駄待ちする
//中身はアセンブラ関数 softPwm.s
volatile void endPwm(uint8_t,uint8_t,uint8_t,uint8_t);

//出力できるPWMは0～256で、内部処理は0～1023なのでそうなるようにスケーリングする
volatile void scalePwm(int16_t*,int16_t*,int16_t*,int16_t*);

//受信機からの信号を受け取る[us]
volatile void getRx(uint16_t*,uint16_t*P,uint16_t*,uint16_t*);

#endif /* HK_MULTICONFIG_H_ */