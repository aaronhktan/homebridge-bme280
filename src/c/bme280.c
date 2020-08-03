#include "bme280.h"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static int i2c_fd;

// Constants for compensation, unique to each chip
static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t dig_H1, dig_H3;
static int16_t dig_H2, dig_H4, dig_H5;
static int8_t dig_H6;

static int32_t t_fine;

static int read_bytes(int fd, uint8_t reg, uint8_t *rx_buf, int len) {
  memset(rx_buf, 0, len);

  uint8_t tx[1];
  tx[0] = reg;
  if (write(i2c_fd, tx, 1) != 1) {
    return ERROR_I2C;
  }

  return (!(read(i2c_fd, rx_buf, len) == len));
}

static int write_bytes(int fd, uint8_t reg, uint8_t *tx_buf, int len) {
  uint8_t tx[len + 1];
  tx[0] = reg;
  memcpy(&tx[1], tx_buf, len);

  return (!(write(i2c_fd, tx, len+1) == len+1));
}

// Calibration data is unique to each chip and must be read
// after startup so compensation for temperature and pressure
// can be applied.
static int read_calibration(void) {
  uint8_t t1[2], t2[2], t3[2];
  uint8_t p1[2], p2[2], p3[2], p4[2], p5[2], p6[2], p7[2], p8[2], p9[2];
  uint8_t h1[1], h2[2], h3[1], h4[2], h5[2], h6[1];

  int rv = 0;
  rv |= read_bytes(i2c_fd, BME280_DIG_T1_REG, t1, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_T2_REG, t2, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_T3_REG, t3, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P1_REG, p1, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P2_REG, p2, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P3_REG, p3, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P4_REG, p4, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P5_REG, p5, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P6_REG, p6, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P7_REG, p7, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P8_REG, p8, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_P9_REG, p9, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_H1_REG, h1, 1);
  rv |= read_bytes(i2c_fd, BME280_DIG_H2_REG, h2, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_H3_REG, h3, 1);
  rv |= read_bytes(i2c_fd, BME280_DIG_H4_REG, h4, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_H5_REG, h5, 2);
  rv |= read_bytes(i2c_fd, BME280_DIG_H6_REG, h6, 1);

  dig_T1 = (t1[1] << 8 | t1[0]) & 0xFFFF;
  dig_T2 = (t2[1] << 8 | t2[0]) & 0xFFFF;
  dig_T3 = (t3[1] << 8 | t3[0]) & 0xFFFF;
  dig_P1 = (p1[1] << 8 | p1[0]) & 0xFFFF;
  dig_P2 = (p2[1] << 8 | p2[0]) & 0xFFFF;
  dig_P3 = (p3[1] << 8 | p3[0]) & 0xFFFF;
  dig_P4 = (p4[1] << 8 | p4[0]) & 0xFFFF;
  dig_P5 = (p5[1] << 8 | p5[0]) & 0xFFFF;
  dig_P6 = (p6[1] << 8 | p6[0]) & 0xFFFF;
  dig_P7 = (p7[1] << 8 | p7[0]) & 0xFFFF;
  dig_P8 = (p8[1] << 8 | p8[0]) & 0xFFFF;
  dig_P9 = (p9[1] << 8 | p9[0]) & 0xFFFF;
  dig_H1 = h1[0] & 0xFF;
  dig_H2 = (h2[1] << 8 | h2[0]) & 0xFFFF;
  dig_H3 = h3[0] & 0xFF;
  dig_H4 = (h4[0] << 4 | (h4[1] & 0xF)) & 0xFFFF;
  dig_H5 = (h5[1] << 4 | h5[0] >> 4) & 0xFFFF;
  dig_H6 = h6[0] & 0xFF;

  debug_print(stdout, "0x%x, 0x%x, 0x%x\n", dig_T1, dig_T2, dig_T3);
  debug_print(stdout, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
      dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9);
  debug_print(stdout, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
      dig_H1, dig_H2, dig_H3, dig_H4, dig_H5, dig_H6);
  return rv;
}

// From the datasheet
static int compensate_pressure(int32_t p_in, double *p_out) {
  int64_t var1, var2, p;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)dig_P6;
  var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
  var2 = var2 + (((int64_t)dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
  var1 = ((((int64_t)1) << 47) + var1) * ((int64_t)dig_P1) >> 33;

  if (!var1) {
    debug_print(stdout, "%s\n", "Pressure compensation: var1 == 0");
    return 0;
  }
  
  p = 1048576 - p_in;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
  *p_out = (double)((uint32_t)p / 256.0);

  return NO_ERROR;
}

// Also from the datasheet
static int compensate_temperature(int32_t t_in, double *t_out) {
  int32_t var1, var2, temperature;

  var1 = ((((t_in >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  var2 = ((((t_in >> 4) - ((int32_t)dig_T1)) * ((t_in >> 4) - ((int32_t)dig_T1))) >> 12 *
      ((int32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  temperature = (t_fine * 5 + 128) >> 8;
  *t_out = temperature / 100.0;

  return NO_ERROR;
}

static int compensate_humidity(int32_t h_in, double *h_out) {
  int32_t var1;
  uint32_t humidity;

  var1 = (t_fine - ((int32_t)76800));
  var1 = (((((h_in << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) *
      var1)) + ((int32_t)16384)) >> 15) * (((((((var1 *
      ((int32_t)dig_H6)) >> 10) * (((var1 * ((int32_t)dig_H3)) >> 11) +
      ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig_H2) +
      8192) >> 14));
  var1 = (var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * ((uint32_t)dig_H1)) >> 4));
  var1 = (var1 < 0 ? 0 : var1);
  var1 = (var1 > 419430400 ? 419430400 : var1); 
  humidity = (uint32_t)(var1 >> 12);
  *h_out = humidity / 1024.0;

  return NO_ERROR;
}

int BME280_init(const char *i2c_adaptor) {
  i2c_fd = open(i2c_adaptor, O_RDWR);
  if (i2c_fd < 0) {
    return ERROR_DEVICE;
  }

  // Set settings for I2C
  int rv = 0;
  rv = ioctl(i2c_fd, I2C_SLAVE, BME280_ADDRESS);
  if (rv < 0) {
    return ERROR_I2C;
  }

  uint8_t id = 0;
  rv = BME280_get_chip_id(&id);
  if (id != BME280_CHIP_ID) {
    debug_print(stderr, "Chip ID 0x%x does not match 0x%x\n", id, BME280_CHIP_ID);
    return ERROR_DEVICE;
  } else if (rv) {
    return rv;
  }

  // Read compensation parameters
  rv |= read_calibration();
  if (rv) {
    return rv;
  }

  // Set default configuration
  rv |= BME280_set_config(MS250, FILTER_16);
  rv |= BME280_set_ctrl_hum(H_OVERSAMPLE_8);
  rv |= BME280_set_ctrl_meas(T_OVERSAMPLE_1, P_OVERSAMPLE_4, NORMAL);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not set default config");
    return ERROR_I2C;
  }

  return NO_ERROR;
}

int BME280_deinit(void) {
  close(i2c_fd);
  return NO_ERROR;
}

int BME280_measure(double *pressure_out,
                   double *temperature_out,
                   double *humidity_out) {
  int rv = 0;
  double pressure = 0, temperature = 0, humidity = 0;

  // Read temperature data
  uint8_t rx_temp[3];
  read_bytes(i2c_fd, BME280_TEMP_MSB, rx_temp, 3);
  int32_t t = (rx_temp[0] << 16 | rx_temp[1] << 8 | rx_temp[2]) >> 4;
  rv |= compensate_temperature(t, &temperature);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not compensate temperature");
    return rv;
  }

  // Read pressure data
  uint8_t rx_pres[3];
  read_bytes(i2c_fd, BME280_PRESS_MSB, rx_pres, 3);
  int32_t p = (rx_pres[0] << 16 | rx_pres[1] << 8 | rx_pres[2]) >> 4;
  rv |= compensate_pressure(p, &pressure);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not compensate pressure");
    return rv;
  }

  // Read humidity data
  uint8_t rx_hum[2];
  read_bytes(i2c_fd, BME280_HUM_MSB, rx_hum, 2);
  int32_t h = (rx_hum[0] << 8 | rx_hum[1]) & 0xFFFF;
  rv |= compensate_humidity(h, &humidity);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not compensate humidity");
    return rv;
  }

  *pressure_out = pressure;
  *temperature_out = temperature;
  *humidity_out = humidity;
  return NO_ERROR;
}

int BME280_get_config(uint8_t *standby_out,
                      uint8_t *filter_coefficient_out) {
  int rv = 0;
  uint8_t config_rx;

  rv = read_bytes(i2c_fd, BME280_CONFIG_REG, &config_rx, 1);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not read config");
    return rv;
  }

  *standby_out = config_rx & 0xE0;
  *filter_coefficient_out = config_rx & 0x1C;
  return NO_ERROR;
}

int BME280_get_ctrl_hum(uint8_t *osrs_h_out) {
  uint8_t ctrl_hum;
  int rv = read_bytes(i2c_fd, BME280_CTRL_HUM_REG, &ctrl_hum, 1);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not get ctrl meas");
    return rv;
  }

  *osrs_h_out = ctrl_hum;
  return NO_ERROR;
}

int BME280_get_ctrl_meas(uint8_t *osrs_p_out,
                         uint8_t *osrs_t_out,
                         uint8_t *mode_out) {
  int rv = 0;
  uint8_t ctrl_meas_rx;

  rv = read_bytes(i2c_fd, BME280_CTRL_MEAS_REG, &ctrl_meas_rx, 1);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not get ctrl meas");
    return rv;
  }

  *osrs_p_out = ctrl_meas_rx & 0x1C;
  *osrs_t_out = ctrl_meas_rx & 0xE0;
  *mode_out = ctrl_meas_rx & 0x03;
  return NO_ERROR;
}

int BME280_get_status(uint8_t *measuring_out,
                      uint8_t *im_update_out) {
  int rv = 0;
  uint8_t status_rx;

  rv = read_bytes(i2c_fd, BME280_STATUS_REG, &status_rx, 1);
  if (rv) {
    debug_print(stderr, "%s\n", "Could not get status");
    return rv;
  }

  *measuring_out = status_rx & 0x08;
  *im_update_out = status_rx & 0x01;
  return NO_ERROR;
}

int BME280_get_chip_id(uint8_t *id_out) {
  int rv = 0;
  uint8_t id_rx;

  rv = read_bytes(i2c_fd, BME280_ID_REG, &id_rx, 1);
  if (rv) {
    debug_print(stderr, "Return value from read_bytes is %d\n", rv);
    return rv;
  }

  *id_out = id_rx;
  return NO_ERROR;
}

int BME280_set_config(uint8_t standby,
                      uint8_t filter_coefficient) {
  uint8_t config_tx = (standby | filter_coefficient) & 0xFE;
  return write_bytes(i2c_fd, BME280_CONFIG_REG, &config_tx, 1);
}

int BME280_set_ctrl_hum(uint8_t osrs_h) {
  return write_bytes(i2c_fd, BME280_CTRL_HUM_REG, &osrs_h, 1);
}

int BME280_set_ctrl_meas(uint8_t osrs_p,
                         uint8_t osrs_t,
                         uint8_t mode) {
  uint8_t ctrl_meas_tx = (osrs_p | osrs_t | mode);
  return write_bytes(i2c_fd, BME280_CTRL_MEAS_REG, &ctrl_meas_tx, 1);
}

