# Hardware Drivers (STM32)

This directory contains low-level hardware abstraction drivers. These drivers are designed to wrap STM32 HAL functions into APIs for application-level use.

## Available Drivers

| Driver | Description | Documentation |
| :--- | :--- | :--- |
| **USB Driver** | UART-based USB communication (Blocking & Interrupt) |  [View Docs](./READMEusb.md) |
| **SPI Flash** | Winbond W25Q128JV Logic & Data Management |  [View Docs](./READMEspi.md) |

---

## Quick Start Summary

### USB Communication
The USB module provides a simple interface for PC-to-MCU communication.
- **Primary Handle:** `huart3`
- **Modes:** Polling (Easy) or Interrupt (Non-blocking)
```c
usb_init(&huart3, true); // Init with Interrupts
usb_transmit("Hello", 5, 100);
```
👉 [USB Documentation & API Reference](./READMEusb.md)

### SPI Flash (W25Q128JV)
Handles the timing and command sequences for external Winbond Flash storage.
- **Capacity:** 128M-bit (16MB)
- **Features:** Safety ID checks and Page-boundary protection.
```c
spi_flash_init(&flash, &hspi1, GPIOD, GPIO_PIN_14);
spi_flash_read_data(&flash, 0x00, buffer, 100);
```
👉 [SPI Flash Documentation & API Reference](./READMEspi.md)

---

## Integration Notes
All drivers in this folder assume:
1. Peripherals (SPI, UART, GPIO) have been initialized via **STM32CubeMX**.
2. The project was tested using the **STM32H7 HAL Library**, confirm application on specific device
3. Global Interrupts are enabled in the NVIC for any module marked as "Interrupt-driven."

---

