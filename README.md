# IntroFCFirmware-2026

Flight Controller Firmware - 2026 Edition

**MCU:** ? 
**Architecture:** ARM Cortex-M7  
**PCB:** ?

**Description:** ?

---

## Working Directory Structure
app/        # Application code (flight firmware, terminal, etc.)
auto/       # STM32CubeMX generated code (not compiled)
drivers/    # Hardware abstraction layers and peripheral drivers
└── usb.c/usb.h/ # USB/UART communication driver
init/       # Microcontroller initialization and configuration
lib/        # Third-party libraries and middleware
mod/        # Hardware-specific code for SDR boards
test/       # Unit and integration tests


## Author

Sun Devil Rocketry - 2026
