#include "I2C.h"
#include "system.h"

uint8_t device_addr = 0x00;

void initI2C(uint32_t baud){
	uint32_t twbr = ((SYS_CLK / baud) - 16) / 2;
	TWSR &= 0xFC;
	if(twbr > 0xFF){
		twbr = ((SYS_CLK / baud) - 16) / 8;
		TWSR |= (1 << TWPS0);
	}
	if(twbr > 0xFF){
		twbr = ((SYS_CLK / baud) - 16) / 32;
		TWSR |= (1 << TWPS1);
	}
	if(twbr > 0xFF){
		twbr = ((SYS_CLK / baud) - 16) / 128;
		TWSR |= (1 << TWPS1 | 1 << TWPS0);
	}
	TWBR = twbr & 0xFF;
	
	PORTC &= 0b11001111;
	DDRC |= 0b00110000;
}

void i2cSetDeviceAddr(uint8_t d_addr){
	device_addr = (d_addr << 1) & 0xFE;
}

static uint8_t i2cSendStart(void){
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); 
	while((TWCR & (1 << TWINT)) == 0);	
	return TW_STATUS;
}

static void i2cSendStop(void){
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); 
}

static uint8_t i2cSendByte(uint8_t data){
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while((TWCR & (1 << TWINT)) == 0);
	return TW_STATUS;
}

int8_t i2cSingleWrite(uint8_t addr,uint8_t data){
	if(device_addr > 0xFf)return I2C_ERR_ADDRESS_LONG;
	
	i2cSendStop();
	i2cSendStart();
	switch(i2cSendByte(device_addr | I2C_WRITE)){
		case TW_MT_SLA_ACK:
			break;
			
		case TW_MT_SLA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
			return I2C_ERR_UNKNOWN;		
	}
	
	switch(i2cSendByte(addr)){
		case TW_MT_DATA_ACK:
			break;
			
		case TW_MT_DATA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
			return I2C_ERR_UNKNOWN;			
	}
	
	switch(i2cSendByte(data)){
		case TW_MT_DATA_ACK:
			break;
			
		case TW_MT_DATA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
			return I2C_ERR_UNKNOWN;			
	}
	i2cSendStop();	
	return 1;
}

int8_t i2cSingleRead(uint8_t addr,uint8_t *pData){
	uint8_t try_cnt=0,error_code=0;
	if(device_addr > 0xff)return I2C_ERR_ADDRESS_LONG;

restart:	
	try_cnt++;
	if(try_cnt > I2C_MAX_TRY)return I2C_ERR_TIMEOUT;
begin:
	
	switch(i2cSendStart()){
		case TW_REP_START:
		case TW_START:
		break;
		
		case TW_MT_ARB_LOST:
		printf("ARB_LOST\n");
		goto begin;
		
		default:
		return I2C_ERR_START;
	}
	
	switch(error_code = i2cSendByte(device_addr | I2C_WRITE)){
		case TW_MT_SLA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MT_SLA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("00 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;
	}
	
	//printf("SendRegAddr\n");
	switch(error_code = i2cSendByte(addr)){
		case TW_MT_DATA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MT_DATA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("01 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;			
	}
	
	//printf("SendStart\n");
	switch(i2cSendStart()){
		case TW_REP_START:
		case TW_START:
		break;
		
		case TW_MT_ARB_LOST:
		printf("ARB_LOST\n");
		goto begin;
		
		default:
		return I2C_ERR_START;
	}
	
	switch(error_code = i2cSendByte(device_addr | I2C_READ)){
		case TW_MR_SLA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MR_SLA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("02 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;
	}
	
	
	TWCR = (1 << TWINT) | (1 << TWEN);
	while((TWCR & (1 << TWINT)) == 0);
	switch(error_code = TW_STATUS){
		case TW_MR_DATA_NACK:
			//printf("Return NACK\n");
			break;
		case TW_MR_DATA_ACK:
			//printf("Return ACK\n");
			break;
			
		default:
		printf("03 Exception:0X%X\n",error_code);
			break;			
	}
	*pData = TWDR;
	i2cSendStop();
	return 1;	
}

int8_t i2cBurstWrite(uint8_t addr,uint8_t length,uint8_t *pData){
	uint8_t txLen=0,error_code=0,try_cnt=0;
	
restart:	
	try_cnt++;
	if(try_cnt > I2C_MAX_TRY)return I2C_ERR_TIMEOUT;
begin:

	switch(i2cSendStart()){
		case TW_REP_START:
		case TW_START:
		break;
		
		case TW_MT_ARB_LOST:
		printf("ARB_LOST\n");
		goto begin;
		
		default:
		return I2C_ERR_START;
	}
	
	switch(error_code = i2cSendByte(device_addr | I2C_WRITE)){
		case TW_MT_SLA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MT_SLA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("00 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;
	}
	
	//printf("SendRegAddr\n");
	switch(error_code = i2cSendByte(addr)){
		case TW_MT_DATA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MT_DATA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("01 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;			
	}
	
	for(;length > 0;length--){
		switch(i2cSendByte(*(pData + txLen))){
			case TW_MT_DATA_ACK:
			txLen++;
			//printf("Recieve Ack\n");
				break;
			
			case TW_MT_DATA_NACK:
			printf("Recieve Nack\n");
				return I2C_ERR_NACK;
			
			default:
				return I2C_ERR_UNKNOWN;			
		}
	}
	i2cSendStop();	
	return txLen;
}

int8_t i2cBurstRead(uint8_t addr,uint8_t length,uint8_t *pData){
	uint8_t try_cnt=0,error_code=0,rxLen=0;
	if(device_addr > 0xff)return I2C_ERR_ADDRESS_LONG;

restart:	
	try_cnt++;
	if(try_cnt > I2C_MAX_TRY)return I2C_ERR_TIMEOUT;
begin:
	
	switch(i2cSendStart()){
		case TW_REP_START:
		case TW_START:
		break;
		
		case TW_MT_ARB_LOST:
		printf("ARB_LOST\n");
		goto begin;
		
		default:
		return I2C_ERR_START;
	}
	
	switch(error_code = i2cSendByte(device_addr | I2C_WRITE)){
		case TW_MT_SLA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MT_SLA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("00 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;
	}
	
	//printf("SendRegAddr\n");
	switch(error_code = i2cSendByte(addr)){
		case TW_MT_DATA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MT_DATA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("01 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;			
	}
	
	//printf("SendStart\n");
	switch(i2cSendStart()){
		case TW_REP_START:
		case TW_START:
		break;
		
		case TW_MT_ARB_LOST:
		printf("ARB_LOST\n");
		goto begin;
		
		default:
		return I2C_ERR_START;
	}
	
	switch(error_code = i2cSendByte(device_addr | I2C_READ)){
		case TW_MR_SLA_ACK:
		//printf("Recieve Ack\n");
			break;
			
		case TW_MR_SLA_NACK:
		printf("Recieve Nack\n");
			return I2C_ERR_NACK;
			
		default:
		printf("02 Exception:0X%X\n",error_code);
			return I2C_ERR_UNKNOWN;
	}
	
	for(rxLen=0;length>0;length--){
		if(length == 1)	TWCR = (1 << TWINT) | (1 << TWEN);
		else TWCR = (1 << TWINT) | (1 << TWEN | (1 << TWEA));
		
		while((TWCR & (1 << TWINT)) == 0);
		switch(error_code = TW_STATUS){
			case TW_MR_DATA_NACK:
				//printf("Return NACK\n");
				length = 0;
			case TW_MR_DATA_ACK:
				//printf("Return ACK\n");
				*(pData+rxLen) = TWDR;
				rxLen++;
				if(error_code == TW_MR_DATA_NACK)goto quit;
				break;
			
			default:
			printf("03 Exception:0X%X\n",error_code);
				break;			
		}
	}
quit:
	i2cSendStop();
	return rxLen;	
}