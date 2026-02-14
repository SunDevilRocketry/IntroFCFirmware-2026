#include "usb.h"
#include <string.h>

// Private stuff begin //
#define USB_RX_BUFFER_SIZE 256

static UART_HandleTypeDef* g_huart = NULL; // Pointer to UART handle from main.c

// Receive buffer and state
static uint8_t rx_buffer[USB_RX_BUFFER_SIZE];
static volatile uint16_t rx_index = 0;
static volatile uint8_t rx_complete = 0;
static volatile uint8_t rx_error = 0;

// Transmit state
static volatile uint8_t uart_tx_busy = 0;

static void usb_rx_restart(void);
// Private stuff end //

// HAL Callback Overrides begin //
/**
  * @brief HAL UART Tx Complete Callback
  * @param huart: UART handle
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == g_huart) {
        uart_tx_busy = 0;
    }
}

/**
  * @brief HAL UART Rx Complete Callback
  * @param huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == g_huart) {
        /* Check for newline char to determine end of message
            or if buffer is almost full */
        if (rx_buffer[rx_index - 1] == '\n')  
        {
            rx_buffer[rx_index -1 ] = '\0';     // Null terminate the string
            rx_complete = 1;
        } else if (rx_index >= USB_RX_BUFFER_SIZE - 1){
            rx_buffer[rx_index] = '\0';         // Null terminate the string
            rx_complete = 1;
        } else {
            // Receive sequential byte otherwise (rx_complete = 0)
            HAL_UART_Receive_IT(huart, &rx_buffer[rx_index++], 1);
        }
    }
}

/**
  * @brief HAL UART Error Callback
  * @param huart: UART handle
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == g_huart) {
        rx_error = 1;
        usb_rx_restart();   // Reset reception on error
    }
}

/**
  * @brief Restart UART reception in interrupt mode
  * @retval None
  */
static void usb_rx_restart(void)
{
    if (g_huart == NULL) return;
    
    // Clear the buffer BEFORE resetting index
    memset(rx_buffer, 0, USB_RX_BUFFER_SIZE);

    rx_index = 0;
    rx_complete = 0;
    rx_error = 0;
    HAL_UART_Receive_IT(g_huart, &rx_buffer[rx_index++], 1);
}
// HAL Callback Overrides end //


// USB Driver Interface begin //

/**
  * @brief Initialize the USB (UART) driver
  * @param huart: Pointer to UART handle to use for communication
  * @param interruptEnabled: TRUE to enable interrupt mode,
  *                          FALSE for blocking-only mode
  * @retval USB_STAT
  * @note The UART handle must be initialized by CubeMX before calling this function.
 */
USB_STAT usb_init(UART_HandleTypeDef* huart, bool interruptEnabled){
    // Validate the UART handle
    if (huart == NULL) {
        return USB_CRIT_FAILED;
    }

    g_huart = huart;
    
    // Initialize receive buffer (interupt mode - prep)
    rx_index = 0;
    rx_complete = 0;
    rx_error = 0;
    uart_tx_busy = 0;
    memset(rx_buffer, 0, USB_RX_BUFFER_SIZE);

     // Start first reception only if reception is enabled
    if(interruptEnabled){
        if (HAL_UART_Receive_IT(g_huart, &rx_buffer[rx_index++], 1) != HAL_OK) {
            return USB_CRIT_FAILED;
        }
    }
    
    return USB_OK;
}

// ==================== INTERRUPT-BASED FUNCTIONS ==================== //

/**
  * @brief Transmit data over USB (UART)
  * @param data: Pointer to data buffer
  * @param size: Number of bytes to transmit
  * @param timeout_ms: Timeout in milliseconds
  * @retval USB_STAT
  */
USB_STAT usb_IT_transmit(uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    uint32_t tickstart;
    
    // Check parameters (ensuring connection is able)
    if (g_huart == NULL || data == NULL || size == 0) {
        return USB_CRIT_FAILED;
    }

    tickstart = HAL_GetTick();
    
    // Wait for any ongoing transmission to complete
    while (uart_tx_busy) {
        if ((HAL_GetTick() - tickstart) > timeout_ms) {
            return USB_TIMEOUT;
        }
    }
    
    // Set busy flag (i.e. active transmission)
    uart_tx_busy = 1;
    
    /* Start transmission:
        Utilizing transmit_IT for non-blocking */
    if (HAL_UART_Transmit_IT(g_huart, data, size) != HAL_OK) {
        uart_tx_busy = 0;  // Clear busy flag on error
        return USB_CRIT_FAILED;
    }
    
    // Wait for transmission to complete
    tickstart = HAL_GetTick();      // Reset timer
    while (uart_tx_busy) {
        if ((HAL_GetTick() - tickstart) > timeout_ms) {
            return USB_TIMEOUT;
        }
    }
    
    return USB_OK; // Transmission succesful UwU
}

/**
  * @brief Receive data over USB (UART)
  * @param data: Pointer to receive buffer
  * @param size: Maximum bytes to receive
  * @param timeout_ms: Timeout in milliseconds
  * @param bytes_received: Pointer to store actual bytes received
  * @retval USB_Status_t
  */
USB_STAT usb_IT_receive(uint8_t* data, uint16_t size, uint32_t timeout_ms, uint16_t* bytes_received)
{
    uint32_t tickstart;
    
    // Check parameters (ensuring connection is able)
    if (g_huart == NULL || data == NULL || size == 0) {
        return USB_CRIT_FAILED;
    }

    // If data is completed/available, return immediately
    if (rx_complete) {
        uint16_t copy_size = (size < rx_index) ? size : rx_index;
        memcpy(data, rx_buffer, copy_size);
        *bytes_received = copy_size;
        rx_complete = 0;    /* Clear the received flag 
                            BUT keep buffer for potential next read */
        usb_rx_restart();   // Restart reception for next message
        
        return USB_OK;
    }

    // Wait for new data
    tickstart = HAL_GetTick();
    while (!rx_complete) {
        if ((HAL_GetTick() - tickstart) > timeout_ms) {
            *bytes_received = 0;
            return USB_TIMEOUT;
        }
        HAL_Delay(1); // Delay adjustable
    }
    
    // Copy received data to user buffer
    uint16_t copy_size = (size < rx_index) ? size : rx_index;
    memcpy(data, rx_buffer, copy_size);
    *bytes_received = copy_size;

    // Clear the received flag
    rx_complete = 0;

    // Restart reception for next message
    usb_rx_restart();
    
    return USB_OK;
}

// ============= BLOCKING FUNCTIONS (No interrupts) ============= //

/**
  * @brief Transmit data using BLOCKING mode
  * @param data: Pointer to data buffer to transmit
  * @param size: Number of bytes to transmit
  * @param timeout_ms: Maximum time to wait for transmission
  * @retval USB_STAT
  *   - USB_OK: Transmission successful
  *   - USB_CRIT_FAILED: Invalid parameters or HAL error
  *   - USB_TIMEOUT: Transmission timed out
  * @note Uses HAL_UART_Transmit directly at a cost of blocking behavior.
  */
USB_STAT usb_transmit(uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    // Verify parameters
    if (g_huart == NULL || data == NULL || size == 0) {
        return USB_CRIT_FAILED;
    }
    
    // Direct blocking transmit
    HAL_StatusTypeDef hal_status = HAL_UART_Transmit(g_huart, data, size, timeout_ms);
    
    switch (hal_status) {
        case HAL_OK:
            return USB_OK;
        case HAL_TIMEOUT:
            return USB_TIMEOUT;
        default:
            return USB_CRIT_FAILED;
    }
}

/**
  * @brief Receive data using BLOCKING mode
  * @param data: Pointer to buffer where received data will be stored
  * @param size: Number of bytes to receive (must match exactly)
  * @param timeout_ms: Maximum time to wait for reception
  * @retval USB_STAT
  *   - USB_OK: Reception successful
  *   - USB_CRIT_FAILED: Invalid parameters or HAL error
  *   - USB_TIMEOUT: Reception timed out
  * @note Uses HAL_UART_Receive directly at a cost of blocking behavior.
  *       Waits until EXACTLY 'size' bytes are received.
  */
USB_STAT usb_receive(uint8_t* data, uint16_t size, uint32_t timeout_ms)
{
    // Verify parameters
    if (g_huart == NULL || data == NULL || size == 0) {
        return USB_CRIT_FAILED;
    }
    
    // Direct blocking receive & waits for exact number of bytes
    HAL_StatusTypeDef hal_status = HAL_UART_Receive(g_huart, data, size, timeout_ms);

    switch (hal_status) {
        case HAL_OK:
            return USB_OK;
        case HAL_TIMEOUT:
            return USB_TIMEOUT;
        default:
            return USB_CRIT_FAILED;
    }
}

// USB Driver Interface end //

// Helper tools begin //

/**
  * @brief Check if data is available to read
  * @retval uint8_t: 1 if data available, 0 otherwise
  */
uint8_t usb_data_available(void)
{
    return (uint8_t)rx_complete;
}

/**
  * @brief Get pointer to received data without copying
  * @param data: Pointer pointer to set to internal buffer
  * @param size: Pointer to store size of available data
  * @retval USB_STAT
  */
USB_STAT usb_get_received_buffer(uint8_t** data, uint16_t* size)
{
    if (data == NULL || size == NULL) {
        return USB_CRIT_FAILED;
    }
    
    if (!rx_complete) {
        *data = NULL;
        *size = 0;
        return USB_CRIT_FAILED;
    }
    
    *data = rx_buffer;
    *size = rx_index;
    
    return USB_OK;
}

/**
  * @brief Clear the receive buffer
  * @retval None
  */
void usb_clear_buffer(void)
{
    memset(rx_buffer, 0, USB_RX_BUFFER_SIZE);
    rx_index = 0;
    rx_complete = 0;
    rx_error = 0;
    
    if (g_huart != NULL) {
        HAL_UART_Receive_IT(g_huart, &rx_buffer[rx_index++], 1);
    }
}

// Helper tools end //
