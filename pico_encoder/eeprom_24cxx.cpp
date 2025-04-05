
#include "stdio.h"
#include "string.h"
#include "eeprom_24cxx.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "board_config.h"

void eeprom_init()
{
    i2c_init(EEPROM_I2C_PORT, EEPROM_I2C_BAUDRATE);
    gpio_set_function(EEPROM_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(EEPROM_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(EEPROM_I2C_SDA_PIN);
    gpio_pull_up(EEPROM_I2C_SCL_PIN);
}

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
static bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

// Packs memory address into byte array, returns the number of bytes packed
static size_t fill_memaddr_buf(
    uint8_t *memaddr_buf, uint32_t memaddr, uint8_t bytesperpage)
{
    size_t memaddr_len = 0;
    for (int16_t i = bytesperpage - 8; i >= 0; i -= 8) {
        memaddr_buf[memaddr_len++] = memaddr >> i;
    }
    return memaddr_len;
}

int eeprom_read_bytes(uint16_t address, uint8_t *target, uint len)
{
    // master sends address without STOP
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(memaddr_buf, address, EEPROM_BYTESPERPAGE);
    int ret = i2c_write_blocking(EEPROM_I2C_PORT, EEPROM_DEVICEADDR, 
        memaddr_buf, memaddr_len, true);
    if (ret != memaddr_len) {
        // must send STOP on error
        i2c_write_blocking(EEPROM_I2C_PORT, EEPROM_DEVICEADDR, NULL, 0, false);
        return ret;
    }

    // then reads data with STOP
    ret = i2c_read_blocking(EEPROM_I2C_PORT, EEPROM_DEVICEADDR, target, len, false);
    if (ret != len) return ret;
    return len;
}

int eeprom_write_bytes(uint16_t address, uint8_t *source, uint len)
{
    uint8_t buf[64];
    size_t memaddr_len = fill_memaddr_buf(buf, address, EEPROM_BYTESPERPAGE);
    
    size_t buf_len = memaddr_len + len;
    if (sizeof(buf) < buf_len) return -1;
    memmove(&buf[memaddr_len], source, len);
    
    int ret = i2c_write_blocking(
        EEPROM_I2C_PORT, EEPROM_DEVICEADDR, buf, buf_len, false);
    if (ret != buf_len) return ret;
    return len;
}

void eeprom_scan_bus()
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
            ret = i2c_read_blocking(EEPROM_I2C_PORT, addr, &rxdata, 1, false);

        if (ret > 0) found_addr = addr; 
        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }

    if (found_addr >= 0) printf("Done: found bus address 0x%02x\n", found_addr);
    else printf("Done, no devices was found.\n");
}
