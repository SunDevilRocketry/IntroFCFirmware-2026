#include <stdio.h>
#include <string.h>
#include <BMP390.h>
#include <stdint.h>
#include "stm32h7xx_hal_i2c.h"

uint16_t BMP390Addy = 0x76 << 8;

int baro_reg_write (uint16_t memaddy, uint8_t* writestuff){
    HAL_StatusTypeDef writeStatus = HAL_I2C_Mem_Write(*hi2c, memaddy, 8, writestuff, sizeof(writestuff), 10);
    if (writeStatus == HAL_OK) {
        return 1;
    }
    else {
        return 0;
    }
}

BARO_STATUS baro_reg_read (uint16_t devAddress, uint16_t memAddress, uint8_t *pData, uint8_t size) {
    HAL_StatusTypeDef readStatus = HAL_I2C_Mem_Read(*hi2c, devaddy, memaddy, I2C_MEMADD_SIZE_8BIT, pData, size);
    if (readStatus != HAL_OK) {
        return BARO_FAIL;
    }
    else {
        return BARO_OK;
    }
}