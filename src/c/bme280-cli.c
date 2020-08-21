#include "bme280.h"

#include <unistd.h>

int main(int argc, char **argv) {
  int rv = BME280_init("/dev/i2c-4");
  if (rv) {
    printf("Failed to init BME280\n");
  }

  uint8_t standby, coefficient;
  BME280_get_config(&standby, &coefficient);
  printf("Standby: 0x%x, Coefficient: 0x%x\n", standby, coefficient);

  uint8_t osrs_p, osrs_t, mode;
  BME280_get_ctrl_meas(&osrs_p, &osrs_t, &mode);
  printf("osrs_p: 0x%x, osrs_t: 0x%x, mode: 0x%x\n", osrs_p, osrs_t, mode);

  uint8_t osrs_h;
  BME280_get_ctrl_hum(&osrs_h);
  printf("osrs_h: 0x%x\n", osrs_h);

  for (int counter = 0; counter < 60; counter++) {
    double pressure, temperature, humidity;
    int rv = BME280_measure(&pressure, &temperature, &humidity);
    printf("Temperature: %f, Pressure: %f, Humidity: %f, rv: %d\n", temperature, pressure, humidity, rv);
    usleep(1000000);
  }

  BME280_deinit();
}
