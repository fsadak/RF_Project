#include "serial_commands.h"
#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include "core/display.h"
#endif
#include "core/mykeyboard.h"

EspSerialCmd::EspSerialCmd() {}

void EspSerialCmd::sendCommands() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayBanner();
    padprintln("Waiting...");
    #endif

    if (!beginSend()) return;

    sendStatus = CONNECTING;
    Message message;

    delay(100);

    while (1) {
        if (check(EscPress)) {
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayInfo("Aborting...");
            #endif
            sendStatus = ABORTED;
            break;
        }

        if (check(SelPress)) { sendStatus = CONNECTING; }

        if (sendStatus == CONNECTING) {
            message = createCmdMessage();

            if (message.dataSize > 0) {
                esp_err_t response = esp_now_send(dstAddress, (uint8_t *)&message, sizeof(message));
                if (response == ESP_OK) sendStatus = SUCCESS;
                else {
                    Serial.printf("Send file response: %s\n", esp_err_to_name(response));
                    sendStatus = FAILED;
                }
            } else {
                Serial.println("No command to send");
                sendStatus = FAILED;
            }
        }

        if (sendStatus == FAILED) {
            displaySentError();
            sendStatus = WAITING;
        }

        if (sendStatus == SUCCESS) {
            displaySentCommand(message.data);
            sendStatus = WAITING;
        }

        delay(100);
    }

    delay(1000);
}

void EspSerialCmd::receiveCommands() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayBanner();
    padprintln("Waiting...");
    #endif

    recvCommand = "";
    recvQueue.clear();
    recvStatus = CONNECTING;
    Message recvMessage;

    if (!beginEspnow()) return;

    delay(100);

    while (1) {
        if (check(EscPress)) {
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayInfo("Aborting...");
            #endif
            recvStatus = ABORTED;
            break;
        }

        if (recvStatus == FAILED) {
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayRecvError();
            #endif
            recvStatus = WAITING;
        }
        if (recvStatus == SUCCESS) {
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayRecvCommand(serialCli.parse(recvCommand));
            #endif
            recvStatus = WAITING;
        }

        if (!recvQueue.empty()) {
            recvMessage = recvQueue.front();
            recvQueue.erase(recvQueue.begin());

            recvCommand = recvMessage.data;
            Serial.println(recvCommand);

            if (recvMessage.done) {
                Serial.println("Recv done");
                recvStatus = recvMessage.bytesSent == recvMessage.totalBytes ? SUCCESS : FAILED;
            }
        }

        delay(100);
    }

    delay(1000);
}

EspSerialCmd::Message EspSerialCmd::createCmdMessage() {
    // debounce
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.fillScreen(bruceConfig.bgColor);
    delay(500);
    #endif

    String command = keyboard("", ESP_DATA_SIZE, "Serial Command");
    Message msg = createMessage(command);
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    printMessage(msg);
    #endif

    return msg;
}

void EspSerialCmd::displayBanner() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    drawMainBorderWithTitle("RECEIVE COMMANDS");
    padprintln("");
    #endif
}

void EspSerialCmd::displayRecvCommand(bool success) {
    String execution = success ? "Execution success" : "Execution failed";
    Serial.println(execution);

    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayBanner();
    padprintln("Command received: ");
    padprintln(recvCommand);
    padprintln("");
    padprintln(execution);

    displayRecvFooter();
    #endif
}

void EspSerialCmd::displayRecvError() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayBanner();
    padprintln("Error receiving command");
    displayRecvFooter();
    #endif
}

void EspSerialCmd::displayRecvFooter() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    padprintln("\n");
    padprintln("Press [ESC] to leave");
    #endif
}

void EspSerialCmd::displaySentCommand(const char *command) {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayBanner();
    padprintln("Command sent: ");
    padprintln(command);
    displaySentFooter();
    #endif
}

void EspSerialCmd::displaySentError() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayBanner();
    padprintln("Error sending command");
    displaySentFooter();
    #endif
}

void EspSerialCmd::displaySentFooter() {
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    padprintln("\n");
    padprintln("Press [OK] to send another command");
    padprintln("");
    padprintln("Press [ESC] to leave");
    #endif
}
