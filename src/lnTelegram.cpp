//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//
#include "lnTelegram.h"
#include <lnLogger_Class.h>

const char* tgLogPrefix = "TG:  ";

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
            sendMsg(m_authorizedIDs[0], "bot just started!");
        } else {
            lnLOG_ERROR("%s Bot begin error!", tgLogPrefix);
        }
        return;
    }


    // -- per alleggerire la richiesta verso telegram (non so se abbia senso)
    if (millis() - m_lastGetMessage > 500) {
        TBMessage msg;
        if (m_bot.getNewMessage(msg)) {
            lnLOG_INFO("%s Message from %s", tgLogPrefix, msg.sender.username);
            handleIncomingMessage(msg);
        }
        m_lastGetMessage = millis();
    }

}







// ##########################################################
// #
// ##########################################################
// void lnTelegram::parseCommand(const char* text, char* command, char* payload) {
//     char buffer[MAX_CMD_LEN + MAX_PAYLOAD_LEN + 2];
//     strncpy(buffer, text, sizeof(buffer));
//     buffer[sizeof(buffer) - 1] = '\0';

//     char* space = strchr(buffer, ' ');
//     if (space) {
//         *space = '\0';
//         strncpy(payload, space + 1, MAX_PAYLOAD_LEN);
//         payload[MAX_PAYLOAD_LEN - 1] = '\0';
//     } else {
//         payload[0] = '\0';
//     }

//     char* at = strchr(buffer, '@');
//     if (at) *at = '\0';

//     strncpy(command, buffer, MAX_CMD_LEN);
//     command[MAX_CMD_LEN - 1] = '\0';
// }

void lnTelegram::parseCommand(const char* text, char* command, char* payload) {
    // 1. Cerchiamo lo spazio che separa comando da payload
    const char* space = strchr(text, ' ');

    if (space) {
        // Se c'è uno spazio, copiamo tutto quello che viene DOPO nel payload
        snprintf(payload, MAX_PAYLOAD_LEN, "%s", space + 1);

        // Per il comando, dobbiamo copiare solo fino allo spazio.
        // Usiamo il modificatore "%.*s" di snprintf per limitare la lunghezza
        int cmdLen = space - text;
        snprintf(command, MAX_CMD_LEN, "%.*s", cmdLen, text);
    } else {
        // Se non c'è spazio, il payload è vuoto e il comando è tutto il testo
        payload[0] = '\0';
        snprintf(command, MAX_CMD_LEN, "%s", text);
    }

    // 2. Gestione "@botname": se il comando contiene '@', tagliamo lì
    char* at = strchr(command, '@');
    if (at) {
        *at = '\0';
    }
}

// ##########################################################
// #
// ##########################################################
/*void lnTelegram::handleIncomingMessage(TBMessage &msg) {
    lnLOG_WARNING("%s received message: %s", tgLogPrefix, msg.text.c_str());

    char command[MAX_CMD_LEN];
    char payload[MAX_PAYLOAD_LEN];
    parseCommand(msg.text.c_str(), command, payload);

    // 3. Passaggio alla callback esterna per i comandi validi
    if (m_cmdCallback) {
        // Qui la tua callback deciderà se il comando è valido o meno
        m_cmdCallback(msg, command, payload);
    }
}*/


// ##########################################################
// #
// ##########################################################
/*void lnTelegram::handleIncomingMessage(TBMessage &msg) {
    // Se c'è già un messaggio che attende di essere processato,
    // ignoriamo quelli nuovi per non sovrascrivere la memoria.
    if (m_pending.exists) {
        lnLOG_WARNING("%s Buffer busy, skipping message.", tgLogPrefix);
        return;
    }

    if (!isAuthorized(msg.chatId)) {
        sendMsg(msg.chatId, "⛔ Accesso negato.");
        return;
    }

    // Copia dei dati essenziali (estrazione dal messaggio originale)
    m_pending.chatId = msg.chatId;
    m_pending.sender_username = msg.sender.username.c_str();
    m_pending.sender_firstName = msg.sender.firstName.c_str();
    m_pending.sender_lastName = msg.sender.lastName.c_str();
    parseCommand(msg.text.c_str(), m_pending.command, m_pending.payload);

    m_pending.exists = true;
    lnLOG_DEBUG("%s Task stored: %s", tgLogPrefix, m_pending.command);
}

*/
void lnTelegram::handleIncomingMessage(TBMessage &msg) {
    if (m_pending.exists) {
        lnLOG_WARNING("%s Buffer busy, skipping message.", tgLogPrefix);
        return;
    }

    if (!isAuthorized(msg.chatId)) {
        sendMsg(msg.chatId, "⛔ Accesso negato.");
        return;
    }

    // Reset della struttura (opzionale ma pulito)
    m_pending.chatId = msg.chatId;

    // COPIA SICURA DELLE STRINGHE
    // snprintf: copia, taglia se necessario e aggiunge SEMPRE il '\0'
    snprintf(m_pending.sender_username, MAX_NAME_LEN, "%s", msg.sender.username.c_str());
    snprintf(m_pending.sender_firstName, MAX_NAME_LEN, "%s", msg.sender.firstName.c_str());
    snprintf(m_pending.sender_lastName, MAX_NAME_LEN, "%s", msg.sender.lastName.c_str());

    // Parsing del comando
    parseCommand(msg.text.c_str(), m_pending.command, m_pending.payload);

    m_pending.exists = true;
    lnLOG_DEBUG("%s Task stored from %s: %s", tgLogPrefix, m_pending.sender_username, m_pending.command);
}


// ##########################################################
// #
// ##########################################################
bool lnTelegram::sendMsg(int64_t chat_id, const char* text) {
    lnLOG_DEBUG("%s net %d - isActive: %d", tgLogPrefix,  m_isNetworkAvailable, m_isActive);
    lnLOG_INFO("%s chatId %lld, text: %s", tgLogPrefix,  chat_id, text);

    if (!m_isActive && m_isNetworkAvailable) { // tentiamo la strada HTTPS
        lnLOG_INFO("%s sending message via HTTP", tgLogPrefix);
        return this->sendHTTP(chat_id, text);
    }
    lnLOG_INFO("%s sending message via BOT", tgLogPrefix);
    return m_bot.sendTo(chat_id, text);
}
