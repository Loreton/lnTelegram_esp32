//
// updated by ...: Loreto Notarantonio
// Date .........: 08-03-2026 08.57.00
//

#pragma once

#include <WiFi.h>
#include <AsyncTelegram2.h>
#include <pgmspace.h>

#define MAX_MSG_LEN 256
#define MAX_CMD_LEN 32
#define MAX_PAYLOAD_LEN 128

class TelegramModule {

public:

    typedef void (*CommandCallback)(const char* chat_id,
                                    const char* command,
                                    const char* payload);

    TelegramModule();

    void init(const char* botToken,
              const char* const allowedIDs[],
              uint8_t idCount,
              const char* const allowedCommands[],
              uint8_t cmdCount,
              CommandCallback cb);

    void loop();

    void sendMsg(const char* chat_id, const char* text);

    void setEVmoving(bool state);

private:

    AsyncTelegram2 bot;

    const char* const* _allowedIDs;
    uint8_t _idCount;

    const char* const* _allowedCommands;
    uint8_t _cmdCount;

    CommandCallback _callback;

    bool EV_moving = false;

    bool isAuthorized(const char* id);
    bool isValidCommand(const char* cmd);
    void extractCommand(const char* text, char* command);
    void extractPayload(const char* text, char* payload);
};
