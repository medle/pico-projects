
#include "m2_globals.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

// EEPROM chip 24C02 (2Kb) is on bus address 0x50 and has 8-byte pages
#define EEPROM_DEVICE_ADDR 0x50
#define EEPROM_PAGE_SIZE 8

// SL Pi Pico board config: I2C0, SDA=Pin20, SCL=Pin21
#define BUS_I2C_SDA_PIN 20
#define BUS_I2C_SCL_PIN 21

void eepromInit()
{
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(BUS_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BUS_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BUS_I2C_SDA_PIN);
    gpio_pull_up(BUS_I2C_SCL_PIN);
}

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
static bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void eepromScanBus()
{
    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    int found_addr = -1;
    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

        if (ret > 0) found_addr = addr; 
        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }

    if (found_addr >= 0) printf("Done: found bus address 0x%02x\n", found_addr);
    else printf("Done, no devices was found.\n");
 
}

// Packs memory address into byte array, returns the number of bytes packed
static size_t fill_memaddr_buf(
    uint8_t *memaddr_buf, uint32_t memaddr, uint8_t addrsize)
{
    size_t memaddr_len = 0;
    for (int16_t i = addrsize - 8; i >= 0; i -= 8) {
        memaddr_buf[memaddr_len++] = memaddr >> i;
    }
    return memaddr_len;
}

/// @brief Reads one byte from EEPROM
/// @param address Memory address of a byte
/// @param buf Buffer to receive the data
/// @return 1 on success, negative value on error
static int eepromReadByte(uint16_t address, uint8_t *buf)
{
    // master sends address without STOP
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(memaddr_buf, address, EEPROM_PAGE_SIZE);
    int ret = i2c_write_blocking(i2c_default, EEPROM_DEVICE_ADDR, 
        memaddr_buf, memaddr_len, true);
    if (ret != memaddr_len) {
        // must send STOP on error
        i2c_write_blocking(i2c_default, EEPROM_DEVICE_ADDR, NULL, 0, false);
        return ret;
    }

    // then reads data
    ret = i2c_read_blocking(i2c_default, EEPROM_DEVICE_ADDR, buf, 1, false);
    if (ret != 1) return ret;
    return 1;
}

/// @brief Writes one byte to EEPROM
/// @param address Memory address
/// @param data Byte value
/// @return 1 on success, negative value on error
static int eepromWriteByte(uint16_t address, uint8_t data)
{
    uint8_t buf[4 + 1];
    size_t memaddr_len = fill_memaddr_buf(buf, address, EEPROM_PAGE_SIZE);
    buf[memaddr_len] = data;
    int ret = i2c_write_blocking(
        i2c_default, EEPROM_DEVICE_ADDR, buf, memaddr_len + 1, false);
    if (ret != memaddr_len + 1) return ret;
    return 1;
}

int eepromReadBytes(uint16_t address, uint8_t *target, uint len)
{
    for(int i=0; i<len; i++) {
        int ret = eepromReadByte(address, &target[i]);
        if (ret != 1) return ret;
    }
    return len;
}

int eepromWriteBytes(uint16_t address, uint8_t *source, uint len)
{
    for(int i=0; i<len; i++) {
        int ret = eepromWriteByte(address, source[i]);
        if (ret != 1) return ret;
    }
    return len;
}

