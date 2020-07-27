const BME280 = require('bindings')('homebridge-bme280');

function measure() {
  console.log(BME280.measure());
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function test() {
  BME280.init();
  console.log(BME280.getChipID());

  for (i = 0; i < 60; i++) {
    console.log(BME280.measure());
    await(sleep(1000));
  }

  BME280.deinit();
}

test();

