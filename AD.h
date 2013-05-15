#ifndef AD_H
#define AD_H

#include <avr/io.h>
#include <avr/interrupt.h>

//AD値の移動平均回数
#define AD_SUM_ORDER (1<<6)

//AD変換を何chまでする?
#define AD_CH_OREDER (6)

/////初期化関数　必ずリセット後に一回実行すること
void initAd(void);

//最近の移動平均データを返す　ノー無駄待ち
volatile int16_t getAd(uint8_t ch);

#endif
