//hello

// Device Address
#define BMP390_DEV_ADDR (0x76 << 1)

// Status Enum
typedef enum {
    BARO_OK,
    BARO_FAIL
} BARO_STATUS;

// Memory addresses
#define REG_CHIP_ID 0x00;

// Default values for addresses
#define CHIP_ID_DEFAULT 0x60;

// Function Prototypes
BARO_STATUS baro_reg_write (uint16_t memAddress, uint8_t *pData, uint8_t size);
BARO_STATUS baro_reg_read (uint16_t memAddress, uint8_t *pData, uint8_t size);