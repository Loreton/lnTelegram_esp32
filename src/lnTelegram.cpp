//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//

#include "lnTelegram.h"
#include <string.h>

TelegramModule::TelegramModule() {}

void TelegramModule::init(const char* botToken,
                          const char* const allowedIDs[],
                          uint8_t idCount,
                          const char* const allowedCommands[],
                          uint8_t cmdCount,
                          CommandCallback cb)
{
    _allowedIDs = allowedIDs;
    _idCount = idCount;

    _allowedCommands = allowedCommands;
    _cmdCount = cmdCount;

    _callback = cb;

    bot.setTelegramToken(botToken);
    bot.begin();
}

void TelegramModule::loop()
{
    if (WiFi.status() != WL_CONNECTED)
        return;   // dormiente se WiFi non connesso

    TBMessage msg;

    if (!bot.getNewMessage(msg))
        return;

    char chat_id[20];
    snprintf(chat_id, sizeof(chat_id), "%lld", msg.chatId);

    if (!isAuthorized(chat_id)) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                 "Non autorizzato\nNome: %s\nID: %s",
                 msg.sender.firstName.c_str(),
                 chat_id);
        sendMsg(chat_id, buffer);
        return;
    }

    if (EV_moving) {
        sendMsg(chat_id, "Sistema occupato (EV moving)");
        return;
    }

    char text[MAX_MSG_LEN];
    strncpy(text, msg.text.c_str(), sizeof(text));
    text[sizeof(text)-1] = '\0';

    char command[MAX_CMD_LEN];
    char payload[MAX_PAYLOAD_LEN];

    extractCommand(text, command);
    extractPayload(text, payload);

    if (!isValidCommand(command)) {
        sendMsg(chat_id, "Comando sconosciuto");
        return;
    }

    if (_callback != nullptr)
        _callback(chat_id, command, payload);
}

void TelegramModule::sendMsg(const char* chat_id, const char* text)
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    bot.sendMessage(chat_id, text, "");
}

void TelegramModule::setEVmoving(bool state)
{
    EV_moving = state;
}

bool TelegramModule::isAuthorized(const char* id)
{
    char buffer[20];

    for (uint8_t i = 0; i < _idCount; i++) {

        strcpy_P(buffer, (PGM_P)pgm_read_ptr(&_allowedIDs[i]));

        if (strcmp(id, buffer) == 0)
            return true;
    }

    return false;
}

bool TelegramModule::isValidCommand(const char* cmd)
{
    char buffer[MAX_CMD_LEN];

    for (uint8_t i = 0; i < _cmdCount; i++) {

        strcpy_P(buffer, (PGM_P)pgm_read_ptr(&_allowedCommands[i]));

        if (strcmp(cmd, buffer) == 0)
            return true;
    }

    return false;
}

void TelegramModule::extractCommand(const char* text, char* command)
{
    uint8_t i = 0;

    while (text[i] != ' ' && text[i] != '\0' && i < MAX_CMD_LEN - 1) {
        command[i] = text[i];
        i++;
    }

    command[i] = '\0';
}

void TelegramModule::extractPayload(const char* text, char* payload)
{
    const char* space = strchr(text, ' ');

    if (space == nullptr) {
        payload[0] = '\0';
        return;
    }

    strncpy(payload, space + 1, MAX_PAYLOAD_LEN);
    payload[MAX_PAYLOAD_LEN - 1] = '\0';
}