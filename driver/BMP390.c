#include "BMP390.h"


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

BARO_STATUS baro_read_data (BARO_DATA* baro_data_out_ptr)
{
    uint32_t pressure = 0;
    uint8_t buffer = 0;

    BARO_STATUS status = BARO_OK;

    status = baro_read_reg(REG_PRESS_23_16, &buffer, 8);
    if (status != BARO_OK)
    {
        return status;
    }
    pressure += buffer;
    pressure = pressure << 8;

    status = baro_read_reg(REG_PRESS_15_8, &buffer, 8);
    if (status != BARO_OK)
    {
        return status;
    }
    pressure += buffer;
    pressure = pressure << 8;

    status = baro_read_reg(REG_PRESS_7_0, &buffer, 8);
    if (status != BARO_OK)
    {
        return status;
    }
    pressure += buffer;

    baro_data_out_ptr->pressure = pressure;

    return BARO_OK;
}