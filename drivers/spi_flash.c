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

    CS_HIGH(flash);  // Ensure CS starts HIGH (Deselected)
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
/**
 * @brief Polls Status Register 1 until the BUSY bit (S0) is cleared.
 * @details The Winbond chip sets S0=1 during internal Erase/Program cycles.
 *          Maximum timeout is critical for Erase operations (~400ms).
 */
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms) {
    uint8_t status;
    uint32_t start_tick = HAL_GetTick();
    
    while ((HAL_GetTick() - start_tick) < timeout_ms) {
        status = 0xFF; // Default to BUSY if SPI receive fails
        
        CS_LOW(flash);
        spi_transmit_byte(flash, CMD_READ_STATUS1);
        spi_receive_byte(flash, &status);
        CS_HIGH(flash);
        
        if ((status & 0x01) == 0) { // BUSY bit is 0, device is ready
            return SPI_FLASH_OK;
        }
        HAL_Delay(1);
    }
    return SPI_FLASH_TIMEOUT;
}

/**
 * @brief Sets the Write Enable Latch (WEL) bit.
 * @details This command must be sent before every Write or Erase operation.
 *          The WEL bit is automatically reset after the write finishes.
 */
SPI_FLASH_STAT spi_flash_write_enable(SPI_Flash_Handle* flash) {
    CS_LOW(flash);
    spi_transmit_byte(flash, CMD_WRITE_ENABLE);
    CS_HIGH(flash);
    return SPI_FLASH_OK;
}

/**
 * @brief Erases a 4KB Sector starting at 'address'.
 * @details Flash memory bits can only be programmed from 1 to 0. To write 
 *          new data, the sector must first be erased back to 0xFF.
 *          Packing cmd+addr into a 4-byte buffer ensures a gapless SPI stream.
 */
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address) {
    spi_flash_write_enable(flash);
    
    // Pack command and address into a single SPI transaction determined by datasheet
    uint8_t cmd[4];
    cmd[0] = CMD_SECTOR_ERASE;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    CS_LOW(flash);
    spi_transmit_bytes(flash, cmd, 4);
    CS_HIGH(flash);
    
    /* Datasheet Table 9.6: tSE (Sector Erase) Typ=45ms, Max=400ms. 
       Use 500ms to allow for HAL/System overhead. */
    return spi_flash_wait_busy(flash, 500); 
}

/**
 * @brief Writes up to 256 bytes to a specific page.
 * @details Page Program (02h) cannot cross a page boundary (address & 0xFF == 0).
 *          If address + size > 256, the chip wraps data to the start of the current page.
 *          This function blocks further input to prevent unintended wrap-around.
 */
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size) {
    // Strictly prevent page boundary wrap-around
    if (size == 0 || size > 256 || ((address & 0xFF) + size > 256)) {
        return SPI_FLASH_ERROR; 
    }
    
    spi_flash_write_enable(flash);
    
    // Pack command and address into a single SPI transaction
    uint8_t cmd[4];
    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    CS_LOW(flash);
    spi_transmit_bytes(flash, cmd, 4);
    spi_transmit_bytes(flash, data, size); // Stream the payload
    CS_HIGH(flash);
    
    /* Datasheet Table 9.6: tPP (Page Program) Typ=0.4ms, Max=3ms. 
       We use 10ms for safety. */
    return spi_flash_wait_busy(flash, 10); 
}

/**
 * @brief Reads a stream of data starting at 'address'.
 * @details Unlike Page Program, Read Data (03h) has no page boundaries and 
 *          will automatically increment the address through the entire chip.
 */
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size) {
    // Ensure flash isn't busy before attempting to read
    if (spi_flash_wait_busy(flash, 100) != SPI_FLASH_OK) {
        return SPI_FLASH_TIMEOUT;
    }
    
    // Pack command and address into a single SPI transaction
    uint8_t cmd[4];
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    CS_LOW(flash);
    spi_transmit_bytes(flash, cmd, 4);
    spi_receive_bytes(flash, data, size); // Stream the read payload
    CS_HIGH(flash);
    
    return SPI_FLASH_OK;
}
