#ifndef COMMAND_H
#define COMMAND_H

#include "usb.h"        // For USB communication functions
#include <stdint.h>
#include <stdbool.h>

// Firmware version //

#define FIRMWARE_VERSION "2026-02-21"  /**< Firmware version string in YYYY-MM-DD format */

// Command byte definitions begin //

#define CMD_PING        0x01    
#define CMD_CONNECT     0x27 
// [Add more if desired]   

// Command byte definitions end //

// Protocol byte definitions begin //

#define ACK_READY       0x0A    // Ready to process - sent immediately after command 
#define ACK_SUCCESS     0x00    // Command succeeded (kinda useless [for future use] 
                                // given return of same command byte as completion.
#define ACK_ERROR       0x0F    // Command failed - sent for unknown commands

// Protocol byte definitions end //


// Command result enum //

/**
 * @brief Result codes returned by command handlers
 */
typedef enum {
    CMD_RESULT_SUCCESS,  // Command executed successfully 
    CMD_RESULT_FAILED    // Command execution failed
} cmd_result_t;

// Command handler prototype //

/**
 * @brief Function pointer type for all command handlers
 * @return cmd_result_t indicating success or failure
 */
typedef cmd_result_t (*cmd_handler_t)(void);

// Public Functions begin //

/**
 * @brief Initialize the command module
 * @param timeout_ms: Default timeout for USB operations in milliseconds
 * @note Must be called before using any other command functions
 */
void command_init(uint32_t timeout_ms);

/**
 * @brief Process an incoming command byte from client
 * 
 * Implements the one-way handshake protocol:
 * 1. Sends ACK_READY (0x0A) immediately
 * 2. Looks up command in dispatch table
 * 3. Executes handler if found, else sends ACK_ERROR (0x0F)
 * 4. Sends completion byte (original command) on success
 * 
 * @param cmd_byte: Command byte received from Python client
 */
void command_process(uint8_t cmd_byte);

// Public Functions end //

// Command Handlers begin //

/**
 * @brief Ping command handler
 * 
 * Tests basic communication. Simply returns success without
 * any additional actions. Completion byte (0x01) will be sent
 * by command_process().
 * 
 * @return CMD_RESULT_SUCCESS always
 */
cmd_result_t cmd_ping(void);

/**
 * @brief Connect command handler
 * 
 * Performs connection handshake by sending firmware version
 * string to client. The version string is null-terminated.
 * Completion byte (0x27) will be sent by command_process().
 * 
 * @return CMD_RESULT_SUCCESS if version sent, CMD_RESULT_FAILED otherwise
 */
cmd_result_t cmd_connect(void);

// Command Handlers end //

#endif /* COMMAND_H */
