#include "lis3dh.h"
#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
// Configure the LIS3DH for normal mode at 100 Hz, all axes enabled.
// CTRL_REG1: ODR=0101 (100Hz), LPen=0, Zen=Yen=Xen=1  → 0x57
// ---------------------------------------------------------------------------
void lis3dh_init(i2c_inst_t *i2c)
{
    uint8_t config[] = {LIS3DH_CTRL_REG1, 0x57};
    i2c_write_blocking(i2c, LIS3DH_ADDR, config, 2, false);
}

// ---------------------------------------------------------------------------
// Read X, Y, Z acceleration values (raw 16-bit signed, left-justified).
// Uses auto-increment to burst-read all 6 bytes in one transaction.
// ---------------------------------------------------------------------------
void lis3dh_read(i2c_inst_t *i2c, lis3dh_data_t *out)
{
    uint8_t reg = LIS3DH_OUT_X_L | LIS3DH_AUTO_INC;
    uint8_t data[6];

    i2c_write_blocking(i2c, LIS3DH_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, LIS3DH_ADDR, data, 6, false);

    out->x = (int16_t)(data[0] | (data[1] << 8));
    out->y = (int16_t)(data[2] | (data[3] << 8));
    out->z = (int16_t)(data[4] | (data[5] << 8));
}
