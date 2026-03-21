## SPI Flash Driver Module

The SPI Flash driver (`spi_flash.c`/`spi_flash.h`) provides a logic abstraction for the **Winbond W25Q128JV** (128M-bit) Serial Flash memory, automating sequences required for erasing, programming, and busy-polling the hardware.

### Features
- **Automatic Busy-Polling**: Internal functions wait for the hardware to finish Erase/Program cycles before returning.
- **Page-Boundary Protection**: Prevents accidental data wrap-around during Page Program operations.
- **GAP-less Streaming**: Packs commands and addresses into single SPI transactions for performance.

### Important Notes
**Dependencies:**
1. **CubeMX Configuration**: Requires a SPI handle (Master mode) and a dedicated GPIO pin for Chip Select (CS).
2. **SPI Mode**: Must be configured as **Mode 0** (CPOL=Low, CPHA=1Edge) or **Mode 3**.

[!IMPORTANT]
**Voltage Level**: This driver is designed for 3.3V logic, specfically only going by the datasheet of **Winbond W25Q128JV**, so don't fry your stuff!

**Hardware Connections**:
Ensure the `/WP` (Write Protect) and `/HOLD` pins (if applicable) on the flash chip are tied to 3.3V (Logic High) to prevent the chip from ignoring commands or entering a pause state.

*This driver is designed for the STM32H7 series but uses standard HAL functions compatible with all STM32 families.*

### Driver Initialization
Initialize the driver by associating it with your SPI bus and the chosen Chip Select pin:

```c
SPI_Flash_Handle flash;
spi_flash_init(&flash, &hspi1, GPIOD, GPIO_PIN_14);
```

---

### Function Reference

#### Management Functions
```c
void spi_flash_init(SPI_Flash_Handle* flash, SPI_HandleTypeDef* hspi, GPIO_TypeDef* cs_port, uint16_t cs_pin);
SPI_FLASH_STAT read_device_id(SPI_Flash_Handle* flash, uint8_t* manufacturer, uint8_t* memory_type, uint8_t* capacity);
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms);
```
Function | Description
--- | ---
`spi_flash_init(...)` | Initializes the handle and ensures the CS pin starts HIGH.
`read_device_id(...)` | Reads the JEDEC ID. Expected Winbond 128JV: `0xEF, 0x40, 0x18`.
`spi_flash_wait_busy(...)` | Blocks until the flash chip completes its internal write/erase cycle.

#### Data Functions
```c
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address);
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size);
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size);
```
Function | Description
--- | ---
`spi_flash_erase_sector(...)` | Erases a 4KB sector. Required before writing new data.
`spi_flash_write_page(...)` | Writes up to 256 bytes. Errors out if crossing a page boundary.
`spi_flash_read_data(...)` | Reads any amount of data starting from the specified address.

---

### Error Handling
All functions return an **SPI_FLASH_STAT** enum to ensure robust error checking:
```c
typedef enum {
    SPI_FLASH_OK = 0,      // Operation successful
    SPI_FLASH_ERROR,       // Invalid parameters or HAL failure
    SPI_FLASH_TIMEOUT      // Device stuck in BUSY state
} SPI_FLASH_STAT;
```

---

### Sample Code

#### Example 1: Identity Verification

```c
uint8_t m, t, c;
if (read_device_id(&flash, &m, &t, &c) == SPI_FLASH_OK) {
    if (m == 0xEF && t == 0x40 && c == 0x18) {
        // W25Q128JV confirmed!
    }
}
```

#### Example 2: Sector Update (Erase -> Write -> Read)
Flash memory requires an erase cycle before new data can be written to a previously used address.
```c
uint32_t addr = 0x000000;
uint8_t data_to_write[] = "Flash Test";
uint8_t rx_buf[10];

// 1. Erase the 4KB sector containing the address
spi_flash_erase_sector(&flash, addr);

// 2. Write data to the page (Max 256 bytes)
spi_flash_write_page(&flash, addr, data_to_write, 10);

// 3. Read the data back to verify
spi_flash_read_data(&flash, addr, rx_buf, 10);

if (memcmp(data_to_write, rx_buf, 10) == 0) {
    // Data Integrity Verified
}
```

--- 

