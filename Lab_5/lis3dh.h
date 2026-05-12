#ifndef LIS3DH_H
#define LIS3DH_H

#include <stdint.h>
#include "hardware/i2c.h"

// I2C address (SA0 pin low → 0x18, SA0 pin high → 0x19)
#define LIS3DH_ADDR     0x19

// Register map (relevant subset)
#define LIS3DH_CTRL_REG1    0x20
#define LIS3DH_OUT_X_L      0x28
#define LIS3DH_OUT_Y_L      0x2A
#define LIS3DH_OUT_Z_L      0x2C

// Auto-increment bit for multi-byte reads
#define LIS3DH_AUTO_INC     0x80

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} lis3dh_data_t;

void lis3dh_init(i2c_inst_t *i2c);
void lis3dh_read(i2c_inst_t *i2c, lis3dh_data_t *out);

#endif // LIS3DH_H
