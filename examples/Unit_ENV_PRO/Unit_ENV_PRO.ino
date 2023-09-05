/**
 * @file Unit_ENV_PRO.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief UNIT ENV PRO TEST DEMO.
 * @version 0.1
 * @date 2023-09-05
 *
 *
 * @Hardwares:UNIT ENV PRO
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5Unified: https://github.com/m5stack/M5Unified
 * BME68x Sensor library: https://github.com/boschsensortec/Bosch-BME68x-Library
 * BSEC2 Software Library: https://github.com/boschsensortec/Bosch-BSEC2-Library
 */

#include <M5Unified.h>
#define STATE_SAVE_PERIOD UINT32_C(60 * 60 * 1000)

#define BSEC_MAX_STATE_BLOB_SIZE (221)

#include <EEPROM.h>
static uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE];

// Pls Download BSEC2 Software Library First
#include <bsec2.h>
#include "config/FieldAir_HandSanitizer/FieldAir_HandSanitizer.h"

Bsec2 envSensor;

void checkBsecStatus(Bsec2 bsec);

bool loadState(Bsec2 bsec) {
    if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
        /* Existing state in EEPROM */
        Serial.println("Reading state from EEPROM");
        Serial.print("State file: ");
        for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
            bsecState[i] = EEPROM.read(i + 1);
            Serial.print(String(bsecState[i], HEX) + ", ");
        }
        Serial.println();

        if (!bsec.setState(bsecState)) return false;
    } else {
        /* Erase the EEPROM with zeroes */
        Serial.println("Erasing EEPROM");

        for (uint8_t i = 0; i <= BSEC_MAX_STATE_BLOB_SIZE; i++)
            EEPROM.write(i, 0);

        EEPROM.commit();
    }

    return true;
}

bool saveState(Bsec2 bsec) {
    if (!bsec.getState(bsecState)) return false;

    Serial.println("Writing state to EEPROM");
    Serial.print("State file: ");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
        EEPROM.write(i + 1, bsecState[i]);
        Serial.print(String(bsecState[i], HEX) + ", ");
    }
    Serial.println();

    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
    EEPROM.commit();

    return true;
}

void updateBsecState(Bsec2 bsec) {
    static uint16_t stateUpdateCounter = 0;
    bool update                        = false;

    if (!stateUpdateCounter ||
        (stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
        /* Update every STATE_SAVE_PERIOD minutes */
        update = true;
        stateUpdateCounter++;
    }

    if (update && !saveState(bsec)) checkBsecStatus(bsec);
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
    updateBsecState(envSensor);
}

void setup() {
    M5.begin();

    EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);

    /* Desired subscription list of BSEC2 outputs */
    bsecSensor sensorList[] = {
        BSEC_OUTPUT_IAQ,          BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE, BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,      BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS};

    M5.Ex_I2C.begin();

    if (!envSensor.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
        checkBsecStatus(envSensor);
    }

    /* Load the configuration string that stores information on how to
    classify
     * the detected gas */
    if (!envSensor.setConfig(FieldAir_HandSanitizer_config)) {
        checkBsecStatus(envSensor);
    }

    /* Copy state from the EEPROM to the algorithm */
    if (!loadState(envSensor)) {
        checkBsecStatus(envSensor);
    }

    /* Subsribe to the desired BSEC2 outputs */
    if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList),
                                      BSEC_SAMPLE_RATE_LP)) {
        checkBsecStatus(envSensor);
    }

    /* Whenever new data is available call the newDataCallback function */
    envSensor.attachCallback(newDataCallback);

    Serial.println("BSEC library version " + String(envSensor.version.major) +
                   "." + String(envSensor.version.minor) + "." +
                   String(envSensor.version.major_bugfix) + "." +
                   String(envSensor.version.minor_bugfix));
}

void loop(void) {
    if (!envSensor.run()) {
        checkBsecStatus(envSensor);
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
