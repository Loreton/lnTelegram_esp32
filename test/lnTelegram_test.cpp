//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//
#include <Arduino.h>
#include <AsyncTelegram2.h>
// #include <SSLClient.h> // Spesso usata con AsyncTelegram
#include <WiFiClientSecure.h>

#include "lnLogger_Class.h"
#include "lnWiFiManager.h"

// =============================
// = WIFI and Telegram Credentials
// =============================
#define __I_AM_MAIN_CPP__
#include <orto_esp32_credentials.h>
const char* ssid      = casetta_ssid;
const char* password  = casetta_password;
const char* BOT_TOKEN = lnesp32orto_bot_token;
const char* channel   = lnesp32orto_bot_name;
const int64_t userid  = nLoreto_ChatID;


// ===== ID autorizzati (in flash) =====
const int64_t allowedIDs[] = {
    user1_ChatID
    user2_ChatID,
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




// --- Configurazione Telegram
// #define BOT_TOKEN "123456789:ABCDEF..." // Il tuo token
WiFiClientSecure client;
AsyncTelegram2 myBot(client);

// --- Variabili di stato
lnWiFiManagerNB wifiManager;
bool canUseNetwork = false;
uint32_t lastRetryTime = 0;
const uint32_t retryInterval = 30000;


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




static bool telegramStarted = false;
// ##################################################################
// CALLBACK: Gestione stato rete
// ##################################################################
void checkTelegram() {

    if (telegramStarted) {
        // 2. Gestisci Telegram SOLO se c'è rete
        TBMessage msg;
        if (myBot.getNewMessage(msg)) {
            lnLOG_INFO("TELEGRAM: Messaggio ricevuto: %s", msg.text);

            if (strcmp(msg.text, "/stato") == 0) {
                char buf[64];
                snprintf(buf, sizeof(buf), "Sono connesso a: %s", wifiManager.getConnectedSSID());
                myBot.sendMessage(msg, buf);
            }
        }

    }
    else {
        // Set the Telegram bot properies
        lnLOG_NOTIFY("WIFI: Rete OK. Configuro Telegram...");
        myBot.setUpdateTime(2000);
        myBot.setTelegramToken(token);

        // Telegram richiede che il client conosca l'ora (NTP) per i certificati SSL
        // configTime(3600, 3600, "pool.ntp.org");

        // Impostiamo il client secure (senza validazione certificato per semplicità, o usa fingerprint)
        client.setInsecure();

        // Avviamo il bot (invio messaggio di boot opzionale)
        if (myBot.begin()) {
            lnLOG_NOTIFY("TELEGRAM: Bot avviato con successo!");
            char welcome_msg[128];
            snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
            // Send a message to specific user who has started your bot
            myBot.sendTo(userid, welcome_msg);
            telegramStarted = true;
        }
    }
}



void wifiInit() {
    // - prima del wifiManager.init()
    // Configura WiFi
    for (int i = 0; i < loretoNetworksCount; i++) {
        wifiManager.addSSID(loretoNetworks[i].ssid, loretoNetworks[i].password);
    }
    wifiManager.setConnectionCallback(onConnectionChanged);
    wifiManager.init(8); // rssiGap

    wifiManager.startScan();
}




// ##################################################################
// CALLBACK: Gestione stato rete
// ##################################################################
void onConnectionChanged(bool connected) {
    canUseNetwork = connected;
    // usciamo subito
}



// ##################################################################
// SETUP
// ##################################################################
void setup() {
    Serial.begin(115200);
    delay(1000);
    lnLog.init(128, 20);

    wifiInit();

    // supponiamo WiFi già gestito altrove
    ln_clock.begin();

}






void onMinuteCB() {
    lnLOG_INFO("Nuovo minuto!");
}

// ##################################################################
// LOOP
// ##################################################################
void loop() {
    // 1. Mantieni vivo il WiFi (gestione risultati scan)
    wifiManager.update();
    ln_clock.update();

    if (canUseNetwork) {
        // checkTelegram();
        ln_clock.begin();
    } else {
        telegramStarted = false;
        // 3. Riconnessione manuale se il WiFi è caduto
        uint32_t now = millis();
        if (now - lastRetryTime > retryInterval) {
            lnLOG_WARNING("SISTEMA: WiFi giù. Cerco reti migliori...");
            wifiManager.startScan();
            lastRetryTime = now;
        }
    }

    // Altri task indipendenti dal WiFi...
}