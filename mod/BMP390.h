//hello

// Device Address
uint16_t BMP390Addy = 0x76 << 1;

// Status Enum
typedef enum {
    BARO_OK,
    BARO_FAIL
} BARO_STATUS;

// Function Prototypes
BARO_STATUS baro_red_read (uint16_t devAddress, uint16_t memAddress, uint8_t *pData, uint8_t size);