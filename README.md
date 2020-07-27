# Homebridge Plugin for BME280

This is a Homebridge plugin for BME280 temperature, humidity, and barometric pressure sensor, working on the Raspberry Pi 3.

It communicates with the sensor using Linux's user-mode I2C API, i2c-dev.

## Configuration
**Before running this plugin, you must add the `homebridge` user to the `i2c` group so Homebridge can access the i2c-dev interface. `sudo adduser homebridge i2c`**

| Field name           | Description                                                | Type / Unit    | Default value       | Required? |
| -------------------- |:-----------------------------------------------------------|:--------------:|:-------------------:|:---------:|
| name                 | Name of the accessory                                      | string         | —                   | Y         |
| i2cAdaptor           | i2cdev interface in `/dev/` that the sensor is mounted at  | string         | /dev/i2c-3          | N         |
| enableFakeGato       | Enable storing data in Eve Home app                        | bool           | false               | N         |
| fakeGatoStoragePath  | Path to store data for Eve Home app                        | string         | (fakeGato default)  | N         |
| enableMQTT           | Enable sending data to MQTT server                         | bool           | false               | N         |
| mqttConfig           | Object containing some config for MQTT                     | object         | —                   | N         |

The mqttConfig object is **only required if enableMQTT is true**, and is defined as follows:

| Field name           | Description                                      | Type / Unit  | Default value       | Required? |
| -------------------- |:-------------------------------------------------|:------------:|:-------------------:|:---------:|
| url                  | URL of the MQTT server, must start with mqtt://  | string       | —                   | Y         |
| temperatureTopic     | MQTT topic to which temperature data is sent     | string       | bme280/temeprature  | N         |
| pressureTopic        | MQTT topic to which pressure data is sent        | string       | bme280/pressure     | N         |
| humidityTopic        | MQTT topic to which humidity data is sent        | string       | bme280/humidity     | N         |

### Example Configuration

```
{
  "bridge": {
    "name": "Homebridge",
    "username": "XXXX",
    "port": XXXX
  },

  "accessories": [
    {
      "accessory": "BME280",
      "name": "BME280",
      "enableFakeGato": true,
      "enableMQTT": true,
      "mqtt": {
          "url": "mqtt://192.168.0.38",
          "pressureTopic": "bme280/pressure",
          "temperatureTopic": "bme280/temperature",
          "humidityTopic": "bme280/humidity"
      }
    }
  ]
}
```

## Project Layout

- All things required by Node are located at the root of the repository (i.e. package.json and index.js).
- The rest of the code is in `src`, further split up by language.
  - `c` contains the C code that runs on the device to communicate with the sensor. It also contains a simple program to check that the sensor is attached and readable.
  - `binding` contains the C++ code using node-addon-api to communicate between C and the Node.js runtime.
  - `js` contains a simple project that tests that the binding between C/Node.js is correctly working. It also contains a custom characteristic that allows Eve to keep barometric air pressure data.
