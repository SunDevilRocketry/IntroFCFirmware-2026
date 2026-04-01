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

> [!IMPORTANT]
> **Voltage Level**: This driver is designed for 3.3V logic, specifically going by the datasheet of **Winbond W25Q128JV**, so don't fry your stuff!

**Hardware Connections**:
Ensure the `/WP` (Write Protect) and `/HOLD` pins (if applicable) on the flash chip are tied to 3.3V (Logic High) to prevent the chip from ignoring commands or entering a pause state. If your project requires runtime write protection control, these pins would need to be wired to GPIO outputs instead.

*This driver is designed using the STM32H7 series but uses standard HAL functions compatible with all STM32 families.*

### Driver Initialization
Initialize the driver by associating it with your SPI bus and the chosen Chip Select pin. Note that `spi_flash_init` returns `SPI_FLASH_STAT`(detailed below), reading the JEDEC ID internally to verify SPI comms and correct device before returning, so always check the return value.

```c
SPI_Flash_Handle flash;
if (spi_flash_init(&flash, &hspi1, GPIOD, GPIO_PIN_14) != SPI_FLASH_OK) {
    // SPI misconfigured or wrong device detected
}
```

> [!NOTE]
> Ensure your CubeMX GPIO configuration sets the CS pin default output level to **High**. This prevents the flash chip from interpreting the window between GPIO init and `spi_flash_init` as the start of a transaction.

---

### Function Reference

#### Management Functions
```c
SPI_FLASH_STAT spi_flash_init(SPI_Flash_Handle* flash, SPI_HandleTypeDef* hspi, GPIO_TypeDef* cs_port, uint16_t cs_pin);
SPI_FLASH_STAT read_device_id(SPI_Flash_Handle* flash, uint8_t* manufacturer, uint8_t* memory_type, uint8_t* capacity);
SPI_FLASH_STAT spi_flash_wait_busy(SPI_Flash_Handle* flash, uint32_t timeout_ms);
```

Function | Description
--- | ---
`spi_flash_init(...)` | Initializes the handle, asserts CS HIGH, and verifies device identity via JEDEC ID. Returns `SPI_FLASH_ERROR` if SPI fails or wrong device detected.
`read_device_id(...)` | Reads the 3-byte JEDEC ID identifying manufacturer, memory type, and capacity. Expected for W25Q128JV: `0xEF, 0x40, 0x18`.
`spi_flash_wait_busy(...)` | Blocks until the flash chip completes its internal write/erase cycle, or returns `SPI_FLASH_TIMEOUT` if the limit is exceeded.

#### Data Functions
```c
SPI_FLASH_STAT spi_flash_erase_sector(SPI_Flash_Handle* flash, uint32_t address);
SPI_FLASH_STAT spi_flash_write_page(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size);
SPI_FLASH_STAT spi_flash_write(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint32_t size);
SPI_FLASH_STAT spi_flash_read_data(SPI_Flash_Handle* flash, uint32_t address, uint8_t* data, uint16_t size);
```

Function | Description
--- | ---
`spi_flash_erase_sector(...)` | Erases a 4KB sector. Required before writing to any address not already `0xFF`.
`spi_flash_write_page(...)` | Writes up to 256 bytes to a single page. Returns `SPI_FLASH_ERROR` if crossing a page boundary.
`spi_flash_write(...)` | Writes any amount of data across page boundaries automatically. Caller must erase first.
`spi_flash_read_data(...)` | Reads any amount of data from the specified address up to the chip boundary.

---

### Error Handling
All functions return a **SPI_FLASH_STAT** enum:
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
    if (m == W25Q128JV_MFR_ID && t == W25Q128JV_MEM_TYPE && c == W25Q128JV_CAPACITY) {
        // W25Q128JV confirmed!
    }
}
```

#### Example 2: Sector Update (Erase -> Write -> Read)
Flash memory bits can only be programmed from `1` to `0` — erasing resets them back to `0xFF`. Always erase before writing to a previously used address.

> [!NOTE]
> The read-back verify step below is illustrative application code showing one way to confirm data integrity. It is not part of the driver itself.

```c
uint32_t addr = 0x000000;
uint8_t data_to_write[] = "Flash Test";
uint8_t rx_buf[10];

// 1. Erase the 4KB sector containing the address
spi_flash_erase_sector(&flash, addr);

// 2. Write data, handling page boundaries automatically
spi_flash_write(&flash, addr, data_to_write, 10);

// 3. Read the data back to verify (illustrative — application code)
spi_flash_read_data(&flash, addr, rx_buf, 10);

if (memcmp(data_to_write, rx_buf, 10) == 0) {
    // Data Integrity Verified
}
```

--- 

