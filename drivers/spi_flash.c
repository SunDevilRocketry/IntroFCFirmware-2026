#include "spi_flash.h"
#include <stddef.h>  // For NULL check if needed

// Command set
#define CMD_WRITE_ENABLE     0x06
#define CMD_READ_STATUS1     0x05
#define CMD_READ_DATA        0x03
#define CMD_PAGE_PROGRAM     0x02
#define CMD_SECTOR_ERASE     0x20 // 4KB Sector Erase
#define CMD_JEDEC_ID         0x9F

// CS control macros
#define CS_LOW(flash)   HAL_GPIO_WritePin((flash)->cs_port, (flash)->cs_pin, GPIO_PIN_RESET)
#define CS_HIGH(flash)  HAL_GPIO_WritePin((flash)->cs_port, (flash)->cs_pin, GPIO_PIN_SET)

// ==================== LOW-LEVEL SPI WRAPPERS (PRIVATE) ==================== //
static SPI_FLASH_STAT spi_transmit_byte(SPI_Flash_Handle* flash, uint8_t data) {
    if (flash == NULL || flash->hspi == NULL) 
        return SPI_FLASH_ERROR;
    
    if (HAL_SPI_Transmit(flash->hspi, &data, 1, 100) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

static SPI_FLASH_STAT spi_transmit_bytes(SPI_Flash_Handle* flash, uint8_t* data, uint16_t size) {
    if (flash == NULL || flash->hspi == NULL || data == NULL) 
        return SPI_FLASH_ERROR;
    
    if (HAL_SPI_Transmit(flash->hspi, data, size, 1000) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

static SPI_FLASH_STAT spi_receive_byte(SPI_Flash_Handle* flash, uint8_t* data) {
    if (flash == NULL || flash->hspi == NULL || data == NULL) 
        return SPI_FLASH_ERROR;
    
    uint8_t dummy = 0xFF;
    if (HAL_SPI_Receive(flash->hspi, data, 1, 100) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

static SPI_FLASH_STAT spi_receive_bytes(SPI_Flash_Handle* flash, uint8_t* data, uint16_t size) {
    if (flash == NULL || flash->hspi == NULL || data == NULL) 
        return SPI_FLASH_ERROR;
    
    uint8_t dummy = 0xFF;
    if (HAL_SPI_Receive(flash->hspi, data, size, 1000) == HAL_OK)
        return SPI_FLASH_OK;
    return SPI_FLASH_ERROR;
}

// ==================== PUBLIC FUNCTIONS ==================== //
/**
 * @brief Initialize the SPI flash driver
 * 
 * Must be called before using any other SPI flash functions. Associates a
 * flash device object with its SPI bus and CS pin. The CS pin is set HIGH
 * (deselected) initially since chip select is active low.
 * 
 * @param flash     Pointer to flash handle structure to initialize
 * @param hspi      Pointer to initialized SPI handle (from CubeMX)
 * @param cs_port   GPIO port for CS pin (e.g., GPIOA, GPIOB)
 * @param cs_pin    GPIO pin for CS pin (e.g., GPIO_PIN_4)
 */
void spi_flash_init(SPI_Flash_Handle* flash, SPI_HandleTypeDef* hspi, 
                    GPIO_TypeDef* cs_port, uint16_t cs_pin) {
    if (flash == NULL) return;
    
    flash->hspi = hspi;
    flash->cs_port = cs_port;
    flash->cs_pin = cs_pin;

    // Tride to test? The fuck?
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET); 
    //CS_HIGH(flash);  // Deselect chip initially (CS active low)
}

/**
 * @brief Read JEDEC ID from flash (command 0x9F)
 * 
 * Reads the 3-byte JEDEC ID which identifies the manufacturer, memory type,
 * and capacity. For W25Q128JV, expected values are:
 * - Manufacturer: 0xEF (Winbond)
 * - Memory Type:  0x40
 * - Capacity:     0x18 (128M-bit / 16M-byte)
 * 
 * The function handles CS control automatically
 * 
 * @param flash         Pointer to initialized flash handle
 * @param manufacturer  Pointer to store manufacturer ID byte
 * @param memory_type   Pointer to store memory type byte
 * @param capacity      Pointer to store capacity byte
 * @return SPI_FLASH_STAT
 */
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
    
    *manufacturer = rx[0];      // Should be 0xEF (Winbond)
    *memory_type = rx[1];       // Should be 0x40 (W25Q128JV)
    *capacity = rx[2];          // Should be 0x18 (128M-bit / 16M-byte)
    
    return SPI_FLASH_OK;
}

// ==================== WRITING DATA BLOCKS ==================== //   
// NOT TESTED NOR COMPLETED NOR EVEN VERIFIED. SIMPLY JUST SKIMMED THROUGH DATA SHEET NGL
// SIMPLY PROTOCAL COPYING 



// 1. Wait for the flash to finish any internal write/erase operations?
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms) {
    uint8_t status = 0;
    uint32_t start_tick = HAL_GetTick();
    
    while ((HAL_GetTick() - start_tick) < timeout_ms) {
        CS_LOW(flash);
        spi_transmit_byte(flash, CMD_READ_STATUS1);
        spi_receive_byte(flash, &status);
        CS_HIGH(flash);
        
        if ((status & 0x01) == 0) { // BUSY bit is 0 :3
            return SPI_FLASH_OK;
        }
        HAL_Delay(1);
    }
    return SPI_FLASH_TIMEOUT;
}

// 2. Enable Writing (Must be called before Erase or Page Program)
SPI_FLASH_STAT spi_flash_write_enable(SPI_Flash_Handle* flash) {
    CS_LOW(flash);
    spi_transmit_byte(flash, CMD_WRITE_ENABLE);
    CS_HIGH(flash);
    return SPI_FLASH_OK;
}

// 3. Erase a 4KB Sector (Flash bits must be set to 1 before they can be written to 0)
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address) {
    spi_flash_write_enable(flash);
    
    CS_LOW(flash);
    spi_transmit_byte(flash, CMD_SECTOR_ERASE);
    spi_transmit_byte(flash, (address >> 16) & 0xFF);
    spi_transmit_byte(flash, (address >> 8) & 0xFF);
    spi_transmit_byte(flash, address & 0xFF);
    CS_HIGH(flash);
    
    return spi_flash_wait_busy(flash, 400); // Wait up to 400ms for sector erase
}

// 4. Write a Page (Max 256 bytes)
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size) {
    if (size > 256) return SPI_FLASH_ERROR; // Cannot cross page boundary
    
    spi_flash_write_enable(flash);
    
    CS_LOW(flash);
    spi_transmit_byte(flash, CMD_PAGE_PROGRAM);
    spi_transmit_byte(flash, (address >> 16) & 0xFF);
    spi_transmit_byte(flash, (address >> 8) & 0xFF);
    spi_transmit_byte(flash, address & 0xFF);
    spi_transmit_bytes(flash, data, size);
    CS_HIGH(flash);
    
    return spi_flash_wait_busy(flash, 10); // Wait up to 10ms for page program
}

// 5. Read Data back
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size) {
    spi_flash_wait_busy(flash, 100);
    
    CS_LOW(flash);
    spi_transmit_byte(flash, CMD_READ_DATA);
    spi_transmit_byte(flash, (address >> 16) & 0xFF);
    spi_transmit_byte(flash, (address >> 8) & 0xFF);
    spi_transmit_byte(flash, address & 0xFF);
    spi_receive_bytes(flash, data, size);
    CS_HIGH(flash);
    
    return SPI_FLASH_OK;
}
