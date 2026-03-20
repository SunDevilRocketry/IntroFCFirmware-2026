//hello
//
// Device Address
#define BMP390_DEV_ADDR (0x76 << 1)

// Status Enum
typedef enum {
    BARO_OK,
    BARO_FAIL
} BARO_STATUS;

// TODO: Define baro struct 
typedef struct {
    uint32_t pressure; // In pascals
    uint32_t temperature;
} BARO_DATA;


// Memory addresses
#define REG_TEMP_23_16 0x09U
#define REG_TEMP_15_8 0x08U
#define REG_TEMP_7_0 0x07U
#define REG_PRESS_23_16 0x06U
#define REG_PRESS_15_8 0x05U
#define REG_PRESS_7_0 0x00U
#define REG_CHIP_ID 0x00U

// Default values for addresses
#define CHIP_ID_DEFAULT 0x60U

// Function Prototypes
BARO_STATUS baro_reg_write (uint16_t memAddress, uint8_t *pData, uint8_t size);
BARO_STATUS baro_reg_read (uint16_t memAddress, uint8_t *pData, uint8_t size);
BARO_STATUS baro_read_data (BARO_DATA* baro_data_out_ptr);