#include <stdio.h>
#include <string.h>
#include <BMP390.h>
#include <stdint.h>

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
