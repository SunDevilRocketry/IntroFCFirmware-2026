## Command Module (`command.c`/`command.h`)

The Command module provides a protocol layer for terminal firmware, implementing a one-way handshake for all commands. It builds on top of the USB driver to provide structured communication w/ the client.

### Features
- **Simple Protocol**: Implements a consistent command → ACK → execution → response
- **Table-Driven Design**: Easy to add new commands w/ modifying core logic
- **Standardized Responses**: ACK (0x0A), success (echo command), error (0x0F)

### Important Notes
**Dependencies:**
(1) USB Driver Module (`usb.h`) - Provides all underlying communication
(2) CubeMX generated configuration

**[!IMPORTANT]**
The Command module expects the USB driver to be initialized **before** calling `command_init()`. All commands follow the exact same protocol flow.

### Command Protocol

Every command follows the same one-way handshake pattern as shown below in the neat table:

```
Client (Python)              STM32
     |                          |
     |── command byte ─────────>|
     |                          |── Send ACK_READY (0x0A)
     |<── ACK_READY ────────────|
     |                          |── Execute command handler
     |                          |── (whatever it may be)
     |<── result byte ──────────|── (Echos initial command byte signifying completion)
```

**Protocol Bytes:**
- **ACK_READY**: `0x0A` - Sent immediately after receiving any command
- **Success**: Echo of the original command byte (e.g., 0x01 for ping success)
- **Error**: `0x0F` for any failure (unknown command, execution error)

### Command Table

The module uses a table design for easy extensibility:

```c
static const struct {
    uint8_t cmd_byte;               // Command identifier (e.g., 0x01)
    cmd_result_t (*handler)(void);  // Function to execute
} command_table[] = {
    {CMD_PING,    cmd_ping},        // Ping command
    {CMD_CONNECT, cmd_connect},     // Connect command
    {0, NULL}                       // (DO NOT REMOVE)
};
```

### Available Commands

| Command | Byte | Description |
|---------|------|-------------|
| `CMD_PING` | 0x01 | Tests communication; always succeeds |
| `CMD_CONNECT` | 0x02 | Establishes connection, sends firmware version |

### API Reference

**Initialization**
```c
void command_init(uint32_t timeout_ms);
```
Call once during startup after USB driver initialization. Sets timeout for all USB operations.

**Command Processing**
```c
void command_process(uint8_t cmd_byte);
```
Main entry point for all commands. Usually best called from a superloop when a command byte is received. Handles the complete protocol:
1. Sends ACK_READY (0x0A) immediately
2. Validates command exists in table
3. Executes the appropriate handler
4. Sends result (echo command or 0x0F) back to client

### Adding New Commands

To add a new command, follow these three simple steps:

1. **Add command byte in `command.h`**:
```c
#define CMD_MY_COMMAND      0x04
```

2. **Write the handler in `command.c`**:
```c
cmd_result_t cmd_my_command(void)
{
    // Implement command logic
    // Return CMD_RESULT_SUCCESS or CMD_RESULT_FAILED
    
    if (do_something() == OK) {
        return CMD_RESULT_SUCCESS;
    }
    return CMD_RESULT_FAILED;
}
```

3. **Add to command table in `command.c`**:
```c
static const struct {
    uint8_t cmd_byte;
    cmd_result_t (*handler)(void);
} command_table[] = {
    {CMD_PING,    cmd_ping},
    {CMD_CONNECT, cmd_connect},
    {CMD_MY_COMMAND, cmd_my_command},  // Add here UwU
    {0, NULL}                   
};
```

That's all. Properly done commands will automatically follow the protocal.

### Example integration with `main.c`

```c
/* USER CODE BEGIN Includes */
#include "usb.h"
#include "command.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
uint8_t rx_byte;
/* USER CODE END PV */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    
    /* USER CODE BEGIN 2 */
    // Initialize USB driver (blocking mode)
    usb_init(&huart3, false);
    
    // Initialize command module with 1000ms timeout
    command_init(1000);
    
    // Send ready message
    usb_transmit((uint8_t*)"Ready\r\n", 7, 1000);
    /* USER CODE END 2 */

    while (1)
    {
        /* USER CODE BEGIN WHILE */
        // Poll for a single command byte
        if (usb_receive(&rx_byte, 1, 100) == USB_OK) {
            // Process the command (handles ACK, execution, and response)
            command_process(rx_byte);
        }
        /* USER CODE END WHILE */
    }
}
```

### Command Handlers Detail [fulfilling requirements]

**Ping Handler:**
```c
cmd_result_t cmd_ping(void)
{
    // Ping requires no additional actions
    // Success is implicit - completion byte will be sent by command_process()
    return CMD_RESULT_SUCCESS;
}
```

**Connect Handler:**
```c
cmd_result_t cmd_connect(void)
{
    // Send firmware version string (null-terminated)
    if (usb_transmit((uint8_t*)FIRMWARE_VERSION, 
                     sizeof(FIRMWARE_VERSION), g_timeout_ms) != USB_OK) {
        return CMD_RESULT_FAILED;
    }
    
    return CMD_RESULT_SUCCESS;
}
```

### Error Handling

The module handles several error cases automatically:

| Scenario | Response |
|----------|----------|
| Unknown command byte | Sends ACK_READY, then ACK_ERROR (0x0F) |
| Handler returns FAILED | Sends ACK_READY, then ACK_ERROR (0x0F) |
| USB transmission failure | Propagated through handler return value |
| Timeout during handler | Handler should return FAILED |

### Protocol Flow Examples

**Successful Ping:**
```
Client: 0x01
STM32:  0x0A (ACK_READY)
STM32:  (executes cmd_ping)
STM32:  0x01 (success - echoes command)
```

**Successful Connect:**
```
Client: 0x02
STM32:  0x0A (ACK_READY)
STM32:  "2026-02-21" (firmware version)
STM32:  0x02 (success - echoes command)
```

**Unknown Command:**
```
Client: 0xFF
STM32:  0x0A (ACK_READY)
STM32:  (command not found in table)
STM32:  0x0F (ACK_ERROR)
```


