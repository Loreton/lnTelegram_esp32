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


        // pending Task
        // Definiamo una struttura "leggera" per il comando pendente
        struct PendingMessage {
            int64_t chatId;
            char command[MAX_CMD_LEN];
            char payload[MAX_PAYLOAD_LEN];
            bool exists = false; // Il nostro flag
        };


        // Aggiungiamo un metodo per verificare se c'è un task da processare
        // bool hasPendingTask() { return m_pending.exists; }
        // PendingTask& getPendingTask() { return m_pending; }
        // void clearPendingTask() { m_pending.exists = false; }

        // Gestione dei messaggi ricevuti (Polling-style)
        bool hasPendingMessage() { return m_pending.exists; }
        PendingMessage getPendingMessage() { return m_pending; }
        void clearPendingMessage() { m_pending.exists = false; } // Libera il buffer




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

        // authorized users
        void setAuthorizedIDs(const int64_t* ids, size_t count);
        bool isAuthorized(int64_t id);

        // valid commands
        void setValidCommands(const char** cmds, size_t count);
        void parseCommand(const char* text, char* command, char* payload);

        // void broadcast(const char* message); // Invia un messaggio a tutti gli ID autorizzati

    private:
        WiFiClientSecure m_client;
        AsyncTelegram2   m_bot;

        TelegramCommandCallback m_cmdCallback = nullptr;

        // pending Task
        PendingMessage m_pending; // Una sola istanza pre-allocata in RAM

        bool        m_isActive = false;
        bool        m_isNetworkAvailable = false;
        uint32_t    m_lastGetMessage = 0; // ultimo momento della lettura di nuovi messaggi
        const char*    m_token = nullptr;

        // authorized users
        const int64_t* m_authorizedIDs = nullptr;
        size_t m_authCount = 0;

        // valid commands
        size_t m_validCmdsCount = 0;
        const char** m_validCommands = nullptr;

        // Helper interni
        void handleIncomingMessage(TBMessage &msg);
};