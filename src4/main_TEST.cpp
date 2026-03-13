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
#include "lnTimeScheduler.h"

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
// const char cmd_status[] PROGMEM = "/status";
// const char cmd_echo[]   PROGMEM = "/echo";

const char*  validCmds[] PROGMEM = {"/start", "/stop", "/status", "/echo"};

// Variabili di stato
#define BUTTON_PIN 19
bool            canUseNetwork = false;


// istanze
lnWiFiManagerNB wifiManager;
lnTimeClock     timeClock;
lnTelegram      tgBot;
lnTimeScheduler timeSched(&timeClock); // passa la classe

// ================  CALLBACKs START ===============================
// #########################################################
// # --- WIFI-CALLBACK
// #########################################################
void onConnectionChanged(bool connected) {
    static uint8_t disconnection_counter=0;
    canUseNetwork = connected;

    if (connected) {
        disconnection_counter=0;
        lnLOG_NOTIFY("%s Rete ripristinata. Avvio servizi...", mainLogPrefix);
    } else {
        disconnection_counter++;
        lnLOG_ERROR("%s Connessione persa. Servizi in pausa. (disconnection_counter: %d)", mainLogPrefix, disconnection_counter);
        lnLOG_WARNING("%s Free heap: %lld", mainLogPrefix, ESP.getFreeHeap());
    }
    if (disconnection_counter > 10) {
        lnLOG_WARNING("%s Disconnessioni totali: %d memory: %lld", mainLogPrefix, disconnection_counter, ESP.getFreeHeap());
        ESP.restart();
    }
}



// bool newTgMsg_has_arrived=false;
// TBMessage* newTgMsg;
// TBMessage newTgMsg1;


// TBMessage newTgMsg;         // Buffer per la copia del messaggio
// bool hasNewMsg = false;     // Flag di stato

// #########################################################
// # --- Telegram-CALLBACK
// # --- fatta per liberare subito la caalBack
// #########################################################
// void myTelegramProcessorCB(TBMessage &msg, const char* command, const char* payload) {
// void myTelegramProcessorCB(TBMessage &msg) {
//     // Se il buffer è libero, carichiamo il nuovo messaggio
//     if (!hasNewMsg) {
//         newTgMsg = msg; // Copia profonda di tutti i campi (text, sender, etc.)
//         hasNewMsg = true;
//         lnLOG_DEBUG("TG: Messaggio copiato nel buffer.");
//     } else {
//         lnLOG_WARNING("TG: Buffer occupato, messaggio scartato.");
//     }
// }

// void processTelegramMessage() {
//     // Qui lavoriamo sulla COPIA (newTgMsg)
//     lnLOG_INFO("TG: Elaborazione comando: %s", newTgMsg.text.c_str());

//     // Esempio di risposta usando la copia
//     tgBot.reply(newTgMsg, "Ho ricevuto il tuo comando!");

//     // IMPORTANTE: resettiamo il flag alla fine
//     hasNewMsg = false;
// }

// void processTelegramMessage() {
//     // Creiamo una copia locale di lavoro se il processo è molto lungo
//     TBMessage currentJob = newTgMsg;
//     hasNewMsg = false; // Liberiamo SUBITO il buffer per il prossimo messaggio

//     // Ora lavoriamo su currentJob con tutta la calma necessaria...
//     if (currentJob.text == "/status") {
//         // ...
//     }
// }

#if 0
// #########################################################
// # --- Telegram process message
// #########################################################
void processTelegramMessage() {
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
#endif


// #########################################################
// # --- lnTimeClock-CALLBACK
// #########################################################
void onHourCB() {
    lnLOG_INFO("Nuova ora!");
    tgBot.sendMsg(nLoreto_ChatID, "I'm alive...");
}
// ================  CALLBACKs END =================================




// #########################################################
// #    WiFi setupe
// #########################################################
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



// #########################################################
// #    S E T U P
// #########################################################
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
    tgBot.setValidCommands(validCmds, sizeof(validCmds) / sizeof(validCmds[0]));
    // tgBot.setCommandCallback(myTelegramProcessorCB);

    timeSched.onHour(onHourCB);
}




//#########################################################
//#    L O O P
//#########################################################
const int16_t RETRY_INTERVAL=60000; // 60 secondi tra i tentativi di scansione se disconnesso

void loop() {
    static bool firstRun = true;
    uint32_t  lastRetryTime;
    int8_t wifiRetryCounter;

    if (firstRun) {
        firstRun      = false;
        wifiRetryCounter = 0;
        lastRetryTime = 0;
    }

    wifiManager.update();

    bool isNetReady = wifiManager.isConnected();
    timeClock.update(isNetReady);


    bool timeOK = timeClock.isTimeValid();
    tgBot.update(isNetReady, timeOK);



    // --- Tentare il rescan dopo un timeout ---
    if (!isNetReady) {
        uint32_t now = millis();
        // Attendi almeno 10 secondi dall'ultima disconnessione prima di scansionare
        if (now - lastRetryTime > RETRY_INTERVAL) {
            lnLOG_NOTIFY("%s WiFi giù, attendo stabilità prima di scansionare... (wifiRetryCounter: %d)", mainLogPrefix, wifiRetryCounter);
            wifiManager.startScan();
            lastRetryTime = now;
            wifiRetryCounter++;
        }
    }
    if (wifiRetryCounter > 10) {
        lnLOG_WARNING("%s tentativi totali: %d memory: %lld", mainLogPrefix, wifiRetryCounter, ESP.getFreeHeap());
        ESP.restart();
    }


    // --- TEST DISCONNESSIONE MANUALE ---
    // Se premi il pulsante (o colleghi il PIN 19 a GND)
    if (digitalRead(BUTTON_PIN) == LOW && isNetReady) {
        if (wifiManager.isConnected()) {
            wifiManager.disconnect();
            delay(500); // Debounce brutale per il test
        }
    }

    timeSched.everySeconds(1, [](){ // senza callback ogni 2 secondi
        // 2. Controlla se il bot ha depositato un comando
        if (tgBot.hasPendingMessage()) {

            // Recuperiamo il riferimento al Messaggio
            auto& msg = tgBot.getPendingMessage();

            lnLOG_INFO("Processing: %s with payload: %s", msg.command, msg.payload);

            // 3. Esegui la logica
            if (strcasecmp(msg.command, "/status") == 0) {
                tgBot.sendMsg(msg.chatId, "📊 Sistema operativo.");
            }
            else if (strcasecmp(msg.command, "/echo") == 0) {
                tgBot.sendMsg(msg.chatId, "Hai detto: %s", msg.payload);
            }

            // 4. Libera il buffer per il prossimo messaggio
            tgBot.clearPendingMessage();
        }

    });




}




#endif