//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//

#include "lnTelegram.h"

// Costructor
LnTelegram::LnTelegram() : bot(client) {}

void LnTelegram::init(const char* botToken,
                          const int64_t allowedIDs[],
                          uint8_t idCount,
                          const char* const allowedCommands[],
                          uint8_t cmdCount,
                          CommandCallback cb) {
    _allowedIDs = allowedIDs;
    _idCount = idCount;
    _allowedCommands = allowedCommands;
    _cmdCount = cmdCount;
    _callback = cb;

    client.setInsecure();        // necessario per HTTPS Telegram
    bot.setTelegramToken(botToken);
    // bot.begin();


    /* configurazione DNS
        IPAddress local_IP;
        IPAddress gateway;
        IPAddress subnet;
        IPAddress dns(8,8,8,8);  // Google DNS
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns);

        oppure tutto statico....

        IPAddress local_IP(192,168,1,50);
        IPAddress gateway(192,168,1,1);
        IPAddress subnet(255,255,255,0);
        IPAddress dns(8,8,8,8);
        WiFi.config(local_IP, gateway, subnet, dns);
    */


}
void LnTelegram::loop() {
    if (m_wifiScanning)
        return;

    if (WiFi.status() != WL_CONNECTED) {
        m_started = false;
        return;
    }

    // avvio Telegram una sola volta
    if (!m_started) {

        if (millis() - m_lastReconnectAttempt < 5000)
            return;

        m_lastReconnectAttempt = millis();

        Serial.println("Telegram connecting...");
        bot.begin();

        m_started = true;
        Serial.println("Telegram connected...");
    }

    TBMessage msg;

    if (!bot.getNewMessage(msg))
        return;

    int64_t chat_id = msg.chatId;

    if (!isAuthorized(chat_id)) {
        sendMsg(chat_id, "Utente non autorizzato");
        return;
    }

    if (m_busy) {
        sendMsg(chat_id, "Sistema occupato");
        return;
    }

    char command[MAX_CMD_LEN];
    char payload[MAX_PAYLOAD_LEN];

    parseCommand(msg.text.c_str(), command, payload);

    if (!isValidCommand(command)) {
        sendMsg(chat_id, "Comando sconosciuto");
        return;
    }

    if (_callback)
        _callback(msg, command, payload);
}



void LnTelegram::sendMsg(int64_t chat_id, const char* text) {

    if (!m_started)
        return;

    if (WiFi.status() != WL_CONNECTED)
        return;

    bot.sendTo(chat_id, text);
}

void LnTelegram::setBusy(bool state) {
    m_busy = state;
}


bool LnTelegram::isAuthorized(int64_t id) {
    for (uint8_t i = 0; i < _idCount; i++) {
        if (id == _allowedIDs[i])
            return true;
    }
    return false;
}


bool LnTelegram::isValidCommand(const char* cmd) {
    char buffer[MAX_CMD_LEN];

    for (uint8_t i = 0; i < _cmdCount; i++) {

        strcpy_P(buffer, (PGM_P)pgm_read_ptr(&_allowedCommands[i]));

        if (strcmp(cmd, buffer) == 0)
            return true;
    }

    return false;
}

void LnTelegram::parseCommand(const char* text, char* command, char* payload) {
    // copia sicura
    char buffer[160];
    strncpy(buffer, text, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    // --- separa comando e payload ---
    char* space = strchr(buffer, ' ');

    if (space) {
        *space = '\0';
        strncpy(payload, space + 1, MAX_PAYLOAD_LEN);
        payload[MAX_PAYLOAD_LEN - 1] = '\0';
    } else {
        payload[0] = '\0';
    }

    // --- rimuove eventuale "@botname" ---
    char* at = strchr(buffer, '@');
    if (at)
        *at = '\0';

    strncpy(command, buffer, MAX_CMD_LEN);
    command[MAX_CMD_LEN - 1] = '\0';
}


void LnTelegram::setWifiScanning(bool state) {
    m_wifiScanning = state;
}