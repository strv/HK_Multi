/*
 * HK_Multi.c
 *
 * Created: 2012/09/28 1:01:29
 *  Author: strv
 *
 * HobbyKingのマルチコプター制御ボードV3で動くクアッドコプター用制御プログラム
 * ボードとモータ配置の関係は以下のとおり。
 *  前
 * M1 M3	M1:CW M3:CCW
 *  \/
 *  /\
 * M2 M4	M2:CCW M4:CW
 *  後
 *
 * 出力PWMは400Hz近いので対応してるESCにつなぐこと。
 * サーボをつなぐと悲しいことになりかね無いのでつながないのがおすすめ。
 *
 * 出力PWMの生成は命令の実行タイミングに依存しているので、
 * これ以上割り込みを増やさないほうがジッタを増やさないので吉
 * 
 * 最適化はOsかO3をかけるべし
 * 
 * 更新履歴
 * 2012/10/03	・公開
 * 2012/10/04	・ジャイロ中心値の取得で使うiの初期化忘れ修正
 *				・スティック操作が敏感過ぎたので、全体に割り算入れるようにした
 *				　それに伴い、ポテンショの補正係数を変更しやすいようにした
 *				　それに伴い、各補正係数変更　このへんは機体に合わせて適宜
 *				・起動時にLEDが点滅するようにした
 *
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "HK_MultiUtil.h"
#include "AD.h"
#include "I2C.h"

//I項の積算量の最大値
#define ISUMLIMIT (100)
//Iゲインの分母
#define IGAIN_DIV (1<<4)

//ジャイロの中心値とるために平均する回数
#define CENTER_SUM_ORDER (16)

//ジャイロのAD値から計算に使いやすい値に補正する値　これを小さくするとゲイン上がったのと同等
#define SCALE_GYRO (1<<1)		//分子
#define SCALE_GYRO_DIV (1<<1)	//分母
//受信機の信号から計算に使いやすい値に補正する値　これ小さくすると操作に対してせんしちぶ
#define SCALE_RX (1<<2)
#define SCALE_RX_RP (1<<8)
//ポテンショの値からゲインにする補正係数 これを大きくするとゲインが小さくなる
#define SCALE_GAIN (1<<3)		
//最終的な値を作る時の補正係数　これを大きくすると、全体的に鈍くなる
#define SCALE_OUT (1<<2)

//ジャイロの向きの補正 1か-1
#define GYRO_DIR_R (1)
#define GYRO_DIR_P (1)
#define GYRO_DIR_Y (-1)

//デバッグするならwaitが邪魔なのですっ飛ばす　0でしない　1でする
#define DO_DEBUG 0

int main(void)
{
	volatile int16_t rxRoll,rxPitch,rxYaw,rxThr;	//受信機の情報
	volatile int16_t out1=0,out2=0,out3=0,out4=0;	//出力値
	int16_t roll,pitch,yaw,diff;					//各軸の制御出力　ぶっちゃけ一個で順番に計算すればいい
	int16_t roll_last,pitch_last,yaw_last;			//D制御用
	int16_t pGain=0,iGain=0,yawPGain=0,dGain=0;		//制御ゲイン
	int16_t iSumP,iSumR,iSumY;						//各軸のI項
	int16_t rollCenter,pitchCenter,yawCenter;		//各軸ジャイロの中心値
	int32_t target;									//制御目標値のテンポラリ
	uint8_t i=0,mode=0,mode_count = 0;				//雑用のi、モード、モード繰り返し数カウンタ
	
	//適宜イニシャライズ
	cli();
	initBoardIo();
	initRxInt();
	initSoftPwm();
	initAd();
	sei();	
	
	LED_OFF();
	
	//ジャイロとか安定させるために無駄待ち
#if DO_DEBUG == 0
	for(i=0;i<5;i++){
		wait(250);
		LED_ON();
		wait(250);
		LED_OFF();
	}	
#endif
	
    while(1)
    {
		//受信機の信号を取得
	    getRx(&rxRoll,&rxPitch,&rxYaw,&rxThr);
		
		//スロットルが最低(付近)?
		if(rxThr < RX_THR_THR){
			//ラダーが左いっぱい　かつ　モードが飛ぼうとしてる?
			if(rxYaw < -RX_YAW_THR && mode >= 10){
				mode_count++;
				//そんな状態がしばらく続いたら
				if(mode_count > 200){
					//LEDをOFFにして、待機モードに移行
					LED_OFF();
					mode_count = 0;
					mode = 0;					
				}
			}else if(rxYaw > RX_YAW_THR && mode == 0){ //ラダーが右いっぱい　かつ　モードが待機?
				mode_count++;
				//そんな状態がしばらく続いたら
				if(mode_count > 200){
					//LEDをONにして、飛ぼうとする
					LED_ON();
					mode_count = 0;
					mode = 10;
				}
			}				
		}
		
		switch(mode){
			//待機状態
			case 0:
			out1 = 0;
			out2 = 0;
			out3 = 0;
			out4 = 0;
			wait(1);
			break;
			
			//ジャイロの中心値取得の準備
			case 10:			
			rollCenter = 0;
			pitchCenter = 0;
			yawCenter = 0;
			i=0;
			wait(1);
			
			mode++;
			break;
			
			//ジャイロの中心値取得
			case 11:
			rollCenter += getAd(ADC2D);
			pitchCenter += getAd(ADC1D);
			yawCenter += getAd(ADC0D);
			wait(1);
			i++;
			
			//一定回数取得したら…
			if(i >= CENTER_SUM_ORDER)mode++;
			break;
			
			//ジャイロの中心値計算
			case 12:			
			rollCenter /= CENTER_SUM_ORDER;
			pitchCenter /= CENTER_SUM_ORDER;
			yawCenter /= CENTER_SUM_ORDER;
			
			//一応積分項の累積をクリア
			iSumR = 0;
			iSumP = 0;
			iSumY = 0;
			
			//ポテンショから値とってきてゲインにする
			pGain = getAd(PIN_POT_ROL)/SCALE_GAIN;
			//iGain = getAd(PIN_POT_PIT)/SCALE_GAIN;
			dGain = getAd(PIN_POT_PIT) * 8 /SCALE_GAIN;
			yawPGain = getAd(PIN_POT_YAW)/SCALE_GAIN;
			wait(1);
				
			//制御ループに移行
			mode++;
			break;
			
			//メインの制御ループ
			case 13:
			
			//各軸のジャイロ取得して、中心値引く
			roll = getAd(ADC2D) - rollCenter;
			pitch = getAd(ADC1D) - pitchCenter;
			yaw = getAd(ADC0D) - yawCenter;
			//roll = 0;
			//pitch = 0;
			//yaw = 0;
				
			//以下PI計算
			out1 = rxThr*14/16;
			out2 = rxThr*14/16;
			out3 = rxThr*14/16;
			out4 = rxThr*14/16;
			
			//ロール軸
			target = rxRoll;
			target *= rxRoll;
			target /= SCALE_RX_RP;
			target *= rxRoll;
			target /= SCALE_RX_RP;
			target /= SCALE_RX;
			diff = (int16_t)target - (roll * GYRO_DIR_R) * SCALE_GYRO_DIV / SCALE_GYRO;
			iSumR += diff;
			if(iSumR > ISUMLIMIT)iSumR=ISUMLIMIT;
			if(iSumR < -ISUMLIMIT)iSumR=-ISUMLIMIT;
			roll = diff * pGain + (diff - roll_last) * dGain + iSumR * iGain / IGAIN_DIV;
			roll /= SCALE_OUT;
			roll_last = diff;
			
			out1 += roll;
			out2 += roll;
			out3 -= roll;
			out4 -= roll;
			
			//ピッチ軸
			target = rxPitch;
			target *= rxPitch;
			target /= SCALE_RX_RP;
			target *= rxPitch;
			target /= SCALE_RX_RP;
			target /= SCALE_RX;
			diff = (int16_t)target - (pitch * GYRO_DIR_P) * SCALE_GYRO_DIV / SCALE_GYRO;
			iSumP += diff;
			if(iSumP > ISUMLIMIT)iSumP=ISUMLIMIT;
			if(iSumP < -ISUMLIMIT)iSumP=-ISUMLIMIT;
			pitch = diff * pGain + (diff - pitch_last) * dGain + iSumP * iGain / IGAIN_DIV;
			pitch /= SCALE_OUT;
			pitch_last = diff;
			
			out1 += pitch;
			out2 -= pitch;
			out3 += pitch;
			out4 -= pitch;
			
			//ヨー軸
			//ヨーだけゲインが違う
			yaw = rxYaw / SCALE_RX - (yaw * GYRO_DIR_Y) * SCALE_GYRO_DIV / SCALE_GYRO;
			iSumY += yaw;
			if(iSumY > ISUMLIMIT)iSumY=ISUMLIMIT;
			if(iSumY < -ISUMLIMIT)iSumY=-ISUMLIMIT;
			yaw *= yawPGain;
			yaw += iSumY * iGain / IGAIN_DIV;
			yaw /= SCALE_OUT;
			
			out1 -= yaw;
			out2 += yaw;
			out3 += yaw;
			out4 -= yaw;
			
			//スロットルが最低付近だったら制御しない
			if(rxThr < RX_THR_THR){
				out1 = 0;
				out2 = 0;
				out3 = 0;
				out4 = 0;
				break;
			}
			
			break;
		}
		//PWM出力をたちあげて…
		startPwm();
		//出力波形をスケーリングして…
		scalePwm(&out1,&out2,&out3,&out4);
		//出力を下げる
		endPwm(out1,out2,out3,out4);
		
#if DO_DEBUG == 0
		//前処理が軽いのでPWM周期が短くなり過ぎないように調整
		//前処理が重いならいらない
		//wait(1);
#endif
    }
}

