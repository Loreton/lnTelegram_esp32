//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//

#ifdef  __ln_MAIN_TEST_MODULE__
const char* mainLogPrefix = "MAIN:";

// #include <ssid_credentials_esp32.h>

#define __I_AM_MAIN_CPP__
#include "lnLogger_Class.h"
#include "lnWiFiManager.h"
#include "lnTimeClock.h"
#include "lnTimeScheduler.h"
#include "lnTelegram.h"

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
        lnLOG_WARNING("%s Free heap: %d", mainLogPrefix, ESP.getFreeHeap());
    }
    if (disconnection_counter > 10) {
        lnLOG_WARNING("%s Disconnessioni totali: %d memory: %d", mainLogPrefix, disconnection_counter, ESP.getFreeHeap());
        ESP.restart();
    }
}



// #########################################################
// # --- Telegram process message
// #########################################################
void processTelegramMessage() {
    auto& msg = tgBot.getPendingMessage();

    int64_t chatId = msg.chatId;
    const char* payload = msg.payload;
    const char* command = msg.command;

    // lnLOG_INFO("%s   chatID:  %lld", mainLogPrefix, chatId);
    // lnLOG_INFO("%s   command: %s",   mainLogPrefix, command);
    // lnLOG_INFO("%s   payload: %s",   mainLogPrefix, payload);

    lnLOG_INFO("%s chatID:  %lld - command: %s - payload: %s", mainLogPrefix, chatId, command, payload);


    if (strcmp(command, "/start") == 0) {
        tgBot.sendMsg(chatId, "Bot avviato correttamente");
    }

    else if (strcmp(command, "/status") == 0) {
        tgBot.sendMsg(chatId, "Sistema OK - ESP32 online");
    }

    else if (strcmp(command, "/echo") == 0) {
        if (strlen(payload) == 0)
            tgBot.sendMsg(chatId, "Uso: /echo testo");
        else
            tgBot.sendMsg(chatId, payload);
    }

    else if (strcmp(command, "/whoami") == 0) {

        char buffer[256];

        snprintf(buffer, sizeof(buffer),
            "command:  %s\n"
            "chatId:   %lld\n"
            "username: %s\n"
            "firstName:%s\n"
            "lastName: %s",
            command,
            msg.chatId,
            strlen(msg.sender_username) ? msg.sender_username : "-",
            msg.sender_firstName,
            strlen(msg.sender_lastName) ? msg.sender_lastName : "-"
        );

        tgBot.sendMsg(chatId, buffer);

    }
}


// #########################################################
// # --- lnTimeClock-CALLBACK
// #########################################################
void onHourCB() {
    lnLOG_INFO("Nuova ora!");
    tgBot.sendMsg(nLoreto_ChatID, "I'm alive...");
}


bool onMinute=false;
void onMinuteCB() {
    onMinute=true;
    // lnLOG_DEBUG("Nuovo minuto!");
    // lnLOG_INFO("%s free memory: %d - isNetReady: %d - isTimeValid: %d", mainLogPrefix, ESP.getFreeHeap(), wifiManager.isConnected(), timeClock.isTimeValid() );
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
    timeSched.onMinute(onMinuteCB);
}




//#########################################################
//#    L O O P
//#########################################################
void loop() {
    // Queste variabili "sopravvivono" tra un giro e l'altro del loop
    // static bool  firstRun = true;
    static uint32_t lastRetryTime = 0;
    static int8_t wifiRetryCounter = 0;

    uint32_t now = millis(); // 'static' non serve qui se lo assegni ogni volta

    now = millis();

    wifiManager.update();
    timeSched.update();

    bool isNetReady = wifiManager.isConnected();
    timeClock.update(isNetReady);


    bool timeOK = timeClock.isTimeValid();
    tgBot.update(isNetReady, timeOK);



    if (onMinute) {
        onMinute=false;
        lnLOG_DEBUG("%s now: %lu - lastRetryTime: %lu - diff: %lu", mainLogPrefix, now, lastRetryTime, (now - lastRetryTime));
        lnLOG_INFO("%s free memory: %d - isNetReady: %d - isTimeValid: %d", mainLogPrefix, ESP.getFreeHeap(), isNetReady, timeOK );
    }

    // --- Tentare il rescan dopo un timeout ---
    if (isNetReady) {
        wifiRetryCounter=0;
    } else {
        // Attendi almeno 60 secondi dall'ultima disconnessione prima di scansionare
        if (now - lastRetryTime > 60000) {
            lnLOG_NOTIFY("%s WiFi giù, attendo stabilità prima di scansionare... (wifiRetryCounter: %d)", mainLogPrefix, wifiRetryCounter);
            wifiManager.startScan();
            lastRetryTime = now;
            wifiRetryCounter++;
        }
    }

    if (wifiRetryCounter > 10) {
        lnLOG_WARNING("%s tentativi totali: %d memory: %d", mainLogPrefix, wifiRetryCounter, ESP.getFreeHeap());
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

    timeSched.everySeconds(2, [](){ // senza callback ogni 2 secondi
    // 2. Controlla se il bot ha depositato un comando
        if (tgBot.hasPendingMessage()) {
            lnLOG_DEBUG("%s telegram message processing starting", mainLogPrefix);
            processTelegramMessage();
            // 4. Libera il buffer per il prossimo messaggio
            tgBot.clearPendingMessage();
            lnLOG_DEBUG("%s telegram message processing completed", mainLogPrefix);
        }

    });




}




#endif