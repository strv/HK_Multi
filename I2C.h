#ifndef I2C_H
#define I2C_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdlib.h>
#include <inttypes.h>

#define I2C_WRITE (0)
#define I2C_READ (1)
#define I2C_ERR_START (-1)
#define I2C_ERR_ADDRESS_LONG (-2)
#define I2C_ERR_NACK (-3)
#define I2C_ERR_TIMEOUT (-4)
#define I2C_ERR_START (-5)
#define I2C_ERR_UNKNOWN (0)

#define I2C_MAX_TRY (250)

void initI2C(uint32_t baud);
void i2cSetDeviceAddr(uint8_t d_addr);
int8_t i2cSingleWrite(uint8_t addr,uint8_t data);
int8_t i2cSingleRead(uint8_t addr,uint8_t *pData);
int8_t i2cBurstWrite(uint8_t addr,uint8_t length,uint8_t *pData);
int8_t i2cBurstRead(uint8_t addr,uint8_t length,uint8_t *pData);

static uint8_t i2cSendStart(void); 
static void i2cSendStop(void); 
static uint8_t i2cSendByte(uint8_t data);

#endif
