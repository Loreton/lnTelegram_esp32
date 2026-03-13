//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//
#ifdef __ln_MAIN_TEST_MODULE__
#include <WiFi.h>
#include "lnTelegram.h"


// --- Project
#define  __I_AM_MAIN_CPP__
// #define LOG_MODULE_LEVEL LOG_MODULE_INFO
#include "lnLogger_Class.h"
#include "lnWiFiManager.h"
#include "lnTimeClock.h"



// =============================
// = WIFI and Telegram Credentials
// =============================
#include <orto_esp32_credentials.h>
const char* ssid      = casetta_ssid;
const char* password  = casetta_password;
const char* BOT_TOKEN = lnesp32orto_bot_token;
const char* channel   = lnesp32orto_bot_name;

// ===== ID autorizzati (in flash) =====
const int64_t allowedIDs[] = {
    nLoreto_ChatID,
    nLoreto_ChatID
};

// ===== Comandi validi (in flash) =====
const char cmd1[] PROGMEM = "/start";
const char cmd2[] PROGMEM = "/stop";
const char cmd3[] PROGMEM = "/status";
const char cmd4[] PROGMEM = "/whoami";
const char cmd5[] PROGMEM = "/echo";

const char* const allowedCommands[] PROGMEM = {
    cmd1,
    cmd2,
    cmd3,
    cmd4,
    cmd5
};

// =============================
// ISTANZA MODULO
// =============================
LnTelegram      telegram;
lnWiFiManagerNB   wifiManager;
lnTimeClock     lnTime;


void wifiScanEvent(bool scanning) {
    telegram.setWifiScanning(scanning);
}

// =============================
// CALLBACK COMANDI
// =============================
void myCallback(const TBMessage& msg,
                const char* command,
                const char* payload) {
    int64_t chat_id = msg.chatId;

    Serial.println("---- NUOVO COMANDO ----");
    Serial.print("Command: ");
    Serial.println(command);
    Serial.print("Payload: ");
    Serial.println(payload);
    Serial.println("-----------------------");

    if (strcmp(command, "/start") == 0) {
        telegram.sendMsg(chat_id, "Bot avviato correttamente");
    }

    else if (strcmp(command, "/status") == 0) {
        telegram.sendMsg(chat_id, "Sistema OK - ESP32 online");
    }

    else if (strcmp(command, "/echo") == 0) {
        if (strlen(payload) == 0)
            telegram.sendMsg(chat_id, "Uso: /echo testo");
        else
            telegram.sendMsg(chat_id, payload);
    }

    else if (strcmp(command, "/whoami") == 0) {

        char buffer[256];

        snprintf(buffer, sizeof(buffer),
            "command: %s\n"
            "username: %s\n"
            "chatId: %lld\n"
            "userId: %lld\n"
            "firstName: %s\n"
            "lastName: %s",
            command,
            msg.sender.username.length() ? msg.sender.username.c_str() : "-",
            msg.chatId,
            msg.sender.id,
            msg.sender.firstName.c_str(),
            msg.sender.lastName.length() ? msg.sender.lastName.c_str() : "-"
        );

        telegram.sendMsg(chat_id, buffer);
    }
}


void wifiInit() {
    // - prima del wifiManager.init()
    for (int8_t i = 0; i < loretoNetworksCount; i++) {
        wifiManager.addSSID(loretoNetworks[i].ssid, loretoNetworks[i].password);
    }

    wifiManager.setScanCallback(wifiScanEvent);

    wifiManager.init(
        5*60,   // scan ogni 5*60s (5 minuti)se connesso
        30,   // scan ogni 30s se non connesso
        10*60,  // timeout max 5 minuti (5*60)
        8        // rssi gap
    );
    WiFi.setSleep(false); // riduce glitch radio durante TLS.


    // Serial.print("Gateway: ");
    // Serial.println(WiFi.gatewayIP());

    // Serial.print("DNS: ");
    // Serial.println(WiFi.dnsIP());

    // Serial.print("RSSI: ");
    // Serial.println(WiFi.RSSI());
}




// =============================
// SETUP
// =============================
void setup() {
    Serial.begin(115200);
    delay(1000);
    lnLog.init(128, 20);  // line_buffer_len, filename_buffer_len

    wifiInit();

    // supponiamo WiFi già gestito altrove
    lnTime.begin();

    #ifdef __ln_INCLUDE_TELEGRAM__
        telegram.init(
            BOT_TOKEN,
            allowedIDs,
            sizeof(allowedIDs) / sizeof(allowedIDs[0]),
            allowedCommands,
            sizeof(allowedCommands) / sizeof(allowedCommands[0]),
            myCallback
        );

        Serial.println("LnTelegram inizializzato");
    #endif
}

// =============================
// LOOP
// =============================
void loop() {
    static unsigned long lastCheck = 0;
    wifiManager.update();
    lnTime.update();


    #ifdef __ln_INCLUDE_TELEGRAM__
        if (millis() - lastCheck > 3000) {   // polling ogni 1 secondo
            lnLOG_INFO("Free heap: %d", ESP.getFreeHeap());
            telegram.loop();
            lastCheck = millis();
        }
    #endif

    delay(100);
}

#endif