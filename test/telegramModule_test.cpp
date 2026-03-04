//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//

#include <WiFi.h>
#include "TelegramModule.h"
#include "NetworkUtils.h"

#define __I_AM_MAIN_CPP__
// =============================
// CONFIG WIFI
// =============================
#include "ssid_credentials_esp32.h"
const char* ssid     = casetta_ssid;
const char* password = casetta_password;

// =============================
// CONFIG TELEGRAM
// =============================
#include "telegram_credentials_esp32.h"
const char* BOT_TOKEN = Loreto_Esp32_BotToken;

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
TelegramModule telegram;


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

void initWiFi() {
    // ----------------------------------
    // - connessione WiFi
    // ----------------------------------
    Serial.println("\nConnessione WiFi...");
    Serial.printf("ssid.......: %s\n", ssid);
    // Serial.printf("password...: %s\n", password);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());


    // ----------------------------------
    // - NTP altrimenti Telegram va in errore a causa
    // - dell'HTTPS che richiede il timing corretto
    // ----------------------------------
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Sincronizzazione NTP");

    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {   // attende tempo valido
       delay(500);
       Serial.print(".");
       now = time(nullptr);
    }

    Serial.println("\nOra sincronizzata!");

}




// =============================
// SETUP
// =============================
void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!initNetwork(ssid, password)) {
        Serial.println("Network FAILED");
        while (true);  // blocca se non connesso
    }

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    telegram.init(
        BOT_TOKEN,
        allowedIDs,
        sizeof(allowedIDs) / sizeof(allowedIDs[0]),
        allowedCommands,
        sizeof(allowedCommands) / sizeof(allowedCommands[0]),
        myCallback
    );

    Serial.println("TelegramModule inizializzato");
}

// =============================
// LOOP
// =============================
void loop() {
    maintainNetwork();

    static unsigned long lastCheck = 0;

    if (millis() - lastCheck > 1000) {   // polling ogni 1 secondo
        telegram.loop();
        lastCheck = millis();
    }
}