#include "command.h"
#include <string.h>

// Private variables begin //

static uint32_t g_timeout_ms = 1000;    // Default timeout for USB operations

// Private variables end //

// Command table definition begin //

/**
 * @brief Command dispatch table
 * 
 * Maps command bytes to their corresponding handler functions.
 * Table is terminated with a sentinel entry (0, NULL).
 * Add new commands here by inserting a new row.
 */
static const struct {
    uint8_t cmd_byte;               // Command byte received from client
    cmd_result_t (*handler)(void);  // Handler function to execute
} command_table[] = {
    {CMD_PING,    cmd_ping},        // Ping command - tests communication 
    {CMD_CONNECT, cmd_connect},     // Connect command - sends firmware version
    {0, NULL}                   
};

// Command table definition end //

// Private function prototypes begin //

/**
 * @brief Find command index in dispatch table
 * @param cmd_byte: Command byte to search for
 * @return int: Index in command_table if found, -1 if not found
 */
static int find_command_index(uint8_t cmd_byte);

// Private function prototypes end //

// Public Functions begin //

void command_init(uint32_t timeout_ms)
{
    g_timeout_ms = timeout_ms;
}

void command_process(uint8_t cmd_byte)
{
    int cmd_index = find_command_index(cmd_byte);
    
    // Step 1: Send ACK_READY (0x0A) to acknowledge
    uint8_t ack = ACK_READY;
    usb_transmit(&ack, 1, g_timeout_ms);
    
    if (cmd_index >= 0) {
        // Known command - execute handler
        cmd_result_t result = command_table[cmd_index].handler();

        if (result == CMD_RESULT_SUCCESS) {
            // Step 3a: Command succeeded - send completion byte (original command)
            usb_transmit(&cmd_byte, 1, g_timeout_ms);
        } else {
            // Step 3b: Command failed - send error code (0x0F)
            uint8_t error = ACK_ERROR;
            usb_transmit(&error, 1, g_timeout_ms);
        }
    } else {
        // Unknown command - send error code
        uint8_t error = ACK_ERROR;
        usb_transmit(&error, 1, g_timeout_ms);
    }
}

// Public Functions end //

// Command Handlers begin //

cmd_result_t cmd_ping(void)
{
    // Ping requires no additional actions
    // Success is implicit - completion byte will be sent by command_process()
    return CMD_RESULT_SUCCESS;
}

cmd_result_t cmd_connect(void)
{
    // Send firmware version string (null-terminated)
    if (usb_transmit((uint8_t*)FIRMWARE_VERSION, 
                     sizeof(FIRMWARE_VERSION), g_timeout_ms) != USB_OK) {
        // Transmission failed - report error
        return CMD_RESULT_FAILED;
    }
    
    // Version sent successfully
    return CMD_RESULT_SUCCESS;
}

// Command Handlers end //

// Private Functions begin //

static int find_command_index(uint8_t cmd_byte)
{
    // Iterate through command table until sentinel is reached
    for (int i = 0; command_table[i].handler != NULL; i++) {
        if (command_table[i].cmd_byte == cmd_byte) {
            return i;  // Found matching command
        }
    }
    return -1;  // Command not found in table
}

// Private Functions end //
