#include "core/display.h"

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

void ReverseShell() {
    WebServer webServer(80); // HTTP server
    DNSServer dnsServer;
    IPAddress apGateway(192, 168, 4, 1);
    WiFiServer tcpServer(23); // Reverse shell server
    WiFiClient tcpClient;
    String lastCommand;
    bool shellConnected = false;

#if defined(HAS_TFT) || defined(HAS_SCREEN)
    options.clear();

    // Display initialization messages
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Reverse Shell", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(15, 33);
    tft.println("Developed by Fourier (github.com/9dl)");
    tft.println("Starting reverse shell server...");
#endif

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(apGateway, apGateway, IPAddress(255, 255, 255, 0))) {
#if (defined(HAS_TFT) || defined(HAS_SCREEN))
        tft.println("Failed to configure AP");
#endif
        return;
    }

#if defined(HAS_TFT) || defined(HAS_SCREEN)
    if (!WiFi.softAP("BruceShell", "", 1)) {
        tft.println("Failed to start AP");
        return;
    }

    tft.println("Wi-Fi AP Started: BruceShell");
#endif

    delay(3000);

    tcpServer.begin();
#if defined(HAS_TFT) || defined(HAS_SCREEN)
    tft.println("TCP server started on port 23.");
#endif

    webServer.on("/", [&webServer]() {
        String html = R"rawliteral(
            <!DOCTYPE html>
            <html>
            <head>
                <title>BruceShell Web Interface</title>
                <style>
                    body { display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background-color: #282c34; color: #61dafb; font-family: Arial, sans-serif; }
                    .container { text-align: center; }
                    input[type="text"] { width: 300px; padding: 10px; margin: 10px; border: none; border-radius: 5px; }
                    button { padding: 10px 20px; background-color: #61dafb; color: #282c34; border: none; border-radius: 5px; cursor: pointer; }
                    button:hover { background-color: #21a1f1; }
                </style>
            </head>
            <body>
                <div class="container">
                    <h1>BruceShell Executor</h1>
                    <form action="/execute" method="GET">
                        <input type="text" name="command" placeholder="Enter command" required>
                        <br>
                        <button type="submit">Run Command</button>
                    </form>
                    <br>
                    <a href="/status"><button>Server Status</button></a>
                </div>
            </body>
            </html>
        )rawliteral";
        webServer.send(200, "text/html", html);
    });

    webServer.on("/execute", [&webServer, &tcpClient, &lastCommand, &shellConnected]() {
        if (webServer.hasArg("command")) {
            lastCommand = webServer.arg("command");

            if (shellConnected && tcpClient) {
                tcpClient.println(lastCommand);
                webServer.send(200, "text/plain", "Command executed: " + lastCommand);
            } else {
                webServer.send(503, "text/plain", "Error: No active shell connection.");
            }
        } else {
            webServer.send(400, "text/plain", "Error: Command parameter is missing!");
        }
    });

    webServer.on("/status", [&webServer, &shellConnected]() {
        String status = shellConnected ? "Connected" : "Disconnected";
        webServer.send(200, "text/plain", "Server is online. Shell status: " + status);
    });

    dnsServer.start(53, "*", apGateway);
    webServer.begin();
#if (defined(HAS_TFT) || defined(HAS_SCREEN))
    tft.println("Web server started!");
#endif

    while (true) {
        dnsServer.processNextRequest();
        webServer.handleClient();

        if (!shellConnected) {
            tcpClient = tcpServer.available();
            if (tcpClient) {
#if (defined(HAS_TFT) || defined(HAS_SCREEN))
                tft.println("Client connected.");
#endif
                tcpClient.println("~Welcome to BruceShell.");
                tcpClient.println("~Developed by Fourier (github.com/9dl)");
                shellConnected = true;
            }
        }

        if (shellConnected && !tcpClient.connected()) {
#if (defined(HAS_TFT) || defined(HAS_SCREEN))
            tft.println("Client disconnected.");
#endif
            shellConnected = false;
            tcpClient.stop();
        }

#if defined(HAS_TFT) || defined(HAS_SCREEN)
        if (check(EscPress)) {
            tft.println("Exiting reverse shell server...");
            tcpServer.stop();
            webServer.stop();
            dnsServer.stop();
            break;
        }
#endif
    }
}
