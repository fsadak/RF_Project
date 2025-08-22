// TODO: Be able to read bytes from server in background/task
//       so there is no loss of data when inputing
#include "modules/wifi/tcp_utils.h"
#include "core/wifi/wifi_common.h"

bool inputMode;

void listenTcpPort() {
    if (!wifiConnected) wifiConnectMenu();

    WiFiClient tcpClient;
    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    #endif


    String portNumber = keyboard("", 5, "TCP port to listen");
    if (portNumber.length() == 0) {
        #if defined(HAS_TFT) || defined(HAS_SCREEN)
        displayError("No port number given, exiting");
        #else
        Serial.println("No port number given, exiting");
        #endif
        return;
    }
    int portNumberInt = atoi(portNumber.c_str());
    if (portNumberInt == 0) {
        #if defined(HAS_TFT) || defined(HAS_SCREEN)
        displayError("Invalid port number, exiting");
        #else
        Serial.println("Invalid port number, exiting");
        #endif
        return;
    }

    WiFiServer server(portNumberInt);
    server.begin();

    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.println("Listening...");
    tft.print(WiFi.localIP().toString().c_str());
    tft.println(":" + portNumber);
    #else
    Serial.println("Listening...");
    Serial.print(WiFi.localIP().toString().c_str());
    Serial.println(":" + portNumber);
    #endif

    for (;;) {
        WiFiClient client = server.available(); // Wait for a client to connect

        if (client) {
            Serial.println("Client connected");
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            tft.println("Client connected");
            #endif

            while (client.connected()) {
                if (inputMode) {
                    String keyString = keyboard("", 16, "send input data, q=quit");
                    if (keyString == "q") {
                        #if defined(HAS_TFT) || defined(HAS_SCREEN)
                        displayError("Exiting Listener");
                        #else
                        Serial.println("Exiting Listener");
                        #endif
                        client.stop();
                        server.stop();
                        return;
                    }
                    inputMode = false;
                    #if defined(HAS_TFT) || defined(HAS_SCREEN)
                    tft.fillScreen(TFT_BLACK);
                    tft.setCursor(0, 0);
                    #endif
                    if (keyString.length() > 0) {
                        client.print(keyString); // Send the entire string to the client
                        Serial.print(keyString);
                    }
                } else {
                    if (client.available()) {
                        char incomingChar = client.read(); // Read one byte at time from the client
                        #if defined(HAS_TFT) || defined(HAS_SCREEN)
                        tft.print(incomingChar);
                        #endif
                        Serial.print(incomingChar);
                    }
                    if (check(SelPress)) { inputMode = true; }
                }
            }
            client.stop();
            Serial.println("Client disconnected");
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayError("Client disconnected");
            #endif
        }
        if (check(EscPress)) {
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayError("Exiting Listener");
            #else
            Serial.println("Exiting Listener");
            #endif
            server.stop();
            break;
        }
    }
}

void clientTCP() {
    if (!wifiConnected) wifiConnectMenu();

    String serverIP = keyboard("", 15, "Enter server IP");
    String portString = keyboard("", 5, "Enter server Port");
    int portNumber = atoi(portString.c_str());

    if (serverIP.length() == 0 || portNumber == 0) {
        #if defined(HAS_TFT) || defined(HAS_SCREEN)
        displayError("Invalid IP or Port");
        #else
        Serial.println("Invalid IP or Port");
        #endif
        return;
    }

    WiFiClient client;
    if (!client.connect(serverIP.c_str(), portNumber)) {
        #if defined(HAS_TFT) || defined(HAS_SCREEN)
        displayError("Connection failed");
        #else
        Serial.println("Connection failed");
        #endif
        return;
    }

    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Connected to:");
    tft.println(serverIP + ":" + portString);
    Serial.println("Connected to server");
    #else
    Serial.print("Connected to: ");
    Serial.println(serverIP + ":" + portString);
    Serial.println("Connected to server");
    #endif


    while (client.connected()) {
        if (inputMode) {
            String keyString = keyboard("", 16, "send input data");
            inputMode = false;
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            #endif
            if (keyString.length() > 0) {
                client.print(keyString);
                Serial.print(keyString);
            }
        } else {
            if (client.available()) {
                char incomingChar = client.read();
                #if defined(HAS_TFT) || defined(HAS_SCREEN)
                tft.print(incomingChar);
                #endif
                Serial.print(incomingChar);
            }
            if (check(SelPress)) { inputMode = true; }
        }
        if (check(EscPress)) {
            #if defined(HAS_TFT) || defined(HAS_SCREEN)
            displayError("Exiting Client");
            #else
            Serial.println("Exiting Client");
            #endif
            client.stop();
            break;
        }
    }

    #if defined(HAS_TFT) || defined(HAS_SCREEN)
    displayError("Connection closed.");
    #endif
    Serial.println("Connection closed.");
    client.stop();
}
