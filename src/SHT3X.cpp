#include "SHT3X.h"

/* Motor()

*/
SHT3X::SHT3X(uint8_t address, uint8_t deviceType) {
    if (deviceType == 0)
        Wire.begin();
    else if (deviceType == 1)
        Wire.begin(0, 26);
    else if (deviceType == 2)
        Wire.begin(2, 1);
    _address = address;
}

// /*! @Detecting whether a device is a Unit or a Hat
//     @return Return 0 for Unit, 1 for Hat */
// bool SHT3X::detectDevice() {
//     unsigned int data[6];
//     Wire.begin();
//     Wire.beginTransmission(_address);
//     Wire.write(0x2C);
//     Wire.write(0x06);
//     // Stop I2C transmission
//     Wire.endTransmission();
//     Wire.requestFrom(_address, (uint8_t)1);
//     for (int i = 0; i < 6; i++) {
//         data[i] = Wire.read();
//         Serial.printf("data%d:%d ", i, data[i]);
//     };
//     if (data[1] == -1) {
//         Wire.begin(0, 26);
//         return 1;
//     }
//     return 0;
// }

byte SHT3X::get() {
    unsigned int data[6];

    // Start I2C Transmission
    Wire.beginTransmission(_address);
    // Send measurement command
    Wire.write(0x2C);
    Wire.write(0x06);
    // Stop I2C transmission
    if (Wire.endTransmission() != 0) return 1;

    delay(200);

    // Request 6 bytes of data
    Wire.requestFrom(_address, (uint8_t)6);

    // Read 6 bytes of data
    // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
    for (int i = 0; i < 6; i++) {
        data[i] = Wire.read();
    };

    delay(50);

    if (Wire.available() != 0) return 2;

    // Convert the data
    cTemp    = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
    fTemp    = (cTemp * 1.8) + 32;
    humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

    return 0;
}
