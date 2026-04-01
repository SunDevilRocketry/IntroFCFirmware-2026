#include "spi_flash.h"
#include <stddef.h>  /* NULL */

// Command set
#define CMD_WRITE_ENABLE     0x06
#define CMD_READ_STATUS1     0x05
#define CMD_READ_DATA        0x03
#define CMD_PAGE_PROGRAM     0x02
#define CMD_SECTOR_ERASE     0x20 // 4KB Sector Erase
#define CMD_JEDEC_ID         0x9F

// Timeout constants
#define SPI_FLASH_TIMEOUT_DEFAULT_MS    10
#define SPI_FLASH_TIMEOUT_ERASE_MS      500

// CS control macros
#define CS_LOW(flash)   HAL_GPIO_WritePin((flash)->cs_port, (flash)->cs_pin, GPIO_PIN_RESET)
#define CS_HIGH(flash)  HAL_GPIO_WritePin((flash)->cs_port, (flash)->cs_pin, GPIO_PIN_SET)

// ==================== LOW-LEVEL SPI WRAPPERS (PRIVATE) ==================== //
/**
 * @brief Sends a single byte over SPI.
 */
static SPI_FLASH_STAT spi_transmit_byte(SPI_Flash_Handle* flash, uint8_t data) {
    if (flash == NULL || flash->hspi == NULL) 
        return SPI_FLASH_ERROR;
    
    if (HAL_SPI_Transmit(flash->hspi, &data, 1, 100) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

/**
 * @brief Sends a block of data over SPI. Used for headers and payloads.
 */
static SPI_FLASH_STAT spi_transmit_bytes(SPI_Flash_Handle* flash, uint8_t* data, uint16_t size) {
    if (flash == NULL || flash->hspi == NULL || data == NULL) 
        return SPI_FLASH_ERROR;
    
    if (HAL_SPI_Transmit(flash->hspi, data, size, 1000) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

/**
 * @brief Receives a single byte from SPI.
 */
static SPI_FLASH_STAT spi_receive_byte(SPI_Flash_Handle* flash, uint8_t* data) {
    if (flash == NULL || flash->hspi == NULL || data == NULL) 
        return SPI_FLASH_ERROR;
    
    if (HAL_SPI_Receive(flash->hspi, data, 1, 100) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

/**
 * @brief Receives a block of data from SPI. Used for memory reads.
 */
static SPI_FLASH_STAT spi_receive_bytes(SPI_Flash_Handle* flash, uint8_t* data, uint16_t size) {
    if (flash == NULL || flash->hspi == NULL || data == NULL) 
        return SPI_FLASH_ERROR;
    
    if (HAL_SPI_Receive(flash->hspi, data, size, 1000) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

// ==================== PUBLIC FUNCTIONS ==================== //

/* CS starts HIGH (active low), then JEDEC ID is read to verify SPI comms before returning. */
SPI_FLASH_STAT spi_flash_init(SPI_Flash_Handle* flash, SPI_HandleTypeDef* hspi, 
                    GPIO_TypeDef* cs_port, uint16_t cs_pin) {
    
    if (flash == NULL || hspi == NULL || cs_port == NULL){
        return SPI_FLASH_ERROR;
    }

    flash->hspi = hspi;
    flash->cs_port = cs_port;
    flash->cs_pin = cs_pin;

    CS_HIGH(flash);

    /* Verify SPI comms and correct device by reading JEDEC ID */
    uint8_t mfr, mem_type, cap;
    if (read_device_id(flash, &mfr, &mem_type, &cap) != SPI_FLASH_OK){
        return SPI_FLASH_ERROR;
    }
    if (mfr != W25Q128JV_MFR_ID || mem_type != W25Q128JV_MEM_TYPE || cap != W25Q128JV_CAPACITY){
        return SPI_FLASH_ERROR;
    }   
    return SPI_FLASH_OK;
}

/* Sends CMD_JEDEC_ID (0x9F) and clocks out 3 response bytes in a single CS transaction. */
SPI_FLASH_STAT read_device_id(SPI_Flash_Handle* flash, uint8_t* manufacturer, 
                              uint8_t* memory_type, uint8_t* capacity) {
    // Validate parameters
    if (flash == NULL || manufacturer == NULL || memory_type == NULL || capacity == NULL) {
        return SPI_FLASH_ERROR;
    }
    
    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t rx[3];
    
    CS_LOW(flash);
    
    if (spi_transmit_byte(flash, cmd) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    
    if (spi_receive_bytes(flash, rx, 3) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    
    CS_HIGH(flash);
    
    *manufacturer = rx[0];
    *memory_type = rx[1];
    *capacity = rx[2];
    
    return SPI_FLASH_OK;
}

// ==================== WRITING DATA BLOCKS ==================== //

/* Polls STATUS1 register, S0 is the BUSY bit per datasheet section 7.1. Returns ERROR on SPI failure, TIMEOUT if limit exceeded. */
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms) {
    uint8_t status;
    uint32_t start_tick = HAL_GetTick();
    
    while ((HAL_GetTick() - start_tick) < timeout_ms) {
        
        CS_LOW(flash);
        if (spi_transmit_byte(flash, CMD_READ_STATUS1) != SPI_FLASH_OK) {
            CS_HIGH(flash);
            return SPI_FLASH_ERROR;
        }
        if (spi_receive_byte(flash, &status) != SPI_FLASH_OK) {
            CS_HIGH(flash);
            return SPI_FLASH_ERROR;
        }
        CS_HIGH(flash);
        
        if ((status & 0x01) == 0) { // BUSY bit is 0, device is ready
            return SPI_FLASH_OK;
        }
        HAL_Delay(1);
    }
    return SPI_FLASH_TIMEOUT;
}

/* WEL bit is automatically cleared by hardware after each Write or Erase completes. */
SPI_FLASH_STAT spi_flash_write_enable(SPI_Flash_Handle* flash) {
    CS_LOW(flash);
    if (spi_transmit_byte(flash, CMD_WRITE_ENABLE) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    CS_HIGH(flash);
    return SPI_FLASH_OK;
}

/* cmd+addr packed into single 4-byte buffer to ensure good SPI stream per datasheet. */
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address) {
    if (spi_flash_write_enable(flash) != SPI_FLASH_OK){
        return SPI_FLASH_ERROR;
    }
    // Pack command and address into a single SPI transaction determined by datasheet
    uint8_t cmd[4];
    cmd[0] = CMD_SECTOR_ERASE;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    CS_LOW(flash);
    if (spi_transmit_bytes(flash, cmd, 4) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    CS_HIGH(flash);
    
    /* Datasheet Table 9.6: tSE (Sector Erase) Typ=45ms, Max=400ms. */
    return spi_flash_wait_busy(flash, SPI_FLASH_TIMEOUT_ERASE_MS); 
}

/* Hardware silently wraps writes past page boundary + guard check prevents this before any SPI transaction. */
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size) {
    // Strictly prevent page boundary wrap-around
    if (size == 0 || size > 256 || ((address & 0xFF) + size > 256)) {
        return SPI_FLASH_ERROR; 
    }
    
    if (spi_flash_write_enable(flash) != SPI_FLASH_OK){
        return SPI_FLASH_ERROR;
    }
    
    // Pack command and address into a single SPI transaction
    uint8_t cmd[4];
    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    CS_LOW(flash);
    if (spi_transmit_bytes(flash, cmd, 4) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    if (spi_transmit_bytes(flash, data, size) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    CS_HIGH(flash);
    
    /* Datasheet Table 9.6: tPP (Page Program) Typ=0.4ms, Max=3ms. */
    return spi_flash_wait_busy(flash, SPI_FLASH_TIMEOUT_DEFAULT_MS); 
}

/* Chunks data into page-aligned spi_flash_write_page calls. Does NOT erase — uncleared bits cause silent AND corruption. */
SPI_FLASH_STAT spi_flash_write(SPI_Flash_Handle* flash, uint32_t address,
                                uint8_t* data, uint32_t size) {
    if (flash == NULL || data == NULL || size == 0){
        return SPI_FLASH_ERROR;
    }

    if ((address + size) > W25Q128JV_FLASH_SIZE){
        return SPI_FLASH_ERROR;
    }
    uint32_t bytes_written = 0;

    while (bytes_written < size) {
        /* How many bytes remain in the current page from this address */
        uint16_t page_offset = address & 0xFF;
        uint16_t space_in_page = 256 - page_offset;

        /* Write whichever is smaller: remaining data or space left in page */
        uint16_t chunk = (size - bytes_written) < space_in_page
                         ? (uint16_t)(size - bytes_written)
                         : space_in_page;

        SPI_FLASH_STAT ret = spi_flash_write_page(flash, address, data + bytes_written, chunk);
        if (ret != SPI_FLASH_OK){
            return ret;
        }

        address      += chunk;
        bytes_written += chunk;
    }

    return SPI_FLASH_OK;
}

/* CMD_READ_DATA auto-increments address across page & sector boundaries through the full chip. */
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size) {
    // Ensure flash isn't busy before attempting to read
    if (spi_flash_wait_busy(flash, 100) != SPI_FLASH_OK) {
        return SPI_FLASH_TIMEOUT;
    }

    if (size == 0 || (address + size) > W25Q128JV_FLASH_SIZE) {
        return SPI_FLASH_ERROR;
    }
    
    // Pack command and address into a single SPI transaction
    uint8_t cmd[4];
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    CS_LOW(flash);
    if (spi_transmit_bytes(flash, cmd, 4) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    if (spi_receive_bytes(flash, data, size) != SPI_FLASH_OK) {
        CS_HIGH(flash);
        return SPI_FLASH_ERROR;
    }
    CS_HIGH(flash);
    
    return SPI_FLASH_OK;
}
