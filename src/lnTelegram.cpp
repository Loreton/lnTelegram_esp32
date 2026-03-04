//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//

#include "lnTelegram.h"

// Costructor
TelegramModule::TelegramModule() : bot(client) {}

void TelegramModule::init(const char* botToken,
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
    bot.begin();

    IPAddress dns(8,8,8,8);  // Google DNS
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns);

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

void TelegramModule::loop() {
    if (WiFi.status() != WL_CONNECTED)
        return;

    TBMessage msg;

    if (!bot.getNewMessage(msg))
        return;

    // char chat_id[20];
    // snprintf(chat_id, sizeof(chat_id), "%lld", msg.chatId);
    int64_t chat_id = msg.chatId;

    // --- autorizzazione ---
    if (!isAuthorized(chat_id)) {
        sendMsg(chat_id, "Utente non autorizzato");
        return;
    }

    // --- sistema occupato ---
    if (busy) {
        sendMsg(chat_id, "Sistema occupato");
        return;
    }

    // --- parsing comando ---
    char command[MAX_CMD_LEN];
    char payload[MAX_PAYLOAD_LEN];

    parseCommand(msg.text.c_str(), command, payload);

    if (!isValidCommand(command)) {
        sendMsg(chat_id, "Comando sconosciuto");
        return;
    }

    if (_callback)
        // _callback(chat_id, command, payload);
        _callback(msg, command, payload);
}

void TelegramModule::sendMsg(int64_t chat_id, const char* text) {
    if (WiFi.status() != WL_CONNECTED)
        return;

    bot.sendTo(chat_id, text);
}


void TelegramModule::setBusy(bool state) {
    busy = state;
}


bool TelegramModule::isAuthorized(int64_t id) {
    for (uint8_t i = 0; i < _idCount; i++) {
        if (id == _allowedIDs[i])
            return true;
    }
    return false;
}


bool TelegramModule::isValidCommand(const char* cmd) {
    char buffer[MAX_CMD_LEN];

    for (uint8_t i = 0; i < _cmdCount; i++) {

        strcpy_P(buffer, (PGM_P)pgm_read_ptr(&_allowedCommands[i]));

        if (strcmp(cmd, buffer) == 0)
            return true;
    }

    return false;
}

void TelegramModule::parseCommand(const char* text, char* command, char* payload) {
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