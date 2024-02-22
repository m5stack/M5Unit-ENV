
/**
 * @file ENV_PRO.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief
 * @version 0.1
 * @date 2024-01-30
 *
 *
 * @Hardwares: M5Core + Unit ENV_PRO
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5UnitENV: https://github.com/m5stack/M5Unit-ENV
 * BME68x Sensor library: https://github.com/boschsensortec/Bosch-BME68x-Library
 * BSEC2 Software Library: https://github.com/boschsensortec/Bosch-BSEC2-Library
 */

#include <bsec2.h>

/**
 * @brief : This function checks the BSEC status, prints the respective error
 * code. Halts in case of error
 * @param[in] bsec  : Bsec2 class object
 */
void checkBsecStatus(Bsec2 bsec);

/**
 * @brief : This function is called by the BSEC library when a new output is
 * available
 * @param[in] input     : BME68X sensor data before processing
 * @param[in] outputs   : Processed BSEC BSEC output data
 * @param[in] bsec      : Instance of BSEC2 calling the callback
 */
void newDataCallback(const bme68xData data, const bsecOutputs outputs,
                     Bsec2 bsec);

/* Create an object of the class Bsec2 */
Bsec2 envSensor;

/* Entry point for the example */
void setup(void) {
    /* Desired subscription list of BSEC2 outputs */
    bsecSensor sensorList[] = {
        BSEC_OUTPUT_IAQ,          BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE, BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,      BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS};

    /* Initialize the communication interfaces */
    Serial.begin(115200);
    Wire.begin(21, 22);

    /* Valid for boards with USB-COM. Wait until the port is open */
    while (!Serial) delay(10);

    /* Initialize the library and interfaces */
    if (!envSensor.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
        checkBsecStatus(envSensor);
    }

    /* Subsribe to the desired BSEC2 outputs */
    if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList),
                                      BSEC_SAMPLE_RATE_CONT)) {
        checkBsecStatus(envSensor);
    }

    /* Whenever new data is available call the newDataCallback function */
    envSensor.attachCallback(newDataCallback);

    Serial.println("BSEC library version " + String(envSensor.version.major) +
                   "." + String(envSensor.version.minor) + "." +
                   String(envSensor.version.major_bugfix) + "." +
                   String(envSensor.version.minor_bugfix));
}

/* Function that is looped forever */
void loop(void) {
    /* Call the run function often so that the library can
     * check if it is time to read new data from the sensor
     * and process it.
     */
    if (!envSensor.run()) {
        checkBsecStatus(envSensor);
    }
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs,
                     Bsec2 bsec) {
    if (!outputs.nOutputs) {
        return;
    }

    Serial.println(
        "BSEC outputs:\n\ttimestamp = " +
        String((int)(outputs.output[0].time_stamp / INT64_C(1000000))));
    for (uint8_t i = 0; i < outputs.nOutputs; i++) {
        const bsecData output = outputs.output[i];
        switch (output.sensor_id) {
            case BSEC_OUTPUT_IAQ:
                Serial.println("\tiaq = " + String(output.signal));
                Serial.println("\tiaq accuracy = " +
                               String((int)output.accuracy));
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                Serial.println("\ttemperature = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                Serial.println("\tpressure = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                Serial.println("\thumidity = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_GAS:
                Serial.println("\tgas resistance = " + String(output.signal));
                break;
            case BSEC_OUTPUT_STABILIZATION_STATUS:
                Serial.println("\tstabilization status = " +
                               String(output.signal));
                break;
            case BSEC_OUTPUT_RUN_IN_STATUS:
                Serial.println("\trun in status = " + String(output.signal));
                break;
            default:
                break;
        }
    }
}

void checkBsecStatus(Bsec2 bsec) {
    if (bsec.status < BSEC_OK) {
        Serial.println("BSEC error code : " + String(bsec.status));

    } else if (bsec.status > BSEC_OK) {
        Serial.println("BSEC warning code : " + String(bsec.status));
    }

    if (bsec.sensor.status < BME68X_OK) {
        Serial.println("BME68X error code : " + String(bsec.sensor.status));
    } else if (bsec.sensor.status > BME68X_OK) {
        Serial.println("BME68X warning code : " + String(bsec.sensor.status));
    }
}
