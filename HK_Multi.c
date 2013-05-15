/*
 * HK_Multi.c
 *
 * Created: 2012/09/28 1:01:29
 *  Author: strv
 *
 * HobbyKing�̃}���`�R�v�^�[����{�[�hV3�œ����N�A�b�h�R�v�^�[�p����v���O����
 * �{�[�h�ƃ��[�^�z�u�̊֌W�͈ȉ��̂Ƃ���B
 *  �O
 * M1 M3	M1:CW M3:CCW
 *  \/
 *  /\
 * M2 M4	M2:CCW M4:CW
 *  ��
 *
 * �o��PWM��400Hz�߂��̂őΉ����Ă�ESC�ɂȂ����ƁB
 * �T�[�{���Ȃ��Ɣ߂������ƂɂȂ肩�˖����̂łȂ��Ȃ��̂��������߁B
 *
 * �o��PWM�̐����͖��߂̎��s�^�C�~���O�Ɉˑ����Ă���̂ŁA
 * ����ȏ㊄�荞�݂𑝂₳�Ȃ��ق����W�b�^�𑝂₳�Ȃ��̂ŋg
 * 
 * �œK����Os��O3��������ׂ�
 * 
 * �X�V����
 * 2012/10/03	�E���J
 * 2012/10/04	�E�W���C�����S�l�̎擾�Ŏg��i�̏������Y��C��
 *				�E�X�e�B�b�N���삪�q���߂����̂ŁA�S�̂Ɋ���Z�����悤�ɂ���
 *				�@����ɔ����A�|�e���V���̕␳�W����ύX���₷���悤�ɂ���
 *				�@����ɔ����A�e�␳�W���ύX�@���̂ւ�͋@�̂ɍ��킹�ēK�X
 *				�E�N������LED���_�ł���悤�ɂ���
 *
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "HK_MultiUtil.h"
#include "AD.h"
#include "I2C.h"

//I���̐ώZ�ʂ̍ő�l
#define ISUMLIMIT (100)
//I�Q�C���̕���
#define IGAIN_DIV (1<<4)

//�W���C���̒��S�l�Ƃ邽�߂ɕ��ς����
#define CENTER_SUM_ORDER (16)

//�W���C����AD�l����v�Z�Ɏg���₷���l�ɕ␳����l�@���������������ƃQ�C���オ�����̂Ɠ���
#define SCALE_GYRO (1<<1)		//���q
#define SCALE_GYRO_DIV (1<<1)	//����
//��M�@�̐M������v�Z�Ɏg���₷���l�ɕ␳����l�@���ꏬ��������Ƒ���ɑ΂��Ă��񂵂���
#define SCALE_RX (1<<2)
#define SCALE_RX_RP (1<<8)
//�|�e���V���̒l����Q�C���ɂ���␳�W�� �����傫������ƃQ�C�����������Ȃ�
#define SCALE_GAIN (1<<3)		
//�ŏI�I�Ȓl����鎞�̕␳�W���@�����傫������ƁA�S�̓I�ɓ݂��Ȃ�
#define SCALE_OUT (1<<2)

//�W���C���̌����̕␳ 1��-1
#define GYRO_DIR_R (1)
#define GYRO_DIR_P (1)
#define GYRO_DIR_Y (-1)

//�f�o�b�O����Ȃ�wait���ז��Ȃ̂ł�����΂��@0�ł��Ȃ��@1�ł���
#define DO_DEBUG 0

int main(void)
{
	volatile int16_t rxRoll,rxPitch,rxYaw,rxThr;	//��M�@�̏��
	volatile int16_t out1=0,out2=0,out3=0,out4=0;	//�o�͒l
	int16_t roll,pitch,yaw,diff;					//�e���̐���o�́@�Ԃ����Ⴏ��ŏ��ԂɌv�Z����΂���
	int16_t roll_last,pitch_last,yaw_last;			//D����p
	int16_t pGain=0,iGain=0,yawPGain=0,dGain=0;		//����Q�C��
	int16_t iSumP,iSumR,iSumY;						//�e����I��
	int16_t rollCenter,pitchCenter,yawCenter;		//�e���W���C���̒��S�l
	int32_t target;									//����ڕW�l�̃e���|����
	uint8_t i=0,mode=0,mode_count = 0;				//�G�p��i�A���[�h�A���[�h�J��Ԃ����J�E���^
	
	//�K�X�C�j�V�����C�Y
	cli();
	initBoardIo();
	initRxInt();
	initSoftPwm();
	initAd();
	sei();	
	
	LED_OFF();
	
	//�W���C���Ƃ����肳���邽�߂ɖ��ʑ҂�
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
		//��M�@�̐M�����擾
	    getRx(&rxRoll,&rxPitch,&rxYaw,&rxThr);
		
		//�X���b�g�����Œ�(�t��)?
		if(rxThr < RX_THR_THR){
			//���_�[���������ς��@���@���[�h����ڂ��Ƃ��Ă�?
			if(rxYaw < -RX_YAW_THR && mode >= 10){
				mode_count++;
				//����ȏ�Ԃ����΂炭��������
				if(mode_count > 200){
					//LED��OFF�ɂ��āA�ҋ@���[�h�Ɉڍs
					LED_OFF();
					mode_count = 0;
					mode = 0;					
				}
			}else if(rxYaw > RX_YAW_THR && mode == 0){ //���_�[���E�����ς��@���@���[�h���ҋ@?
				mode_count++;
				//����ȏ�Ԃ����΂炭��������
				if(mode_count > 200){
					//LED��ON�ɂ��āA��ڂ��Ƃ���
					LED_ON();
					mode_count = 0;
					mode = 10;
				}
			}				
		}
		
		switch(mode){
			//�ҋ@���
			case 0:
			out1 = 0;
			out2 = 0;
			out3 = 0;
			out4 = 0;
			wait(1);
			break;
			
			//�W���C���̒��S�l�擾�̏���
			case 10:			
			rollCenter = 0;
			pitchCenter = 0;
			yawCenter = 0;
			i=0;
			wait(1);
			
			mode++;
			break;
			
			//�W���C���̒��S�l�擾
			case 11:
			rollCenter += getAd(ADC2D);
			pitchCenter += getAd(ADC1D);
			yawCenter += getAd(ADC0D);
			wait(1);
			i++;
			
			//���񐔎擾������c
			if(i >= CENTER_SUM_ORDER)mode++;
			break;
			
			//�W���C���̒��S�l�v�Z
			case 12:			
			rollCenter /= CENTER_SUM_ORDER;
			pitchCenter /= CENTER_SUM_ORDER;
			yawCenter /= CENTER_SUM_ORDER;
			
			//�ꉞ�ϕ����̗ݐς��N���A
			iSumR = 0;
			iSumP = 0;
			iSumY = 0;
			
			//�|�e���V������l�Ƃ��Ă��ăQ�C���ɂ���
			pGain = getAd(PIN_POT_ROL)/SCALE_GAIN;
			//iGain = getAd(PIN_POT_PIT)/SCALE_GAIN;
			dGain = getAd(PIN_POT_PIT) * 8 /SCALE_GAIN;
			yawPGain = getAd(PIN_POT_YAW)/SCALE_GAIN;
			wait(1);
				
			//���䃋�[�v�Ɉڍs
			mode++;
			break;
			
			//���C���̐��䃋�[�v
			case 13:
			
			//�e���̃W���C���擾���āA���S�l����
			roll = getAd(ADC2D) - rollCenter;
			pitch = getAd(ADC1D) - pitchCenter;
			yaw = getAd(ADC0D) - yawCenter;
			//roll = 0;
			//pitch = 0;
			//yaw = 0;
				
			//�ȉ�PI�v�Z
			out1 = rxThr*14/16;
			out2 = rxThr*14/16;
			out3 = rxThr*14/16;
			out4 = rxThr*14/16;
			
			//���[����
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
			
			//�s�b�`��
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
			
			//���[��
			//���[�����Q�C�����Ⴄ
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
			
			//�X���b�g�����Œ�t�߂������琧�䂵�Ȃ�
			if(rxThr < RX_THR_THR){
				out1 = 0;
				out2 = 0;
				out3 = 0;
				out4 = 0;
				break;
			}
			
			break;
		}
		//PWM�o�͂����������āc
		startPwm();
		//�o�͔g�`���X�P�[�����O���āc
		scalePwm(&out1,&out2,&out3,&out4);
		//�o�͂�������
		endPwm(out1,out2,out3,out4);
		
#if DO_DEBUG == 0
		//�O�������y���̂�PWM�������Z���Ȃ�߂��Ȃ��悤�ɒ���
		//�O�������d���Ȃ炢��Ȃ�
		//wait(1);
#endif
    }
}

