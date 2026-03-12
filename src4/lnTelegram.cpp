//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//
#include "lnTelegram.h"
#include <lnLogger_Class.h>

const char* tgLogPrefix = "TG: ";

// ##########################################################
// #
// ##########################################################
lnTelegram::lnTelegram() : m_bot(m_client) {}

// ##########################################################
// #
// ##########################################################
void lnTelegram::begin(const char* token) {
    m_bot.setUpdateTime(2000); // Polling ogni 2 secondi
    m_bot.setTelegramToken(token);

    m_client.setInsecure(); // Per semplicità, o usa certificati se preferisci
    m_client.setTimeout(5); // 5 secondi max per le operazioni socket
    lnLOG_INFO("%s Module initialized.", tgLogPrefix);
}





void lnTelegram::update(bool isNetworkAvailable, bool isTimeValid) {
    // lnLOG_NOTIFY("%s is net ready...: %d", tgLogPrefix, isNetworkAvailable);
    // lnLOG_NOTIFY("%s is time ok.....: %d", tgLogPrefix,  isTimeValid);

    bool all_OK = (isTimeValid * isNetworkAvailable);

    if (!all_OK) {
        // lnLOG_NOTIFY("%s NOT all is ready: net %d - time: %d", tgLogPrefix,  isNetworkAvailable, isTimeValid);
        if (m_isActive) {
            m_client.stop();
            m_isActive = false;
        }
        return;
    }

    // if (!isTimeValid) return;

    if (!m_isActive) {
        lnLOG_NOTIFY("%s all is ready: net %d - time: %d", tgLogPrefix,  isNetworkAvailable, isTimeValid);
        m_client.setInsecure();
        if (m_bot.begin()) {
            m_isActive = true;
            lnLOG_SUCCESS("%s Bot Ready.", tgLogPrefix);
        } else {
            lnLOG_ERROR("%s Bot begin error!", tgLogPrefix);
        }
        return;
    }

    uint32_t        now = millis();


        // TBMessage msg;
        // if (m_bot.getNewMessage(msg)) {
        //     lnLOG_INFO("Message from %s", msg.sender.username);
        //     handleIncomingMessage(msg);
        // }

    // -- per alleggerire la richiesta verso telegram (non so se abbia senso)
    if (now - m_lastGetMessage > 1000) {
        TBMessage msg;
        if (m_bot.getNewMessage(msg)) {
            lnLOG_INFO("Message from %s", msg.sender.username);
            handleIncomingMessage(msg);
        }
        m_lastGetMessage = now;
    }

}







// ##########################################################
// #
// ##########################################################
void lnTelegram::parseCommand(const char* text, char* command, char* payload) {
    char buffer[MAX_CMD_LEN + MAX_PAYLOAD_LEN + 2];
    strncpy(buffer, text, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char* space = strchr(buffer, ' ');
    if (space) {
        *space = '\0';
        strncpy(payload, space + 1, MAX_PAYLOAD_LEN);
        payload[MAX_PAYLOAD_LEN - 1] = '\0';
    } else {
        payload[0] = '\0';
    }

    char* at = strchr(buffer, '@');
    if (at) *at = '\0';

    strncpy(command, buffer, MAX_CMD_LEN);
    command[MAX_CMD_LEN - 1] = '\0';
}

// ##########################################################
// #
// ##########################################################
void lnTelegram::handleIncomingMessage(TBMessage &msg) {
    lnLOG_WARNING("%s received message: %s", tgLogPrefix, msg.text.c_str());

    char command[MAX_CMD_LEN];
    char payload[MAX_PAYLOAD_LEN];
    parseCommand(msg.text.c_str(), command, payload);

    // 3. Passaggio alla callback esterna per i comandi validi
    if (m_cmdCallback) {
        // Qui la tua callback deciderà se il comando è valido o meno
        m_cmdCallback(msg, command, payload);
    }
}



bool lnTelegram::sendMsg(int64_t chat_id, const char* text) {
    if (!m_isActive) return false;
    return m_bot.sendTo(chat_id, text);
}
