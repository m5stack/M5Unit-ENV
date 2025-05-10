/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Common parts of SCD40/41 test
 */

namespace {
// float t uu int16 (temperature) same as library
constexpr uint16_t float_to_uint16(const float f)
{
    return f * 65536 / 175;
}

constexpr Mode mode_table[]         = {Mode::Normal, Mode::LowPower};
constexpr uint32_t interval_table[] = {
    5 * 1000,
    30 * 1000,
};

template <class U>
elapsed_time_t test_periodic(U* unit, const uint32_t times, const uint32_t measure_duration = 0)
{
    auto tm         = unit->interval();
    auto timeout_at = m5::utility::millis() + 10 * 1000;

    do {
        unit->update();
        if (unit->updated()) {
            break;
        }
        std::this_thread::yield();
    } while (!unit->updated() && m5::utility::millis() <= timeout_at);
    // timeout
    if (!unit->updated()) {
        return 0;
    }

    //
    uint32_t measured{};
    auto start_at = m5::utility::millis();
    timeout_at    = start_at + (times * (tm + measure_duration) * 2);

    do {
        unit->update();
        measured += unit->updated() ? 1 : 0;
        if (measured >= times) {
            break;
        }
        m5::utility::delay(1);

    } while (measured < times && m5::utility::millis() <= timeout_at);
    return (measured == times) ? m5::utility::millis() - start_at : 0;
}
}  // namespace

TEST_P(TestSCD4x, BasicCommand)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->inPeriodic());

    for (auto&& m : mode_table) {
        auto s = m5::utility::formatString("Mode:%u", m);
        SCOPED_TRACE(s);

        // Return False if already stopped
        EXPECT_FALSE(unit->stopPeriodicMeasurement());

        EXPECT_TRUE(unit->startPeriodicMeasurement(m));
        // Return False if already started
        EXPECT_FALSE(unit->startPeriodicMeasurement(m));
        EXPECT_FALSE(unit->startLowPowerPeriodicMeasurement());

        EXPECT_TRUE(unit->inPeriodic());

        // These APIs result in an error during periodic detection
        {
            EXPECT_FALSE(unit->writeTemperatureOffset(0));
            float offset{};
            EXPECT_FALSE(unit->readTemperatureOffset(offset));

            EXPECT_FALSE(unit->writeSensorAltitude(0));
            uint16_t altitude{};
            EXPECT_FALSE(unit->readSensorAltitude(altitude));

            int16_t correction{};
            EXPECT_FALSE(unit->performForcedRecalibration(0, correction));

            EXPECT_FALSE(unit->writeAutomaticSelfCalibrationEnabled(true));
            bool enabled{};
            EXPECT_FALSE(unit->readAutomaticSelfCalibrationEnabled(enabled));

            EXPECT_FALSE(unit->writeAutomaticSelfCalibrationTarget(0));
            uint16_t ppm{};
            EXPECT_FALSE(unit->readAutomaticSelfCalibrationTarget(ppm));

            EXPECT_FALSE(unit->writePersistSettings());

            uint64_t sno{};
            EXPECT_FALSE(unit->readSerialNumber(sno));

            bool malfunction{};
            EXPECT_FALSE(unit->performSelfTest(malfunction));

            EXPECT_FALSE(unit->performFactoryReset());

            EXPECT_FALSE(unit->reInit());
        }

        EXPECT_TRUE(unit->writeAmbientPressure(1013));
        uint16_t pressure{};
        EXPECT_TRUE(unit->readAmbientPressure(pressure));

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());
    }
}

TEST_P(TestSCD4x, OnChipOutputSignalCompensation)
{
    SCOPED_TRACE(ustr);

    {
        constexpr float OFFSET{5.4f};
        EXPECT_TRUE(unit->writeTemperatureOffset(OFFSET));
        float offset{};
        EXPECT_TRUE(unit->readTemperatureOffset(offset));
        EXPECT_EQ(float_to_uint16(offset), float_to_uint16(OFFSET)) << "offset:" << offset << " OFFSET:" << OFFSET;
    }

    {
        constexpr uint16_t ALTITUDE{3776};
        EXPECT_TRUE(unit->writeSensorAltitude(ALTITUDE));
        uint16_t altitude{};
        EXPECT_TRUE(unit->readSensorAltitude(altitude));
        EXPECT_EQ(altitude, ALTITUDE);
    }

    {
        constexpr uint16_t PRESSURE{1111};
        EXPECT_TRUE(unit->writeAmbientPressure(PRESSURE));
        uint16_t pressure{};
        EXPECT_TRUE(unit->readAmbientPressure(pressure));
        EXPECT_EQ(pressure, PRESSURE);

        EXPECT_TRUE(unit->writeAmbientPressure(700));
        EXPECT_TRUE(unit->readAmbientPressure(pressure));
        EXPECT_EQ(pressure, 700);

        EXPECT_TRUE(unit->writeAmbientPressure(1200));
        EXPECT_TRUE(unit->readAmbientPressure(pressure));
        EXPECT_EQ(pressure, 1200);

        EXPECT_FALSE(unit->writeAmbientPressure(699));
        EXPECT_FALSE(unit->writeAmbientPressure(1201));
    }
}

TEST_P(TestSCD4x, FieldCalibration)
{
    SCOPED_TRACE(ustr);

    {
        int16_t correction{};
        EXPECT_TRUE(unit->performForcedRecalibration(1234, correction));
    }

    {
        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(false));
        bool enabled{};
        EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
        EXPECT_FALSE(enabled);

        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(true));
        EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
        EXPECT_TRUE(enabled);
    }

    {
        constexpr uint16_t PPM{12345};
        EXPECT_TRUE(unit->writeAutomaticSelfCalibrationTarget(PPM));
        uint16_t ppm{};
        EXPECT_TRUE(unit->readAutomaticSelfCalibrationTarget(ppm));
        EXPECT_EQ(ppm, PPM);
    }
}

TEST_P(TestSCD4x, AdvancedFeatures)
{
    SCOPED_TRACE(ustr);

    {
        // Read direct [MSB] SNB_3, SNB_2, CRC, SNB_1, SNB_0, CRC [LSB]
        std::array<uint8_t, 9> rbuf{};
        EXPECT_TRUE(unit->readRegister(m5::unit::scd4x::command::GET_SERIAL_NUMBER, rbuf.data(), rbuf.size(), 1));

        // M5_LOGI("%02x%02x%02x%02x%02x%02x", rbuf[0], rbuf[1], rbuf[3],
        // rbuf[4],
        //         rbuf[6], rbuf[7]);

        m5::types::big_uint16_t w0(rbuf[0], rbuf[1]);
        m5::types::big_uint16_t w1(rbuf[3], rbuf[4]);
        m5::types::big_uint16_t w2(rbuf[6], rbuf[7]);
        uint64_t d_sno = (((uint64_t)w0.get()) << 32) | (((uint64_t)w1.get()) << 16) | ((uint64_t)w2.get());

        // M5_LOGI("d_sno[%llX]", d_sno);

        //
        uint64_t sno{};
        char ssno[13]{};
        EXPECT_TRUE(unit->readSerialNumber(sno));
        EXPECT_TRUE(unit->readSerialNumber(ssno));

        // M5_LOGI("s:[%s] uint64:[%x]", ssno, sno);

        EXPECT_EQ(sno, d_sno);

        std::stringstream stream;
        stream << std::uppercase << std::setw(12) << std::hex << std::setfill('0') << sno;
        std::string s(stream.str());
        EXPECT_STREQ(s.c_str(), ssno);
    }

    // Set
    constexpr float OFFSET{1.234f};
    EXPECT_TRUE(unit->writeTemperatureOffset(OFFSET));
    constexpr uint16_t ALTITUDE{3776};
    EXPECT_TRUE(unit->writeSensorAltitude(ALTITUDE));
    EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(false));
    constexpr uint16_t PPM{12345};
    EXPECT_TRUE(unit->writeAutomaticSelfCalibrationTarget(PPM));

    EXPECT_TRUE(unit->writePersistSettings());  // Save EEPROM

    // Overwrite settings
    EXPECT_TRUE(unit->writeTemperatureOffset(OFFSET * 2));
    EXPECT_TRUE(unit->writeSensorAltitude(ALTITUDE * 2));
    EXPECT_TRUE(unit->writeAutomaticSelfCalibrationEnabled(true));
    EXPECT_TRUE(unit->writeAutomaticSelfCalibrationTarget(PPM * 2));

    float off{};
    uint16_t alt{}, ppm{};
    bool enabled{};

    EXPECT_TRUE(unit->readTemperatureOffset(off));
    EXPECT_TRUE(unit->readSensorAltitude(alt));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationTarget(ppm));

    EXPECT_EQ(float_to_uint16(off), float_to_uint16(OFFSET * 2));
    EXPECT_EQ(alt, ALTITUDE * 2);
    EXPECT_EQ(ppm, PPM * 2);
    EXPECT_TRUE(enabled);

    EXPECT_TRUE(unit->reInit());  // Load EEPROM

    // Check saved settings
    EXPECT_TRUE(unit->readTemperatureOffset(off));
    EXPECT_TRUE(unit->readSensorAltitude(alt));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationTarget(ppm));

    EXPECT_EQ(float_to_uint16(off), float_to_uint16(OFFSET));
    EXPECT_EQ(alt, ALTITUDE);
    EXPECT_EQ(ppm, PPM);
    EXPECT_FALSE(enabled);

    bool malfunction{};
    EXPECT_TRUE(unit->performSelfTest(malfunction));

    EXPECT_TRUE(unit->performFactoryReset());  // Reset EEPROM

    EXPECT_TRUE(unit->readTemperatureOffset(off));
    EXPECT_TRUE(unit->readSensorAltitude(alt));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationEnabled(enabled));
    EXPECT_TRUE(unit->readAutomaticSelfCalibrationTarget(ppm));

    EXPECT_NE(float_to_uint16(off), float_to_uint16(OFFSET));
    EXPECT_NE(alt, ALTITUDE);
    EXPECT_NE(ppm, PPM);
    EXPECT_TRUE(enabled);
}

TEST_P(TestSCD4x, Periodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->performFactoryReset());  // Reset EEPROM

    uint32_t idx{};
    for (auto&& m : mode_table) {
        auto s = m5::utility::formatString("Mode:%u", m);
        SCOPED_TRACE(s);

        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_TRUE(unit->startPeriodicMeasurement(m));
        EXPECT_TRUE(unit->inPeriodic());
        EXPECT_EQ(unit->updatedMillis(), 0);

        auto it      = interval_table[idx];
        auto elapsed = test_periodic(unit.get(), STORED_SIZE, it);

        EXPECT_TRUE(unit->stopPeriodicMeasurement());
        EXPECT_FALSE(unit->inPeriodic());

        EXPECT_NE(elapsed, 0);
        EXPECT_GE(elapsed, STORED_SIZE * it);

        //
        EXPECT_EQ(unit->available(), STORED_SIZE);
        EXPECT_FALSE(unit->empty());
        EXPECT_TRUE(unit->full());

        uint32_t cnt{STORED_SIZE / 2};
        while (cnt-- && unit->available()) {
            EXPECT_NE(unit->co2(), 0);
            EXPECT_TRUE(std::isfinite(unit->celsius()));
            EXPECT_TRUE(std::isfinite(unit->fahrenheit()));
            EXPECT_TRUE(std::isfinite(unit->humidity()));

            EXPECT_EQ(unit->co2(), unit->oldest().co2());
            EXPECT_FLOAT_EQ(unit->celsius(), unit->oldest().celsius());
            EXPECT_FLOAT_EQ(unit->fahrenheit(), unit->oldest().fahrenheit());
            EXPECT_FLOAT_EQ(unit->humidity(), unit->oldest().humidity());

            EXPECT_FALSE(unit->empty());
            unit->discard();
        }
        EXPECT_EQ(unit->available(), STORED_SIZE / 2);
        EXPECT_FALSE(unit->empty());
        EXPECT_FALSE(unit->full());

        unit->flush();
        EXPECT_EQ(unit->available(), 0);
        EXPECT_TRUE(unit->empty());
        EXPECT_FALSE(unit->full());

        EXPECT_EQ(unit->co2(), 0);
        EXPECT_FALSE(std::isfinite(unit->celsius()));
        EXPECT_FALSE(std::isfinite(unit->fahrenheit()));
        EXPECT_FALSE(std::isfinite(unit->humidity()));
    }
}
