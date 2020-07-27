#ifndef BME280
#define BME280

#include <stdint.h>
#include <stdio.h>

// Print function that only prints if DEBUG is defined
#ifdef DEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif
#define debug_print(fd, fmt, ...) \
            do { if (DEBUG_PRINT) fprintf(fd, fmt, __VA_ARGS__); } while (0)

enum Error {
  NO_ERROR,
  ERROR_DEVICE,         // Couldn't find device
  ERROR_DRIVER,         // Driver failed to init
  ERROR_INVAL,          // Invalid argument
  ERROR_I2C             // I2C driver failed to read or write data
};

// Chip defines
// Standby time (not actively measuring) in milliseconds
enum Standby {
  MS0_5     = 0x00,
  MS62_5    = 0x20,
  MS125     = 0x40,
  MS250     = 0x60,
  MS500     = 0x80,
  MS1000    = 0xA0,
  MS10      = 0xC0,
  MS20      = 0xE0
};

enum Filter_Coefficient {
  FILTER_0  = 0x00, // 1 sample
  FILTER_2  = 0x04, // 2 samples
  FILTER_4  = 0x08, // 5 samples
  FILTER_8  = 0x0C, // 11 samples
  FILTER_16 = 0x10  // 22 samples
};

enum Humidity_Oversampling {
  H_OVERSAMPLE_SKIP = 0x00,
  H_OVERSAMPLE_1    = 0x01,
  H_OVERSAMPLE_2    = 0x02,
  H_OVERSAMPLE_4    = 0x03,
  H_OVERSAMPLE_8    = 0x04,
  H_OVERSAMPLE_16   = 0x05
};

enum Temperature_Oversampling {
  T_OVERSAMPLE_SKIP = 0x00,
  T_OVERSAMPLE_1    = 0x20,
  T_OVERSAMPLE_2    = 0x40,
  T_OVERSAMPLE_4    = 0x60,
  T_OVERSAMPLE_8    = 0x80,
  T_OVERSAMPLE_16    = 0xA0
};

enum Pressure_Oversampling {
  P_OVERSAMPLE_SKIP = 0x00,
  P_OVERSAMPLE_1    = 0x04,
  P_OVERSAMPLE_2    = 0x08,
  P_OVERSAMPLE_4    = 0x0C,
  P_OVERSAMPLE_8    = 0x10,
  P_OVERSAMPLE_16    = 0x14
};

enum Mode {
  SLEEP             = 0x00,
  FORCED            = 0x01,
  NORMAL            = 0x03
};

#define BME280_ADDRESS 0x76
#define BME280_MEASURING 0x08
#define BME280_IM_UPDATE 0x01
#define BME280_CHIP_ID 0x60

#define BME280_PRESS_MSB  0xF7
#define BME280_PRESS_LSB  0xF8
#define BME280_PRESS_XLSB 0xF9

#define BME280_TEMP_MSB   0xFA
#define BME280_TEMP_LSB   0xFB
#define BME280_TEMP_XLSB  0xFC

#define BME280_HUM_MSB    0xFD
#define BME280_HUM_LSB    0xFE

#define BME280_CONFIG_REG 0xF5
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_STATUS_REG 0xF3
#define BME280_CTRL_HUM_REG 0xF2
#define BME280_RESET_REG  0xE0
#define BME280_ID_REG     0xD0

#define BME280_DIG_T1_REG 0x88
#define BME280_DIG_T2_REG 0x8A
#define BME280_DIG_T3_REG 0x8C
#define BME280_DIG_P1_REG 0x8E
#define BME280_DIG_P2_REG 0x90
#define BME280_DIG_P3_REG 0x92
#define BME280_DIG_P4_REG 0x94
#define BME280_DIG_P5_REG 0x96
#define BME280_DIG_P6_REG 0x98
#define BME280_DIG_P7_REG 0x9A
#define BME280_DIG_P8_REG 0x9C
#define BME280_DIG_P9_REG 0x9E
#define BME280_DIG_H1_REG 0xA1
#define BME280_DIG_H2_REG 0xE1
#define BME280_DIG_H3_REG 0xE3
#define BME280_DIG_H4_REG 0xE4
#define BME280_DIG_H5_REG 0xE5
#define BME280_DIG_H6_REG 0xE7

// Set up and tear down BME280 interface
int BME280_init(const char *i2c_adaptor);
int BME280_deinit(void);

// Fetch data from BME280
int BME280_measure(double *pressure_out,
                   double *temperature_out,
                   double *humidity_out);
int BME280_get_config(uint8_t *standby_out,
                      uint8_t *filter_coefficient_out);
int BME280_get_ctrl_hum(uint8_t *osrs_h_out);
int BME280_get_ctrl_meas(uint8_t *osrs_p_out,
                         uint8_t *osrs_t_out,
                         uint8_t *mode_out);
int BME280_get_status(uint8_t *measuring_out,
                      uint8_t *im_update_out);
int BME280_get_chip_id(uint8_t *id_out);

// Set data in BME280
int BME280_set_config(uint8_t standby,
                      uint8_t filter_coefficient);
int BME280_set_ctrl_hum(uint8_t osrs_h);
int BME280_set_ctrl_meas(uint8_t osrs_p,
                         uint8_t osrs_t,
                         uint8_t mode);

#endif // BME280

