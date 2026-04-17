#ifndef COMMAND_H
#define COMMAND_H

#include "usb.h"        // For USB communication functions
#include <stdint.h>
#include <stdbool.h>


/* Command byte definitions (Opcodes) */

#define CMD_PING        0x01    
#define CMD_CONNECT     0x27 
// [Add more if desired]   


// Protocol byte definitions begin //

#define ACK_READY       0x0A    // Ready to process - sent immediately after command 
#define ACK_SUCCESS     0x00    // Command succeeded (kinda useless [for future use] 
                                // given return of same command byte as completion.
#define ACK_ERROR       0x0F    // Command failed - sent for unknown commands

// Protocol byte definitions end //


/**
 * @brief Result codes returned by command handlers
 */
typedef enum {
    CMD_RESULT_SUCCESS, 
    CMD_RESULT_FAILED   
} cmd_result_t;


/**
 * @brief Function pointer type for all command handlers
 * @return cmd_result_t indicating success or failure
 */
typedef cmd_result_t (*cmd_handler_t)(void);

/* Public Functions */

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
 * Performs connection handshake. Instead of sending a heavy version 
 * string, it simply returns success. The command_process() function 
 * will react by echoing the unique firmware opcode (0x27) back to 
 * the client to verify firmware identity.
 * 
 * @return CMD_RESULT_SUCCESS always
 */
cmd_result_t cmd_connect(void);

// Command Handlers end //

#endif /* COMMAND_H */
