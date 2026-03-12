//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//
#include "lnTelegram.h"
#include <lnLogger_Class.h>

const char* tgLog = "TG: ";

// ##########################################################
// #
// ##########################################################
lnTelegram::lnTelegram() : m_bot(m_client) {}

// ##########################################################
// #
// ##########################################################
void lnTelegram::begin(const char* token) {
    m_client.setInsecure(); // Per semplicità, o usa certificati se preferisci
    m_bot.setUpdateTime(2000); // Polling ogni 2 secondi
    m_client.setTimeout(5); // 5 secondi max per le operazioni socket
    m_bot.setTelegramToken(token);
    lnLOG_INFO("%sModule initialized.", tgLog);
}

// ##########################################################
// #
// ##########################################################
void lnTelegram::setAuthorizedIDs(const int64_t* ids, size_t count) {
    m_authorizedIDs = ids;
    m_authCount = count;
}

// ##########################################################
// #
// ##########################################################
bool lnTelegram::isAuthorized(int64_t id) {
    for (size_t i = 0; i < m_authCount; i++) {
        if (m_authorizedIDs[i] == id) return true;
    }
    return false;
}

void lnTelegram::update(bool isNetworkAvailable, bool isTimeValid) {
    if (!isNetworkAvailable) {
        if (m_isActive) {
            m_client.stop();
            m_isActive = false;
        }
        return;
    }

    if (!m_isActive) {
        m_client.setInsecure();
        if (m_bot.begin()) {
            m_isActive = true;
            lnLOG_SUCCESS("%sBot Ready.", tgLog);
        }
        return;
    }

    if (!isTimeValid) return;

    // --- MESSAGGIO DI BOOT ---
    if (!m_firstConnectDone) {
        // Aumentiamo il delay a 5 secondi: diamo tempo al server di
        // "digerire" la nostra nuova sessione SSL dopo il sync dell'ora
        static uint32_t bootDelay = millis();
        if (millis() - bootDelay > 5000) {
            lnLOG_INFO("%sSending startup message...", tgLog);
            broadcast("🚀 <b>Sistema Online!</b>");
            m_firstConnectDone = true;
        }
        return;
    }

    TBMessage msg;
    if (m_bot.getNewMessage(msg)) {
        handleIncomingMessage(msg);
    }
}

// Rendiamo HTML il default assoluto
bool lnTelegram::sendMessage(int64_t id, const char* msg, const char* parseMode) {
    if (!m_isActive || msg == nullptr) return false;

    // Se parseMode è vuoto, usa "HTML" di default
    const char* mode = (strlen(parseMode) == 0) ? "HTML" : parseMode;

    return m_bot.sendTo(id, msg, mode);
}

bool lnTelegram::reply(TBMessage &msg, const char* txt, const char* parseMode) {
    return sendMessage(msg.sender.id, txt, parseMode);
}

void lnTelegram::broadcast(const char* message, const char* parseMode) {
    if (!m_isActive || m_authCount == 0) return;
    for (size_t i = 0; i < m_authCount; i++) {
        sendMessage(m_authorizedIDs[i], message, parseMode);
    }
}

// ##########################################################
// #
// ##########################################################
void lnTelegram::onBotReady() {
    // Usiamo HTML: i tag sono <b> <i> <code> <pre>
    broadcast("🚀 <b>Sistema Online!</b>\nWiFi connesso e Bot pronto.", "HTML");
}





// ##########################################################
// #
// ##########################################################
void lnTelegram::handleIncomingMessage(TBMessage &msg) {
    // 2. Comando di sistema: /whoami (gestito internamente)
    lnLOG_WARNING("%received message: %s", msg.text.c_str());
    if (msg.text.equalsIgnoreCase("/whoami")) {
        char response[64];
        snprintf(response, sizeof(response), "👤 Nome: %s\n🆔 ID: %lld", msg.sender.username, msg.sender.id);
        m_bot.sendMessage(msg, response);
        return;
    }

    // 1. Controllo Autorizzazione
    if (!isAuthorized(msg.sender.id)) {
        lnLOG_WARNING("%sUnauthorized access attempt from ID: %lld", tgLog, msg.sender.id);
        m_bot.sendMessage(msg, "⛔ Accesso negato. Il tuo ID non è autorizzato.");
        return;
    }


    // 3. Passaggio alla callback esterna per i comandi validi
    if (m_cmdCallback) {
        // Qui la tua callback deciderà se il comando è valido o meno
        m_cmdCallback(msg);
    }
}