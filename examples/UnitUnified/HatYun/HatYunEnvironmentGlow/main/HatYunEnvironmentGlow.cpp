/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  HatYunEnvironmentGlow — Hat Yun exclusive example
  Supported boards: M5StickC / M5StickCPlus / M5StickCPlus2

  LED layout: 14 LEDs, index 0 = bottom-left, counter-clockwise
    LED 0-6:  temperature (blue=cold -> green -> red=hot)
    LED 7-13: humidity    (dark=dry -> cyan=humid)
  Light sensor auto-dims LEDs in dark environments
  BtnA toggles between environment mode and rainbow mode
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedENV.h>
#include <cmath>

namespace {
auto& lcd = M5.Display;
M5Canvas canvas(&lcd);
m5::unit::UnitUnified Units;
m5::unit::HatYun unit;

auto& sht20  = unit.sht20;
auto& bmp280 = unit.bmp280;

// LED modes: 0=glow, 1=rainbow, 2=off
uint8_t led_mode{};
uint8_t rainbow_offset{};

// LED state cache to avoid flicker — only write changed LEDs
constexpr uint8_t NUM_LEDS = m5::unit::hatyun::NUM_LEDS;
uint8_t led_r[NUM_LEDS]{};
uint8_t led_g[NUM_LEDS]{};
uint8_t led_b[NUM_LEDS]{};

inline float clampf(const float v, const float lo, const float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Write LED array without flicker: set each LED individually, skip unchanged
void apply_leds(const uint8_t* r, const uint8_t* g, const uint8_t* b)
{
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        if (r[i] != led_r[i] || g[i] != led_g[i] || b[i] != led_b[i]) {
            unit.writeLED(i, r[i], g[i], b[i]);
            led_r[i] = r[i];
            led_g[i] = g[i];
            led_b[i] = b[i];
        }
    }
}

// HSV to RGB (hue 0-255, sat 255, val = brightness)
void hsv_to_rgb(const uint8_t hue, const uint8_t val, uint8_t& r, uint8_t& g, uint8_t& b)
{
    uint8_t region = hue / 43;
    uint8_t frac   = (hue - region * 43) * 6;
    uint8_t q      = (val * (255 - frac)) >> 8;
    uint8_t t      = (val * frac) >> 8;
    switch (region) {
        case 0:
            r = val;
            g = t;
            b = 0;
            break;
        case 1:
            r = q;
            g = val;
            b = 0;
            break;
        case 2:
            r = 0;
            g = val;
            b = t;
            break;
        case 3:
            r = 0;
            g = q;
            b = val;
            break;
        case 4:
            r = t;
            g = 0;
            b = val;
            break;
        default:
            r = val;
            g = 0;
            b = q;
            break;
    }
}

// Temperature to hue: 15C=170(blue), 25C=85(green), 35C=0(red)
uint8_t temp_to_hue(const float temp)
{
    return static_cast<uint8_t>(170.f - (clampf(temp, 15.f, 35.f) - 15.f) * (170.f / 20.f));
}

// Temperature to RGB565 for display
uint16_t temp_to_color565(const float temp)
{
    uint8_t r, g, b;
    hsv_to_rgb(temp_to_hue(temp), 255, r, g, b);
    return lcd.color565(r, g, b);
}

// Arc gauge constants: 135° (7:30) to 405° (4:30) = 270° sweep, bottom open
constexpr float ARC_START  = 135.f;
constexpr float ARC_RANGE  = 270.f;
constexpr uint16_t COL_BG  = 0x2104;  // dark gray track
constexpr uint16_t COL_DIM = 0x4208;  // dim label

void draw_arc_gauge(const int32_t cx, const int32_t cy, const int32_t ri, const int32_t ro, const float ratio,
                    const uint16_t fg, const uint16_t bg = COL_BG)
{
    canvas.fillArc(cx, cy, ri, ro, ARC_START, ARC_START + ARC_RANGE, bg);
    if (ratio > 0.001f) {
        canvas.fillArc(cx, cy, ri, ro, ARC_START, ARC_START + ARC_RANGE * clampf(ratio, 0.f, 1.f), fg);
    }
}

// Temperature arc with gradient segments
void draw_temp_arc(const int32_t cx, const int32_t cy, const int32_t ri, const int32_t ro, const float ratio)
{
    canvas.fillArc(cx, cy, ri, ro, ARC_START, ARC_START + ARC_RANGE, COL_BG);
    if (ratio <= 0.001f) {
        return;
    }
    float end_angle = ARC_START + ARC_RANGE * clampf(ratio, 0.f, 1.f);
    float seg       = ARC_START;
    while (seg < end_angle) {
        float seg_end = seg + 10.f;
        if (seg_end > end_angle) {
            seg_end = end_angle;
        }
        float seg_temp = 15.f + ((seg - ARC_START) / ARC_RANGE) * 20.f;
        uint16_t col   = temp_to_color565(seg_temp);
        canvas.fillArc(cx, cy, ri, ro, seg, seg_end, col);
        seg = seg_end;
    }
}

// Tick marks around arc
void draw_ticks(const int32_t cx, const int32_t cy, const int32_t ro, const uint8_t n, const int32_t len,
                const uint16_t col)
{
    for (uint8_t i = 0; i <= n; ++i) {
        float rad = (ARC_START + ARC_RANGE * i / n) * 3.14159265f / 180.f;
        float cs  = cosf(rad);
        float sn  = sinf(rad);
        canvas.drawLine(cx + (int32_t)((ro + 1) * cs), cy + (int32_t)((ro + 1) * sn),
                        cx + (int32_t)((ro + 1 + len) * cs), cy + (int32_t)((ro + 1 + len) * sn), col);
    }
}

// Needle indicator
void draw_needle(const int32_t cx, const int32_t cy, const int32_t len, const float ratio, const uint16_t col)
{
    float rad  = (ARC_START + ARC_RANGE * clampf(ratio, 0.f, 1.f)) * 3.14159265f / 180.f;
    int32_t x1 = cx + (int32_t)(len * cosf(rad));
    int32_t y1 = cy + (int32_t)(len * sinf(rad));
    canvas.drawLine(cx, cy, x1, y1, col);
    canvas.drawLine(cx + 1, cy, x1 + 1, y1, col);
    canvas.fillCircle(cx, cy, 3, col);
    canvas.fillCircle(cx, cy, 1, TFT_WHITE);
}

void draw_display(const float temp, const float humi, const float pres, const float bmp_temp, const uint16_t light)
{
    const int16_t H = canvas.height();

    canvas.fillScreen(TFT_BLACK);

    char buf[16];

    // === Left: Temperature hero gauge ===
    {
        constexpr int32_t cx = 68, cy = 68, ro = 52, ri = 42;

        float t_ratio = std::isfinite(temp) ? (clampf(temp, 15.f, 35.f) - 15.f) / 20.f : 0.f;

        draw_temp_arc(cx, cy, ri, ro, t_ratio);
        draw_ticks(cx, cy, ro, 4, 4, COL_DIM);
        draw_needle(cx, cy, ri - 4, t_ratio, TFT_WHITE);

        canvas.setTextDatum(middle_center);
        if (std::isfinite(temp)) {
            canvas.setFont(&fonts::Orbitron_Light_24);
            canvas.setTextColor(temp_to_color565(temp));
            snprintf(buf, sizeof(buf), "%.1f", temp);
            canvas.drawString(buf, cx, cy - 2);
            canvas.setFont(&fonts::FreeSans9pt7b);
            canvas.setTextColor(0x8410);
            canvas.drawString("C", cx + 2, cy + 16);
        }

        canvas.setFont(&fonts::FreeSans9pt7b);
        canvas.setTextColor(COL_DIM);
        canvas.setTextDatum(top_center);
        canvas.drawString("TEMP", cx, H - 16);
    }

    // Vertical separator
    canvas.drawFastVLine(136, 4, H - 8, COL_BG);

    // === Right top: Humidity mini gauge ===
    {
        constexpr int32_t cx = 188, cy = 30, ro = 24, ri = 18;

        float h_ratio = std::isfinite(humi) ? clampf(humi, 0.f, 100.f) / 100.f : 0.f;
        draw_arc_gauge(cx, cy, ri, ro, h_ratio, TFT_CYAN);

        canvas.setTextDatum(middle_center);
        if (std::isfinite(humi)) {
            canvas.setFont(&fonts::Orbitron_Light_24);
            canvas.setTextColor(TFT_WHITE);
            snprintf(buf, sizeof(buf), "%.0f", humi);
            canvas.drawString(buf, cx, cy - 2);
            canvas.setFont(&fonts::Font0);
            canvas.setTextColor(0x8410);
            canvas.drawString("%", cx, cy + 10);
        }
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(TFT_CYAN);
        canvas.setTextDatum(top_center);
        canvas.drawString("HUMI", cx, cy + ro + 2);
    }

    // === Right bottom: Pressure mini gauge ===
    {
        constexpr int32_t cx = 188, cy = 88, ro = 24, ri = 18;

        float p_ratio = std::isfinite(pres) ? (clampf(pres, 950.f, 1050.f) - 950.f) / 100.f : 0.f;
        draw_arc_gauge(cx, cy, ri, ro, p_ratio, TFT_ORANGE);

        canvas.setTextDatum(middle_center);
        if (std::isfinite(pres)) {
            canvas.setFont(&fonts::Orbitron_Light_24);
            canvas.setTextColor(TFT_WHITE);
            snprintf(buf, sizeof(buf), "%.0f", pres);
            canvas.drawString(buf, cx, cy - 2);
            canvas.setFont(&fonts::Font0);
            canvas.setTextColor(0x8410);
            canvas.drawString("hPa", cx, cy + 10);
        }
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(TFT_ORANGE);
        canvas.setTextDatum(top_center);
        canvas.drawString("PRES", cx, cy + ro + 2);
    }

    // === Bottom strip: BMP temp | mode | Light ===
    {
        constexpr const char* mode_names[] = {"Glow", "Rainbow", "Off"};
        canvas.setFont(&fonts::Font0);
        canvas.setTextColor(0x8410);

        canvas.setTextDatum(bottom_left);
        if (std::isfinite(bmp_temp)) {
            snprintf(buf, sizeof(buf), "%.1fC", bmp_temp);
            canvas.drawString(buf, 140, H - 1);
        }

        canvas.setTextDatum(bottom_center);
        canvas.setTextColor(led_mode == 0 ? TFT_GREEN : led_mode == 1 ? (uint16_t)TFT_MAGENTA : (uint16_t)0x8410);
        canvas.drawString(mode_names[led_mode], 188, H - 1);

        canvas.setTextDatum(bottom_right);
        canvas.setTextColor(TFT_YELLOW);
        snprintf(buf, sizeof(buf), "%u", light);
        canvas.drawString(buf, canvas.width() - 2, H - 1);
    }

    canvas.pushSprite(0, 0);
}

struct HatPins {
    int sda, scl;
};
HatPins get_hat_pins(const m5::board_t board)
{
    switch (board) {
        case m5::board_t::board_M5StickC:
        case m5::board_t::board_M5StickCPlus:
        case m5::board_t::board_M5StickCPlus2:
            return {0, 26};
        default:
            return {-1, -1};
    }
}

}  // namespace

void setup()
{
    auto m5cfg         = M5.config();
    m5cfg.pmic_button  = false;
    m5cfg.internal_imu = false;
    m5cfg.internal_rtc = false;
    M5.begin(m5cfg);

    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    canvas.setColorDepth(8);
    canvas.setPsram(false);
    canvas.createSprite(lcd.width(), lcd.height());

    auto board      = M5.getBoard();
    const auto pins = get_hat_pins(board);
    if (pins.sda < 0 || pins.scl < 0) {
        M5_LOGE("Unsupported board for HatYun");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    Wire.end();
    Wire.begin(pins.sda, pins.scl, 400 * 1000U);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    lcd.fillScreen(TFT_BLACK);
    M5_LOGI("%s", Units.debugInfo().c_str());
}

void loop()
{
    M5.update();
    Units.update();

    // BtnA: cycle glow -> rainbow -> off -> glow...
    if (M5.BtnA.wasClicked()) {
        led_mode = (led_mode + 1) % 3;
    }

    // Brightness from ambient light (0-1000 -> 4-64)
    uint16_t ambient   = unit.light();
    uint8_t brightness = static_cast<uint8_t>(4.f + clampf(ambient, 0.f, 1000.f) / 1000.f * 60.f);

    // Prepare new LED state
    uint8_t nr[NUM_LEDS]{}, ng[NUM_LEDS]{}, nb[NUM_LEDS]{};

    if (led_mode == 0) {
        // Glow: LED 0-6 temperature, LED 7-13 humidity
        float temp = sht20.temperature();
        float humi = sht20.humidity();

        if (std::isfinite(temp)) {
            uint8_t hue = temp_to_hue(temp);
            uint8_t r, g, b;
            hsv_to_rgb(hue, brightness, r, g, b);
            for (uint8_t i = 0; i < 7; ++i) {
                nr[i] = r;
                ng[i] = g;
                nb[i] = b;
            }
        }
        if (std::isfinite(humi)) {
            float h_ratio = clampf(humi, 0.f, 100.f) / 100.f;
            uint8_t hb    = static_cast<uint8_t>(brightness * h_ratio);
            if (hb < 1) {
                hb = 1;
            }
            for (uint8_t i = 7; i < NUM_LEDS; ++i) {
                nr[i] = 0;
                ng[i] = hb;
                nb[i] = hb;
            }
        }
    } else if (led_mode == 1) {
        // Rainbow animation
        for (uint8_t i = 0; i < NUM_LEDS; ++i) {
            uint8_t hue = rainbow_offset + (i * 256 / NUM_LEDS);
            hsv_to_rgb(hue, brightness, nr[i], ng[i], nb[i]);
        }
        ++rainbow_offset;
    }
    // led_mode == 2: all zeros (off)

    // Apply LEDs without flicker (only write changed)
    apply_leds(nr, ng, nb);

    // Teleplot output per sensor
    if (sht20.updated()) {
        M5.Log.printf(">SHT20_Temp:%2.2f\n>SHT20_Humidity:%2.2f\n", sht20.temperature(), sht20.humidity());
    }
    if (bmp280.updated()) {
        M5.Log.printf(">BMP280_Temp:%2.2f\n>BMP280_Pressure:%.2f\n", bmp280.temperature(), bmp280.pressure() * 0.01f);
    }
    if (unit.updated()) {
        M5.Log.printf(">Light:%u\n", unit.light());
    }

    // LCD update via sprite (flicker-free)
    if (sht20.updated() || bmp280.updated() || unit.updated()) {
        draw_display(sht20.temperature(), sht20.humidity(), bmp280.pressure() * 0.01f, bmp280.temperature(),
                     unit.light());
    }
}
