#include <stdio.h>
#include <string.h>
#include <BMP390.h>
#include <stdint.h>
#include "main.h"
#include "stm32h7xx_hal_i2c.h"



BARO_STATUS baro_reg_write (uint16_t memAddress, uint8_t *pData, uint8_t size){
    HAL_StatusTypeDef writeStatus = HAL_I2C_Mem_Write(&hi2c, BMP390_DEV_ADDR, memAddress, I2C_MEMADD_SIZE_8BIT, pData, size, 1);
    if (writeStatus != HAL_OK) {
        return BARO_FAIL;
    }
    else {
        return BARO_OK;
    }
}

BARO_STATUS baro_reg_read (uint16_t memAddress, uint8_t *pData, uint8_t size) {
    HAL_StatusTypeDef readStatus = HAL_I2C_Mem_Read(&hi2c, BMP390_DEV_ADDR, memAddress, I2C_MEMADD_SIZE_8BIT, pData, size, 1);
    if (readStatus != HAL_OK) {
        return BARO_FAIL;
    }
    else {
        return BARO_OK;
    }
}

BARO_STATUS validate_dev_id ()
{
    uint8_t dev_id;
    
    // Read device ID from baro
    HAL_StatusTypeDef status = baro_red_read(REG_CHIP_ID, &dev_id, sizeof(char));
    if (status != HAL_OK) {
        return BARO_FAIL;
    }
    else {
        return BARO_OK;
    }

    // Validate
    if (dev_id == CHIP_ID_DEFAULT) {
        return BARO_OK;
    }
    else {
        return BARO_FAIL;
    }
}