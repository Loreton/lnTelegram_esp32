#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTelegram2.h>
#include <pgmspace.h>

#define MAX_CMD_LEN      32
#define MAX_PAYLOAD_LEN  128

class TelegramModule {

public:

    // typedef void (*CommandCallback)(
    //     const char* chat_id,
    //     const char* command,
    //     const char* payload
    // );
    // typedef void (*CommandCallback)(
    //     int64_t chat_id,
    //     const char* command,
    //     const char* payload
    // );
    typedef void (*CommandCallback)(
        const TBMessage& msg,
        const char* command,
        const char* payload
    );
    TelegramModule();

    void init(const char* botToken,
              const int64_t allowedIDs[],
              uint8_t idCount,
              const char* const allowedCommands[],
              uint8_t cmdCount,
              CommandCallback cb);

    void loop();

    // void sendMsg(const char* chat_id, const char* text);
    void sendMsg(int64_t chat_id, const char* text);

    void setBusy(bool state);

private:

    WiFiClientSecure client;
    AsyncTelegram2 bot;

    // const char* const* _allowedIDs;
    const int64_t* _allowedIDs;
    uint8_t _idCount;

    const char* const* _allowedCommands;
    uint8_t _cmdCount;

    CommandCallback _callback;
    unsigned long lastTelegramSuccess = 0;
    bool started = false;

    bool busy = false;

    bool isAuthorized(int64_t id);
    bool isValidCommand(const char* cmd);

    void parseCommand(const char* text, char* command, char* payload);
};