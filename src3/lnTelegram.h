#pragma once
#include <Arduino.h>
#include <AsyncTelegram2.h>

#define MAX_CMD_LEN      32
#define MAX_PAYLOAD_LEN  128

// Callback avanzata con comando e payload già estratti
typedef void (*TelegramCommandCallback)(TBMessage &msg, const char* cmd, const char* payload);

class lnTelegram {
    public:
        lnTelegram();
        void begin(const char* token);
        void update(bool isNetworkAvailable, bool isTimeValid);

        void setCommandCallback(TelegramCommandCallback cb) { _callback = cb; }
        void setAuthorizedIDs(const int64_t* ids, size_t count);
        void setValidCommands(const char** cmds, size_t count);

        // Invio messaggi con formattazione printf integrata
        bool sendMessage(int64_t id, const char* format, ...);
        bool reply(TBMessage &msg, const char* format, ...);
        bool broadcast(const char* format, ...);

    private:
        WiFiClientSecure m_client;
        AsyncTelegram2   m_bot;

        TelegramCommandCallback _callback = nullptr;
        const int64_t* m_authorizedIDs = nullptr;
        size_t m_authCount = 0;
        const char** m_validCommands = nullptr;
        size_t m_validCmdsCount = 0;

        bool m_isActive = false;
        bool m_firstConnectDone = false;

        bool isAuthorized(int64_t id);
        bool isValidCommand(const char* cmd);
        void parseCommand(const char* text, char* command, char* payload);
        void handleIncomingMessage(TBMessage &msg);
};