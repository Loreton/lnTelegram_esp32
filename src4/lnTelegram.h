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

        // Inizializzazione
        void begin(const char* token);

        // Invia a un ID specifico
        // bool sendMessage(int64_t id, const char* msg, const char* parseMode = "");
        bool sendMsg(int64_t chat_id, const char* text);
        bool sendHTTP(int64_t chat_id, const char* msg);
        uint16_t urlEncode(const char* src, char* dest);
        // Risponde all'ultimo messaggio ricevuto (comodo per la callback)
        // bool reply(TBMessage &msg, const char* txt, const char* parseMode = "");

        // Invia a tutti i membri della whitelist
        // void broadcast(const char* message, const char* parseMode = "");

        // Il cuore: riceve lo stato della rete dal main
        // void update(bool isNetworkAvailable);
        void update(bool isNetworkAvailable, bool isTimeValid);

        // Configurazione
        void setCommandCallback(TelegramCommandCallback cb) { m_cmdCallback = cb; }

        // void setAuthorizedIDs(const int64_t* ids, size_t count);

        // void broadcast(const char* message); // Invia un messaggio a tutti gli ID autorizzati

    private:
        WiFiClientSecure m_client;
        AsyncTelegram2   m_bot;

        TelegramCommandCallback m_cmdCallback = nullptr;

        // const int64_t* m_authorizedIDs = nullptr;
        // size_t m_authCount = 0;

        bool        m_isActive = false;
        uint32_t    m_lastGetMessage = 0; // ultimo momento della lettura di nuovi messaggi
        const char*    m_token = nullptr;

        // Helper interni
        void handleIncomingMessage(TBMessage &msg);
        void parseCommand(const char* text, char* command, char* payload);
};