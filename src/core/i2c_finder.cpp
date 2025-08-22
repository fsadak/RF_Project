#include "i2c_finder.h"
#include "display.h"
#include "mykeyboard.h"
#include <Wire.h>

#define FIRST_I2C_ADDRESS 0x01
#define LAST_I2C_ADDRESS 0x7F

void find_i2c_addresses() {
#if defined(HAS_I2C)
#if defined(HAS_TFT) || defined(HAS_SCREEN)
    drawMainBorderWithTitle("I2C Finder");
    padprintln("");
    padprintln("");
#endif

    bool first_found = true;
    Wire.begin(GROVE_SDA, GROVE_SCL);

#if defined(HAS_TFT) || defined(HAS_SCREEN)
    padprintln("Checking I2C addresses ...\n\n");
    delay(300);

    padprint("Found: ");
#else
    Serial.println("Checking I2C addresses ...");
    delay(300);
    Serial.print("Found: ");
#endif

    for (uint8_t i = FIRST_I2C_ADDRESS; i <= LAST_I2C_ADDRESS; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
#if defined(HAS_TFT) || defined(HAS_SCREEN)
            if (!first_found) tft.print(", ");
            else first_found = false;
            tft.printf("0x%X", i);
#else
            if (!first_found) Serial.printf(",");
            else first_found = false;
            Serial.printf("0x%X", i);
#endif
        }
    }

    while (1) {
        if (check(EscPress) || check(SelPress)) {
            returnToMenu = true;
            break;
        }
    }
#endif
}

uint8_t find_first_i2c_address() {
#if defined(HAS_I2C)
    for (uint8_t i = FIRST_I2C_ADDRESS; i <= LAST_I2C_ADDRESS; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) return i;
    }
    return 0;
#endif
}

bool check_i2c_address(uint8_t i2c_address) {
#if defined(HAS_I2C)
    Wire.begin(GROVE_SDA, GROVE_SCL);
    Wire.beginTransmission(i2c_address);
    int error = Wire.endTransmission();
    return (error == 0);
#endif
}
