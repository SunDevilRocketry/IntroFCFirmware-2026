#ifndef USB_H
#define USB_H

#include "main.h"       // For UART_HandleTypeDef, HAL status, etc.
                        // (essentially, the STM32CubeMX configurations)
#include <stdint.h>     // Goofy uint8_t, uint16_t, etc.   
#include <stddef.h> 

typedef enum {
    USB_OK = 0,           // Operation successful
    USB_CRIT_FAILED,      // Critical failure (hardware error, invalid params)
    USB_TIMEOUT           // Operation timed out
} USB_STAT;

/**
  * @brief Initialize the USB (UART) driver
  * @retval USB_STAT: USB_OK if successful, otherwise error code
  */
USB_STAT usb_init(void);

/**
  * @brief Transmit data over USB (via UART)
  * @param data: Pointer to data buffer to transmit
  * @param size: Number of bytes to transmit
  * @param timeout_ms: Maximum time to wait for transmission (milliseconds)
  * @retval USB_STAT: USB_OK if successful, otherwise error code
  */
USB_STAT usb_transmit(uint8_t* data, uint16_t size, uint32_t timeout_ms);

/**
  * @brief Receive data over USB (via UART)
  * @param data: Pointer to buffer where received data will be stored
  * @param size: Maximum number of bytes to receive
  * @param timeout_ms: Maximum time to wait for reception in milliseconds
  * @param bytes_received: Pointer to store actual number of bytes received
  * @retval USB_STAT: USB_OK if successful, otherwise error code
  */
USB_STAT usb_receive(uint8_t* data, uint16_t size, uint32_t timeout_ms, uint16_t* bytes_received);

/**
  * @brief Check if data is available to read
  * @retval uint8_t: 1 if data available, 0 if no data
  */
uint8_t usb_data_available(void);

/**
  * @brief Get pointer to received data w/o copying
  * @param data: Pointer pointer to set to the internal receive buffer
  * @param size: Pointer to store the size of available data
  * @retval USB_STAT: USB_OK if data available, otherwise error code
  */
USB_STAT usb_get_received_buffer(uint8_t** data, uint16_t* size);

/**
  * @brief Clear the receive buffer
  * @retval None
  */
void usb_clear_buffer(void);

#endif
