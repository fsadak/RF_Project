#include "power_commands.h"
#include "core/settings.h"
#include <globals.h>

uint32_t poweroffCallback(cmd *c) {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    powerOff();
    esp_deep_sleep_start(); // only wake up via hardware reset
    return true;
    #else
    return false;
    #endif
}

uint32_t rebootCallback(cmd *c) {
    ESP.restart();
    return true;
}

uint32_t sleepCallback(cmd *c) {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    setSleepMode();
    return true;
    #else
    return false;
    #endif
}

void createPoweroffCommand(SimpleCLI *cli) { Command cmd = cli->addCommand("poweroff", poweroffCallback); }

void createRebootCommand(SimpleCLI *cli) { Command cmd = cli->addCommand("reboot", rebootCallback); }

void createSleepCommand(SimpleCLI *cli) { Command cmd = cli->addCommand("sleep", sleepCallback); }

void createPowerCommand(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCommand("power");

    Command cmdOff = cmd.addCommand("off", poweroffCallback);
    Command cmdReboot = cmd.addCommand("reboot", rebootCallback);
    Command cmdSleep = cmd.addCommand("sleep", sleepCallback);
}

void createPowerCommands(SimpleCLI *cli) {
    createPoweroffCommand(cli);
    createRebootCommand(cli);
    createSleepCommand(cli);

    createPowerCommand(cli);
}
