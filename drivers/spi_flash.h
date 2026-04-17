#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include "main.h"      /* For HAL types: SPI_HandleTypeDef, GPIO_TypeDef */
#include <stdint.h>
#include <stddef.h>

/* W25Q128JV Device Identity */
#define W25Q128JV_MFR_ID     0xEF
#define W25Q128JV_MEM_TYPE   0x40
#define W25Q128JV_CAPACITY   0x18
#define W25Q128JV_FLASH_SIZE 0x1000000   /* 128M-bit / 16M-byte */

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
 * @brief Initialize the SPI flash driver and verify device identity.
 *
 * Must be called before any other SPI flash functions. Associates the handle
 * with its SPI bus and CS pin, then reads the JEDEC ID to confirm SPI comms
 * and correct device are working before returning.
 *
 * @param flash     Pointer to flash handle structure to initialize
 * @param hspi      Pointer to initialized SPI handle (from CubeMX)
 * @param cs_port   GPIO port for CS pin (e.g., GPIOA, GPIOB)
 * @param cs_pin    GPIO pin for CS pin (e.g., GPIO_PIN_4)
 * @return SPI_FLASH_OK on success, SPI_FLASH_ERROR if SPI failed or wrong device detected
 */
SPI_FLASH_STAT spi_flash_init(SPI_Flash_Handle* flash, 
                    SPI_HandleTypeDef* hspi, 
                    GPIO_TypeDef* cs_port, 
                    uint16_t cs_pin);

/*============================================================================
 * High-Level Flash Commands
 *============================================================================*/

/**
 * @brief Read the 3-byte JEDEC ID from the flash chip (command 0x9F).
 * @details Identifies manufacturer, memory type, and capacity.
 *          Expected values for W25Q128JV:
 *          - Manufacturer: 0xEF (Winbond)
 *          - Memory Type:  0x40
 *          - Capacity:     0x18 (128M-bit / 16M-byte)
 * @note CS control is handled automatically within this function.
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
 * @brief Poll Status Register 1 until the BUSY bit (S0) is cleared.
 * @param flash      Pointer to initialized flash handle
 * @param timeout_ms Maximum time to wait in milliseconds
 * @return SPI_FLASH_OK when ready, SPI_FLASH_ERROR on SPI failure, SPI_FLASH_TIMEOUT if limit exceeded
 */
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms);

/**
 * @brief Set the Write Enable Latch (WEL) bit.
 * @note The WEL bit is automatically cleared by hardware after each Write or Erase completes.
 * @param flash  Pointer to initialized flash handle
 * @return SPI_FLASH_STAT
 */
SPI_FLASH_STAT spi_flash_write_enable(SPI_Flash_Handle* flash);

/**
 * @brief Erase the 4KB sector containing the given address.
 * @note Required before writing to any address that isn't already 0xFF.
 *       Any address within the target sector is valid b/c the chip aligns to the sector boundary.
 * @param flash    Pointer to initialized flash handle
 * @param address  Any address within the 4KB sector to erase
 * @return SPI_FLASH_STAT
 */
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address);

/**
 * @brief Write up to 256 bytes to a single page.
 * @note Will return SPI_FLASH_ERROR if address + size crosses a 256-byte page boundary.
 *       Target region must be erased before calling or silent AND corruption will occur.
 * @param flash    Pointer to initialized flash handle
 * @param address  Starting address (must not cross a page boundary with size)
 * @param data     Pointer to data buffer
 * @param size     Number of bytes to write (1-256)
 * @return SPI_FLASH_STAT
 */
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, 
                                    uint8_t* data, uint16_t size);
/**
 * @brief Write any amount of data starting at address, handling page boundaries automatically.
 * @warning Target region must be erased before calling. Writing to unerased flash causes
 *          silent AND corruption, so the chip will not report an error.
 * @param flash    Pointer to initialized flash handle
 * @param address  Starting address
 * @param data     Pointer to data buffer
 * @param size     Total number of bytes to write
 * @return SPI_FLASH_STAT
 */
SPI_FLASH_STAT spi_flash_write(SPI_Flash_Handle* flash, uint32_t address,
                                uint8_t* data, uint32_t size);
/**
 * @brief Read a block of data starting from the given address.
 * @note Reads have no page boundaries and auto-increment through the full chip.
 *       Address + size must not exceed W25Q128JV_FLASH_SIZE.
 * @param flash    Pointer to initialized flash handle
 * @param address  Starting address to read from
 * @param data     Pointer to buffer to store read data
 * @param size     Number of bytes to read
 * @return SPI_FLASH_STAT
 */
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, 
                                   uint8_t* data, uint16_t size);

#endif
