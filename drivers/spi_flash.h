#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include "main.h"      /* For HAL types: SPI_HandleTypeDef, GPIO_TypeDef */
#include <stdint.h>
#include <stddef.h>


typedef enum {
    SPI_FLASH_OK = 0,      /* Operation completed successfully                      */
    SPI_FLASH_ERROR,       /* Operation failed (HAL error, invalid params, etc.)    */
    SPI_FLASH_TIMEOUT      /* Operation timed out (flash busy, no response)         */
} SPI_FLASH_STAT;

/**
 * @brief SPI Flash device handle
 * 
 * Contains all info needed to communicate with a specific SPI flash chip.
 * This allows the driver to support multiple flash chips on different SPI buses or w/ different CS pins.
 */
typedef struct {
    SPI_HandleTypeDef* hspi;       /* Pointer to SPI handle (from CubeMX)    */
    GPIO_TypeDef* cs_port;         /* GPIO port for Chip Select pin          */
    uint16_t cs_pin;               /* GPIO pin for Chip Select pin           */
} SPI_Flash_Handle;

/*============================================================================
 * Driver Initialization
 *============================================================================*/

/**
 * @brief Initialize the SPI flash driver
 * 
 * Must be called before using any other SPI flash functions. Associates a
 * flash device object w/ its SPI bus and CS pin. (Default CS pin set HIGH).
 * 
 * @param flash     Pointer to flash handle structure to initialize
 * @param hspi      Pointer to initialized SPI handle (from CubeMX)
 * @param cs_port   GPIO port for CS pin (e.g., GPIOA, GPIOB)
 * @param cs_pin    GPIO pin for CS pin (e.g., GPIO_PIN_4)
 * 
 */
void spi_flash_init(SPI_Flash_Handle* flash, 
                    SPI_HandleTypeDef* hspi, 
                    GPIO_TypeDef* cs_port, 
                    uint16_t cs_pin);

/*============================================================================
 * High-Level Flash Commands
 *============================================================================*/

/**
 * @brief Read JEDEC ID from flash (command 0x9F)
 * 
 * Reads the 3-byte JEDEC ID which identifies the manufacturer, memory type,
 * and capacity. For W25Q128JV, expected values are:
 * - Manufacturer: 0xEF (Winbond)
 * - Memory Type:  0x40
 * - Capacity:     0x18 (128M-bit / 16M-byte)
 * 
 * @param flash         Pointer to initialized flash handle
 * @param manufacturer  Pointer to store manufacturer ID byte
 * @param memory_type   Pointer to store memory type byte
 * @param capacity      Pointer to store capacity byte
 * @return SPI_FLASH_STAT
 */
SPI_FLASH_STAT read_device_id(SPI_Flash_Handle* flash, 
                               uint8_t* manufacturer, 
                               uint8_t* memory_type, 
                               uint8_t* capacity);
/* ============================================================================
   Data Block Management (Erase/Write/Read)
   ============================================================================ */

/**
 * @brief Poll the Status Register until the BUSY bit is cleared
 * @param timeout_ms Maximum time to wait
 */
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms);

/**
 * @brief Sets the Write Enable Latch (WEL) bit. 
 * Must be called before every Erase or Program command.
 */
SPI_FLASH_STAT spi_flash_write_enable(SPI_Flash_Handle* flash);

/**
 * @brief Erase a 4KB sector. 
 * Required before writing new data to an address that isn't already 0xFF.
 * @param address Any address within the 4KB sector to be erased
 */
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address);

/**
 * @brief Write up to 256 bytes to a single page.
 * @note If 'size' + 'address' crosses a page boundary (256 bytes), the 
 *       data will wrap around to the beginning of the same page.
 */
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, 
                                    uint8_t* data, uint16_t size);

/**
 * @brief Read a block of data starting from address.
 * No size limit (except physical chip capacity).
 */
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, 
                                   uint8_t* data, uint16_t size);

#endif


