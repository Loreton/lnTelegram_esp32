//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//
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


const char* mainLogPrefix = "MAIN:";

// ===== Whitelist in Flash =====
const int64_t allowedIDs[] = { nLoreto_ChatID, nLoreto_ChatID };

// ===== Comandi validi (esempi) =====
// const char cmd_status[] PROGMEM = "/status";
// const char cmd_echo[]   PROGMEM = "/echo";

// Variabili di stato
#define BUTTON_PIN 19
bool            canUseNetwork = false;
uint32_t        lastRetryTime = 0;
const uint32_t  retryInterval = 30000; // 30 secondi tra i tentativi di scansione se disconnesso

// istanze
lnWiFiManagerNB wifiManager;
lnTimeClock     timeClock;
lnTelegram      tgBot;

// --- WIFI-CALLBACK: Qui gestiamo gli eventi di rete
void onConnectionChanged(bool connected) {
    canUseNetwork = connected;

    if (connected) {
        lnLOG_NOTIFY("SISTEMA: Rete ripristinata. Avvio servizi...");
    } else {
        lnLOG_ERROR("SISTEMA: Connessione persa. Servizi in pausa.");
    }
}

void onMinuteCB() {
    lnLOG_INFO("Nuovo minuto!");
}

const char*  validCmds[] PROGMEM = {"/start", "/stop", "/status", "/echo"};

void myTelegramProcessorCB(TBMessage &msg, const char* cmd, const char* payload) {
    lnLOG_DEBUG("%received message: %s", msg.text.c_str());
    // lnLOG_DEBUG("%received payload: %s", payload);

    if (strcasecmp_P(cmd, "/status") == 0) {
        lnLOG_DEBUG("%s processing command: %s", mainLogPrefix, cmd);
        tgBot.reply(msg, "📊 Status OK\nIP: <code>%s</code>", WiFi.localIP().toString().c_str());
    }
    else if (strcasecmp_P(cmd, "/echo") == 0) {
        lnLOG_DEBUG("%s processing command: %s", mainLogPrefix, cmd);
        tgBot.reply(msg, "Hai scritto: <i>%s</i>", payload);
    }
}

void wifiInit() {
    // --- wifi CREDENTIALS
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


    tgBot.setAuthorizedIDs(allowedIDs, sizeof(allowedIDs) / sizeof(allowedIDs[0]));
    tgBot.setValidCommands(validCmds, sizeof(validCmds) / sizeof(validCmds[0]));
    tgBot.setCommandCallback(myTelegramProcessorCB);
    tgBot.begin(BOT_TOKEN);
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
            lnLOG_NOTIFY("SISTEMA: WiFi giù, attendo stabilità prima di scansionare...");
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

    // --- LOGICA DEI SERVIZI ---
    // if (!isNetReady) {
    //     uint32_t now = millis();
    //     // ATTENZIONE: Verifica che non ci sia uno scan già in corso o una connessione pendente
    //     if (now - lastRetryTime > retryInterval && WiFi.status() != WL_IDLE_STATUS) {
    //         lnLOG_NOTIFY("SISTEMA: WiFi non pronto, avvio scan...");
    //         wifiManager.startScan();
    //         lastRetryTime = now;
    //     }
    // }


}