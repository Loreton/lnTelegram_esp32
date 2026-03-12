//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//

#ifdef  __ln_MAIN_TEST_MODULE__
const char* mainLogPrefix = "MAIN:";

#include "lnTelegram.h"
// #include <ssid_credentials_esp32.h>

#define __I_AM_MAIN_CPP__
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




// ===== Whitelist in Flash =====
const int64_t allowedIDs[] = { nLoreto_ChatID, nLoreto_ChatID };

// ===== Comandi validi (esempi) =====
const char cmd_status[] PROGMEM = "/status";
const char cmd_echo[]   PROGMEM = "/echo";

// Variabili di stato
#define BUTTON_PIN 19
bool            canUseNetwork = false;
uint32_t        lastRetryTime = 0;
const uint32_t  retryInterval = 30000; // 30 secondi tra i tentativi di scansione se disconnesso

// istanze
lnWiFiManagerNB wifiManager;
lnTimeClock     timeClock;
lnTelegram      tgBot;

// ================  CALLBACKs START ===============================
// --- WIFI-CALLBACK
void onConnectionChanged(bool connected) {
    canUseNetwork = connected;

    if (connected) {
        lnLOG_NOTIFY("%s Rete ripristinata. Avvio servizi...", mainLogPrefix);
    } else {
        lnLOG_ERROR("%s Connessione persa. Servizi in pausa.", mainLogPrefix);
    }
}


// --- Telegram-CALLBACK
void myTelegramProcessorCB(TBMessage &msg, const char* command, const char* payload) {
    lnLOG_WARNING("%s CallBACK - received message: %s", mainLogPrefix, msg.text.c_str());
    lnLOG_WARNING("%s   user:    %s", mainLogPrefix, msg.sender.username);
    lnLOG_WARNING("%s   chatID:  %lld", mainLogPrefix, msg.chatId);
    lnLOG_WARNING("%s   command: %s", mainLogPrefix, command);
    lnLOG_WARNING("%s   payload: %s", mainLogPrefix, payload);

    int64_t chat_id = msg.chatId;

    if (strcmp(command, "/start") == 0) {
        tgBot.sendMsg(chat_id, "Bot avviato correttamente");
    }

    else if (strcmp(command, "/status") == 0) {
        tgBot.sendMsg(chat_id, "Sistema OK - ESP32 online");
    }

    else if (strcmp(command, "/echo") == 0) {
        if (strlen(payload) == 0)
            tgBot.sendMsg(chat_id, "Uso: /echo testo");
        else
            tgBot.sendMsg(chat_id, payload);
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

        tgBot.sendMsg(chat_id, buffer);
    }
}


// --- lnTimeClock-CALLBACK
void onMinuteCB() {
    lnLOG_INFO("Nuovo minuto!");
}
// ================  CALLBACKs END =================================




void wifiInit() {
    // - prima dell'init()
    for (int8_t i = 0; i < loretoNetworksCount; i++) {
        wifiManager.addSSID(loretoNetworks[i].ssid, loretoNetworks[i].password);
    }

    wifiManager.setConnectionCallback(onConnectionChanged);
    wifiManager.init(8); // rssiGap di 8dB

    // 2. Lanciamo la prima scansione manuale
    wifiManager.startScan();
}



//#########################################################
//#    S E T U P
//#########################################################
void setup() {
    // setCpuFrequencyMhz(240); // Assicurati che l'ESP32 sia al massimo della potenza
    Serial.begin(115200);
    lnLog.init(128, 25);  // line_buffer_len, filename_buffer_len

    // Configura il PIN di test
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    wifiInit();

    // supponiamo WiFi già gestito altrove
    timeClock.begin();


    tgBot.begin(BOT_TOKEN);
    // tgBot.setAuthorizedIDs(allowedIDs, sizeof(allowedIDs) / sizeof(allowedIDs[0]));
    tgBot.setCommandCallback(myTelegramProcessorCB);
}



//#########################################################
//#    S E T U P
//#########################################################

void loop() {
    wifiManager.update();

    bool isNetReady = wifiManager.isConnected();
    timeClock.update(isNetReady);


    bool timeOK = timeClock.isTimeValid();
    tgBot.update(isNetReady, timeOK);



    // --- Tentare il rescan dopo un timeout ---
    if (!isNetReady) {
        uint32_t now = millis();
        // Attendi almeno 10 secondi dall'ultima disconnessione prima di scansionare
        if (now - lastRetryTime > retryInterval) {
            lnLOG_NOTIFY("%s WiFi giù, attendo stabilità prima di scansionare...", mainLogPrefix);
            wifiManager.startScan();
            lastRetryTime = now;
        }
    }



    // --- TEST DISCONNESSIONE MANUALE ---
    // Se premi il pulsante (o colleghi il PIN 19 a GND)
    if (digitalRead(BUTTON_PIN) == LOW && isNetReady) {
        if (wifiManager.isConnected()) {
            wifiManager.disconnect();
            delay(500); // Debounce brutale per il test
        }
    }

}




#endif