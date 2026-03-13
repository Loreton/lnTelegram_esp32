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
    m_token = token;

    m_client.setInsecure(); // Per semplicità, o usa certificati se preferisci
    m_client.setTimeout(5); // 5 secondi max per le operazioni socket
    lnLOG_INFO("%s Module initialized.", tgLogPrefix);
}


// ##########################################################
// # --- AUTHORIZED IDs
// ##########################################################
void lnTelegram::setAuthorizedIDs(const int64_t* ids, size_t count) {
    m_authorizedIDs = ids;
    m_authCount = count;
}

bool lnTelegram::isAuthorized(int64_t id) {
    for (size_t i = 0; i < m_authCount; i++) {
        if (m_authorizedIDs[i] == id) return true;
    }
    return false;
}


// ##########################################################
// # --- VALID COMMANDS
// ##########################################################
void lnTelegram::setValidCommands(const char** cmds, size_t count) {
    m_validCommands = cmds;
    m_validCmdsCount = count;
}


// ##########################################################
// #
// ##########################################################
void lnTelegram::update(bool isNetworkAvailable, bool isTimeValid) {
    m_isNetworkAvailable = isNetworkAvailable; // per sendHTTP
    bool all_OK = (isTimeValid * isNetworkAvailable);

    if (!all_OK) {
        if (m_isActive) {
            m_client.stop();
            m_isActive = false;
        }
        return;
    }

    if (!m_isActive) {
        lnLOG_NOTIFY("%s all is ready: net %d - time: %d", tgLogPrefix,  isNetworkAvailable, isTimeValid);
        m_client.setInsecure();
        if (m_bot.begin()) {
            m_isActive = true;
            lnLOG_SUCCESS("%s Bot Ready.", tgLogPrefix);
            // sendMsg()
        } else {
            lnLOG_ERROR("%s Bot begin error!", tgLogPrefix);
        }
        return;
    }


    // -- per alleggerire la richiesta verso telegram (non so se abbia senso)
    if (millis() - m_lastGetMessage > 500) {
        TBMessage msg;
        if (m_bot.getNewMessage(msg)) {
            lnLOG_INFO("Message from %s", msg.sender.username);
            handleIncomingMessage(msg);
        }
        m_lastGetMessage = millis();
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



// ##########################################################
// #
// ##########################################################
bool lnTelegram::sendMsg(int64_t chat_id, const char* text) {
    if (!m_isActive && m_isNetworkAvailable) { // tentiamo la strada HTTPS
        return this->sendHTTP(chat_id, text);
    }
    return m_bot.sendTo(chat_id, text);
}
