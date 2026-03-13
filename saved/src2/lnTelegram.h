#pragma once
#include <Arduino.h>
#include <AsyncTelegram2.h>
#include <SSLClient.h>

// Tipo di callback: riceve il messaggio completo per poter rispondere o estrarre dati
typedef void (*TelegramCommandCallback)(TBMessage &msg);

class lnTelegram {
    public:
        lnTelegram();

        // Inizializzazione
        void begin(const char* token);

        // Invia a un ID specifico
        bool sendMessage(int64_t id, const char* msg, const char* parseMode = "");

        // Risponde all'ultimo messaggio ricevuto (comodo per la callback)
        bool reply(TBMessage &msg, const char* txt, const char* parseMode = "");

        // Invia a tutti i membri della whitelist
        void broadcast(const char* message, const char* parseMode = "");

        // Il cuore: riceve lo stato della rete dal main
        // void update(bool isNetworkAvailable);
        void update(bool isNetworkAvailable, bool isTimeValid);

        // Configurazione
        void setCommandCallback(TelegramCommandCallback cb) { m_cmdCallback = cb; }
        void setAuthorizedIDs(const int64_t* ids, size_t count);

        // void broadcast(const char* message); // Invia un messaggio a tutti gli ID autorizzati

    private:
        WiFiClientSecure m_client;
        AsyncTelegram2   m_bot;

        TelegramCommandCallback m_cmdCallback = nullptr;

        const int64_t* m_authorizedIDs = nullptr;
        size_t m_authCount = 0;

        bool m_isActive = false;
        bool m_wasActive = false; // Per rilevare la riconnessione
        bool m_firstConnectDone = false; // Per inviare il messaggio di boot una sola volta

        // Helper interni
        bool isAuthorized(int64_t id);
        void handleIncomingMessage(TBMessage &msg);
        void onBotReady(); // Azioni da fare appena il bot è online
};