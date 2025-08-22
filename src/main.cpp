#include "WiFiServer.h"
#include "core/main_menu.h"
#include <globals.h>

#include "core/powerSave.h"
#include "core/serial_commands/cli.h"
#include "core/utils.h"
#include "esp32-hal-psram.h"
#include "esp_task_wdt.h"
#include <functional>
#include <string>
#include <vector>
io_expander ioExpander;
BruceConfig bruceConfig;
BruceConfigPins bruceConfigPins;

SerialCli serialCli;

StartupApp startupApp;
#if defined(HAS_TFT) || defined(HAS_SCREEN)
MainMenu mainMenu;
#endif
SPIClass sdcardSPI;
#ifdef USE_HSPI_PORT
SPIClass CC_NRF_SPI(VSPI);
#else
SPIClass CC_NRF_SPI(HSPI);
#endif

// Navigation Variables
volatile bool NextPress = false;
volatile bool PrevPress = false;
volatile bool UpPress = false;
volatile bool DownPress = false;
volatile bool SelPress = false;
volatile bool EscPress = false;
volatile bool AnyKeyPress = false;
volatile bool NextPagePress = false;
volatile bool PrevPagePress = false;
volatile bool LongPress = false;
volatile bool SerialCmdPress = false;
volatile int forceMenuOption = -1;
volatile uint8_t menuOptionType = 0;
String menuOptionLabel = "";
#ifdef HAS_ENCODER_LED
volatile int EncoderLedChange = 0;
#endif

#if defined(HAS_TOUCH)
TouchPoint touchPoint;
#endif

keyStroke KeyStroke;

TaskHandle_t xHandle;
void __attribute__((weak)) taskInputHandler(void *parameter) {
    auto timer = millis();
    while (true) {
#if defined(HAS_TFT) || defined(HAS_SCREEN)
        checkPowerSaveTime();
#endif
        // Sometimes this task run 2 or more times before looptask,
        // and navigation gets stuck, the idea here is run the input detection
        // if AnyKeyPress is false, or rerun if it was not renewed within 75ms (arbitrary)
        // because AnyKeyPress will be true if didn´t passed through a check(bool var)
        if (!AnyKeyPress || millis() - timer > 75) {
            NextPress = false;
            PrevPress = false;
            UpPress = false;
            DownPress = false;
            SelPress = false;
            EscPress = false;
            AnyKeyPress = false;
            SerialCmdPress = false;
            NextPagePress = false;
            PrevPagePress = false;
#if defined(HAS_TOUCH)
            touchPoint.pressed = false;
            touchPoint.Clear();
#endif
#ifndef USE_TFT_eSPI_TOUCH
            InputHandler();
#endif
            timer = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// Public Globals Variables
unsigned long previousMillis = millis();
int prog_handler; // 0 - Flash, 1 - LittleFS, 3 - Download
String cachedPassword = "";
bool interpreter_start = false;
bool sdcardMounted = false;
bool gpsConnected = false;

// wifi globals
// TODO put in a namespace
bool wifiConnected = false;
bool isWebUIActive = false;
String wifiIP;

bool BLEConnected = false;
bool returnToMenu;
bool isSleeping = false;
bool isScreenOff = false;
bool dimmer = false;
char timeStr[10];
time_t localTime;
struct tm *timeInfo;
#if defined(HAS_RTC)
cplus_RTC _rtc;
RTC_TimeTypeDef _time;
RTC_DateTypeDef _date;
bool clock_set = true;
#else
ESP32Time rtc;
bool clock_set = false;
#endif

std::vector<Option> options;
// Protected global variables
#if defined(HAS_SCREEN)
tft_logger tft = tft_logger(); // Invoke custom library
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite draw = TFT_eSprite(&tft);
volatile int tftWidth = TFT_HEIGHT;
#ifdef HAS_TOUCH
volatile int tftHeight =
    TFT_WIDTH - 20; // 20px to draw the TouchFooter(), were the btns are being read in touch devices.
#else
volatile int tftHeight = TFT_WIDTH;
#endif
#else
#if defined(HAS_TFT)
tft_logger tft;
SerialDisplayClass &sprite = tft;
SerialDisplayClass &draw = tft;
volatile int tftWidth = VECTOR_DISPLAY_DEFAULT_HEIGHT;
volatile int tftHeight = VECTOR_DISPLAY_DEFAULT_WIDTH;
#endif
#endif

#include "core/display.h"
#ifdef HAS_RGB
#include "core/led_control.h"
#endif
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/serialcmds.h"
#include "core/settings.h"
#include "core/wifi/wifi_common.h"
#include "modules/bjs_interpreter/interpreter.h" // for JavaScript interpreter
#include "modules/others/audio.h"                // for playAudioFile
#ifdef HAS_RF
#include "modules/rf/rf_utils.h" // for initCC1101once
#endif
#include <Wire.h>

/*********************************************************************
 **  Function: begin_storage
 **  Config LittleFS and SD storage
 *********************************************************************/
void begin_storage() {
    if (!LittleFS.begin(true)) { LittleFS.format(), LittleFS.begin(); }
    bool checkFS = setupSdCard();
    bruceConfig.fromFile(checkFS);
    bruceConfigPins.fromFile(checkFS);
}

/*********************************************************************
 **  Function: _setup_gpio()
 **  Sets up a weak (empty) function to be replaced by /ports/* /interface.h
 *********************************************************************/
void _setup_gpio() __attribute__((weak));
void _setup_gpio() {}

/*********************************************************************
 **  Function: _post_setup_gpio()
 **  Sets up a weak (empty) function to be replaced by /ports/* /interface.h
 *********************************************************************/
void _post_setup_gpio() __attribute__((weak));
void _post_setup_gpio() {}

/*********************************************************************
 **  Function: setup_gpio
 **  Setup GPIO pins
 *********************************************************************/
void setup_gpio() {

    // init setup from /ports/*/interface.h
    _setup_gpio();

    // Smoochiee v2 uses a AW9325 tro control GPS, MIC, Vibro and CC1101 RX/TX powerlines
    ioExpander.init(IO_EXPANDER_ADDRESS, &Wire);

#if TFT_MOSI > 0
    if (bruceConfigPins.CC1101_bus.mosi == (gpio_num_t)TFT_MOSI)
        initCC1101once(&tft.getSPIinstance()); // (T_EMBED), CORE2 and others
    else
#endif
#ifdef HAS_RF
        if (bruceConfigPins.CC1101_bus.mosi == bruceConfigPins.SDCARD_bus.mosi)
        initCC1101once(&sdcardSPI); // (ARDUINO_M5STACK_CARDPUTER) and (ESP32S3DEVKITC1) and devices that
                                    // share CC1101 pin with only SDCard
    else initCC1101once(NULL);
// (ARDUINO_M5STICK_C_PLUS) || (ARDUINO_M5STICK_C_PLUS2) and others that doesn´t share SPI with
// other devices (need to change it when Bruce board comes to shore)
#endif
}

/*********************************************************************
 **  Function: begin_tft
 **  Config tft
 *********************************************************************/
void begin_tft() {
#if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.setRotation(bruceConfig.rotation); // sometimes it misses the first command
    tft.invertDisplay(bruceConfig.colorInverted);
    tft.setRotation(bruceConfig.rotation);
    tftWidth = tft.width();
#ifdef HAS_TOUCH
    tftHeight = tft.height() - 20;
#else
    tftHeight = tft.height();
#endif
    resetTftDisplay();
    setBrightness(bruceConfig.bright, false);
#endif
}

/*********************************************************************
 **  Function: boot_screen
 **  Draw boot screen
 *********************************************************************/
void boot_screen() {
#if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.drawPixel(0, 0, bruceConfig.bgColor);
    tft.drawCentreString("Bruce", tftWidth / 2, 10, 1);
    tft.setTextSize(FP);
    tft.drawCentreString(BRUCE_VERSION, tftWidth / 2, 25, 1);
    tft.setTextSize(FM);
    tft.drawCentreString(
        "PREDATORY FIRMWARE", tftWidth / 2, tftHeight + 2, 1
    ); // will draw outside the screen on non touch devices
#endif
}

/*********************************************************************
 **  Function: boot_screen_anim
 **  Draw boot screen
 *********************************************************************/
void boot_screen_anim() {
#if defined(HAS_TFT) || defined(HAS_SCREEN)
    boot_screen();
    int i = millis();
    // checks for boot.jpg in SD and LittleFS for customization
    int boot_img = 0;
    bool drawn = false;
    if (sdcardMounted) {
        if (SD.exists("/boot.jpg")) boot_img = 1;
        else if (SD.exists("/boot.gif")) boot_img = 3;
    }
    if (boot_img == 0 && LittleFS.exists("/boot.jpg")) boot_img = 2;
    else if (boot_img == 0 && LittleFS.exists("/boot.gif")) boot_img = 4;
    if (bruceConfig.theme.boot_img) boot_img = 5; // override others

    tft.drawPixel(0, 0, 0);       // Forces back communication with TFT, to avoid ghosting
                                  // Start image loop
    while (millis() < i + 7000) { // boot image lasts for 5 secs
        if ((millis() - i > 2000) && !drawn) {
            tft.fillRect(0, 45, tftWidth, tftHeight - 45, bruceConfig.bgColor);
            if (boot_img > 0 && !drawn) {
                tft.fillScreen(bruceConfig.bgColor);
                if (boot_img == 5) {
                    drawImg(
                        *bruceConfig.themeFS(),
                        bruceConfig.getThemeItemImg(bruceConfig.theme.paths.boot_img),
                        0,
                        0,
                        true,
                        3600
                    );
                    Serial.println("Image from SD theme");
                } else if (boot_img == 1) {
                    drawImg(SD, "/boot.jpg", 0, 0, true);
                    Serial.println("Image from SD");
                } else if (boot_img == 2) {
                    drawImg(LittleFS, "/boot.jpg", 0, 0, true);
                    Serial.println("Image from LittleFS");
                } else if (boot_img == 3) {
                    drawImg(SD, "/boot.gif", 0, 0, true, 3600);
                    Serial.println("Image from SD");
                } else if (boot_img == 4) {
                    drawImg(LittleFS, "/boot.gif", 0, 0, true, 3600);
                    Serial.println("Image from LittleFS");
                }
                tft.drawPixel(0, 0, 0); // Forces back communication with TFT, to avoid ghosting
            }
            drawn = true;
        }
#if !defined(LITE_VERSION)
        if (!boot_img && (millis() - i > 2200) && (millis() - i) < 2700)
            tft.drawRect(2 * tftWidth / 3, tftHeight / 2, 2, 2, bruceConfig.priColor);
        if (!boot_img && (millis() - i > 2700) && (millis() - i) < 2900)
            tft.fillRect(0, 45, tftWidth, tftHeight - 45, bruceConfig.bgColor);
        if (!boot_img && (millis() - i > 2900) && (millis() - i) < 3400)
            tft.drawXBitmap(
                2 * tftWidth / 3 - 30,
                5 + tftHeight / 2,
                bruce_small_bits,
                bruce_small_width,
                bruce_small_height,
                bruceConfig.bgColor,
                bruceConfig.priColor
            );
        if (!boot_img && (millis() - i > 3400) && (millis() - i) < 3600) tft.fillScreen(bruceConfig.bgColor);
        if (!boot_img && (millis() - i > 3600))
            tft.drawXBitmap(
                (tftWidth - 238) / 2,
                (tftHeight - 133) / 2,
                bits,
                bits_width,
                bits_height,
                bruceConfig.bgColor,
                bruceConfig.priColor
            );
#endif
        if (check(AnyKeyPress)) // If any key or M5 key is pressed, it'll jump the boot screen
        {
            tft.fillScreen(bruceConfig.bgColor);
            delay(10);
            return;
        }
    }

    // Clear splashscreen
    tft.fillScreen(bruceConfig.bgColor);
#endif
}

/*********************************************************************
 **  Function: init_clock
 **  Clock initialisation for propper display in menu
 *********************************************************************/
void init_clock() {
#if defined(HAS_RTC)
    _rtc.begin();
    _rtc.GetBm8563Time();
    _rtc.GetTime(&_time);
#endif
}

/*********************************************************************
 **  Function: init_led
 **  Led initialisation
 *********************************************************************/
void init_led() {
#ifdef HAS_RGB
    beginLed();
#endif
}

/*********************************************************************
 **  Function: startup_sound
 **  Play sound or tone depending on device hardware
 *********************************************************************/
void startup_sound() {
    if (bruceConfig.soundEnabled == 0) return; // if sound is disabled, do not play sound
#if !defined(LITE_VERSION)
#if defined(BUZZ_PIN)
    // Bip M5 just because it can. Does not bip if splashscreen is bypassed
    _tone(5000, 50);
    delay(200);
    _tone(5000, 50);
    /*  2fix: menu infinite loop */
#elif defined(HAS_NS4168_SPKR)
    // play a boot sound
    if (bruceConfig.theme.boot_sound) {
        playAudioFile(bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.boot_sound));
    } else if (SD.exists("/boot.wav")) {
        playAudioFile(&SD, "/boot.wav");
    } else if (LittleFS.exists("/boot.wav")) {
        playAudioFile(&LittleFS, "/boot.wav");
    }
#endif
#endif
}

/*********************************************************************
 **  Function: setup
 **  Where the devices are started and variables set
 *********************************************************************/
void setup() {
    Serial.setRxBufferSize(
        SAFE_STACK_BUFFER_SIZE / 4
    ); // Must be invoked before Serial.begin(). Default is 256 chars
    Serial.begin(115200);
    log_d("Starting Bruce");
    WiFiServer dummyServer(80);

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    if (psramInit()) log_d("PSRAM Started");
    if (psramFound()) log_d("PSRAM Found");
    else log_d("PSRAM Not Found");
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());

    // declare variables
    prog_handler = 0;
    sdcardMounted = false;
    wifiConnected = false;
    BLEConnected = false;
    bruceConfig.bright = 100; // theres is no value yet
    bruceConfig.rotation = ROTATION;
    setup_gpio();
    log_d("gpio setup done");
#if defined(HAS_SCREEN)
    tft.init();
    tft.setRotation(bruceConfig.rotation);
    tft.fillScreen(TFT_BLACK);
    // bruceConfig is not read yet.. just to show something on screen due to long boot time
    tft.setTextColor(TFT_PURPLE, TFT_BLACK);
    tft.drawCentreString("Booting", tft.width() / 2, tft.height() / 2, 1);
#else
#if defined(HAS_TFT)
    tft.begin();
    log_d("tft setup done");
#endif
#endif
    begin_storage();
    log_d("storage setup done");
    begin_tft();
    log_d("tft setup done");
    init_clock();
    log_d("clock setup done");
    init_led();
    log_d("led setup done");

    // Some GPIO Settings (such as CYD's brightness control must be set after tft and sdcard)
    _post_setup_gpio();
    log_d("post gpio setup done");

    // #ifndef USE_TFT_eSPI_TOUCH
    // This task keeps running all the time, will never stop
    xTaskCreate(
        taskInputHandler, // Task function
        "InputHandler",   // Task Name
        4096,             // Stack size
        NULL,             // Task parameters
        2,                // Task priority (0 to 3), loopTask has priority 2.
        &xHandle          // Task handle (not used)
    );
    log_d("input handler setup done");
// #endif
#if defined(HAS_TFT) || defined(HAS_SCREEN)
    bruceConfig.openThemeFile(bruceConfig.themeFS(), bruceConfig.themePath);
#endif
    if (!bruceConfig.instantBoot) {
        boot_screen_anim();
        startup_sound();
    }
    log_d("boot screen done");

    if (bruceConfig.wifiAtStartup) {
        xTaskCreate(
            wifiConnectTask,   // Task function
            "wifiConnectTask", // Task Name
            4096,              // Stack size
            NULL,              // Task parameters
            2,                 // Task priority (0 to 3), loopTask has priority 2.
            NULL               // Task handle (not used)
        );
    }
    log_d("wifi setup done");

    //  start a task to handle serial commands while the webui is running
    startSerialCommandsHandlerTask();
    log_d("serial commands setup done");

#if defined(HAS_TFT) || defined(HAS_SCREEN)
    wakeUpScreen();
#endif
    log_d("wake up screen done");

    if (bruceConfig.startupApp != "" && !startupApp.startApp(bruceConfig.startupApp)) {
        bruceConfig.setStartupApp("");
        log_d("bruceConfig.setStartupApp");
    }
}

/**********************************************************************
 **  Function: loop
 **  Main loop
 **********************************************************************/
#if defined(HAS_SCREEN)
void loop() {
    // Interpreter must be ran in the loop() function, otherwise it breaks
    // called by 'stack canary watchpoint triggered (loopTask)'
#if !defined(LITE_VERSION)
    if (interpreter_start) {
        TaskHandle_t interpreterTaskHandler = NULL;
        xTaskCreate(
            interpreterHandler,     // Task function
            "interpreterHandler",   // Task Name
            16384,                  // Stack size
            NULL,                   // Task parameters
            2,                      // Task priority (0 to 3), loopTask has priority 2.
            &interpreterTaskHandler // Task handle
        );

        while (interpreter_start == true) { vTaskDelay(pdMS_TO_TICKS(500)); }
        interpreter_start = false;
        previousMillis = millis(); // ensure that will not dim screen when get back to menu
    }
#endif
    tft.fillScreen(bruceConfig.bgColor);

    mainMenu.begin();
    delay(1);
}
#else

// alternative loop function for headless boards
#include "core/wifi/webInterface.h"

enum WiFiState { TRY_STA, AP_MODE };
WiFiState wifiState = TRY_STA;
unsigned long staStartTime = 0;
const unsigned long STA_TIMEOUT = 15000; // 15 saniye

void loop() {
    switch (wifiState) {
        case TRY_STA:
            if (staStartTime == 0) {
                WiFi.mode(WIFI_STA);
                WiFi.begin("", "");
                staStartTime = millis();
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi, WebUI on STA");
                startWebUi(false);
                wifiState = AP_MODE; // STA bağlantısı ile WebUI başladı
            } else if (millis() - staStartTime > STA_TIMEOUT) {
                Serial.println("STA timeout, starting AP");
                WiFi.mode(WIFI_AP);
                WiFi.softAP("ESP32-AP", "password");
                startWebUi(true);
                wifiState = AP_MODE;
            }
            break;

        case AP_MODE:
            // AP modunda WebUI zaten çalışıyor, loop burada fazla işlem yapmaz
            break;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
}
#endif
