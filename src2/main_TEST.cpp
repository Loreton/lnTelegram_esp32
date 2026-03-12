//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//

#ifdef  __ln_MAIN_TEST_MODULE__

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

// --- WIFI-CALLBACK: Qui gestiamo gli eventi di rete
void onConnectionChanged(bool connected) {
    canUseNetwork = connected;

    if (connected) {
        lnLOG_NOTIFY("SISTEMA: Rete ripristinata. Avvio servizi...");
    } else {
        lnLOG_ERROR("SISTEMA: Connessione persa. Servizi in pausa.");
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


void onMinuteCB() {
    lnLOG_INFO("Nuovo minuto!");
}


void myTelegramProcessor(TBMessage &msg) {
    msg.text.trim();


    if (msg.text.equalsIgnoreCase(FPSTR(cmd_status))) {
        char buf[180];
        snprintf(buf, sizeof(buf),
            "📊 <b>STATUS</b>\n"
            "🌐 IP: <code>%s</code>\n"
            "📶 RSSI: <code>%d dBm</code>",
            WiFi.localIP().toString().c_str(),
            WiFi.RSSI()
        );
        tgBot.reply(msg, buf); // Non serve passare "HTML", è il nuovo default
    }
    else if (msg.text.startsWith(FPSTR(cmd_echo))) {
        // Se vedi duplicati qui, controlla che non ci sia un altro
        // bot.getNewMessage(msg) che gira nel codice!
        tgBot.reply(msg, "Hai scritto: xxx" );
    }
    // else if (msg.text.equalsIgnoreCase(FPSTR(cmd_echo))) {
    //     tgBot.reply(msg, "Test 1: Semplice");

    //     // Prova 2: HTML standard
    //     tgBot.reply(msg, "Test 2: <b>Grassetto HTML</b>", "HTML");

    //     // Prova 3: Markdown standard (quello vecchio, non V2)
    //     tgBot.reply(msg, "Test 3: *Grassetto Markdown*", "Markdown");
    // }

    // else if (msg.text.equalsIgnoreCase(FPSTR(cmd_echo))) {
    //     // Il modulo lo gestisce già internamente, ma se volessi farlo qui:
    //     tgBot.reply(msg, "Ti vedo!");
    //     char buf[150];
    //     snprintf(buf, sizeof(buf), "<b>STATUS SISTEMA</b> Ti vedo!");
    //     tgBot.reply(msg, buf, "HTML");
    //     tgBot.reply(msg, "Ti vedo2!");
    // }
    else {
        // COMANDO ERRATO: Rispondi solo all'utente maldestro
        String warning = "❓ Comando sconosciuto: " + msg.text;
        tgBot.reply(msg, warning.c_str());

        lnLOG_WARNING("Comando ignoto da %lld", msg.sender.id);
    }
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
    tgBot.setAuthorizedIDs(allowedIDs, sizeof(allowedIDs) / sizeof(allowedIDs[0]));
    tgBot.setCommandCallback(myTelegramProcessor);
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




#endif