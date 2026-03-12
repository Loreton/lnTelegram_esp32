//
// updated by ...: Loreto Notarantonio
// Date .........: 12-09-2025 08.29.52
//
#include "lnTelegram.h"
#include <lnLogger_Class.h>
#include <stdarg.h>

const char* tgLogPrefix = "TG:";

lnTelegram::lnTelegram() : m_bot(m_client) {}

// ##########################################################
// #
// ##########################################################
void lnTelegram::begin(const char* token) {
    m_client.setInsecure();
    m_client.setTimeout(10); // Aumentiamo il timeout del socket
    m_bot.setUpdateTime(2000);
    m_bot.setTelegramToken(token);
    lnLOG_INFO("%s Module initialized.", tgLogPrefix);
}

void lnTelegram::setAuthorizedIDs(const int64_t* ids, size_t count) {
    m_authorizedIDs = ids;
    m_authCount = count;
}

void lnTelegram::setValidCommands(const char** cmds, size_t count) {
    m_validCommands = cmds;
    m_validCmdsCount = count;
}

bool lnTelegram::isAuthorized(int64_t id) {
    for (size_t i = 0; i < m_authCount; i++) {
        if (m_authorizedIDs[i] == id) return true;
    }
    return false;
}

bool lnTelegram::isValidCommand(const char* cmd) {
    if (m_validCmdsCount == 0) return true; // Se non definiti, accetta tutto
    for (size_t i = 0; i < m_validCmdsCount; i++) {
        if (strcasecmp_P(cmd, m_validCommands[i]) == 0) return true;
    }
    return false;
}

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

void lnTelegram::handleIncomingMessage(TBMessage &msg) {
    int64_t chat_id = msg.chatId;
    lnLOG_WARNING("%s received message: %s", tgLogPrefix, msg.text.c_str());

    if (!isAuthorized(chat_id)) {
        lnLOG_WARNING("%s Unauthorized from ID: %lld", tgLogPrefix, chat_id);
        sendMessage(chat_id, "⛔ Accesso negato. ID %lld non autorizzato.", chat_id);
        return;
    }

    char command[MAX_CMD_LEN];
    char payload[MAX_PAYLOAD_LEN];
    parseCommand(msg.text.c_str(), command, payload);

    if (strcasecmp_P(command, "/whoami") == 0) {
        lnLOG_DEBUG("%s rocessing command: %s", tgLogPrefix, command);
        sendMessage(chat_id, "👤 User: %s\n🆔 ID: %lld", msg.sender.username, chat_id);
        return;
    }

    if (!isValidCommand(command)) {
        lnLOG_ERROR("%s comando errato: %s", tgLogPrefix, command);
        sendMessage(chat_id, "❓ Comando sconosciuto: <code>%s</code>", command);
        return;
    }

    if (_callback)
        _callback(msg, command, payload);


}


bool lnTelegram::sendMessage(int64_t id, const char* format, ...) {
    if (!m_isActive) return false;

    char buffer[384];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    lnLOG_DEBUG("%s Sending to %lld: [%s]", tgLogPrefix, id, buffer);

    // TRUCCO: Convertiamo esplicitamente in String per la libreria Async
    // Questo assicura che la libreria gestisca correttamente il buffer
    bool success = m_bot.sendTo(id, String(buffer), "HTML");

    if (!success) {
        lnLOG_ERROR("%s Send failed! Check connection or HTML tags.", tgLogPrefix);
        // Non resettiamo m_isActive qui, lasciamo che lo faccia update se serve
    } else {
        lnLOG_SUCCESS("%s Message sent.", tgLogPrefix);
    }
    return success;
}


bool lnTelegram::reply(TBMessage &msg, const char* format, ...) {
    // Usiamo msg.chatId che è più affidabile di sender.id per le risposte
    // if (!m_isActive) return false;
    char buffer[384];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return sendMessage(msg.chatId, buffer);
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
            lnLOG_SUCCESS("%s Bot Ready.", tgLogPrefix);
        }
        return;
    }

    if (!isTimeValid) return;

    if (!m_firstConnectDone) {
        static uint32_t bootDelay = millis();
        if (millis() - bootDelay > 5000) {
            // Test di invio senza tag HTML complessi per la prima volta
            if (broadcast("🚀 <b>Sistema Online!</b>")) {
                m_firstConnectDone = true;
            } else {
                // Se fallisce, riproverà al prossimo ciclo di update
                bootDelay = millis();
            }
        }
        return;
    }

    TBMessage msg;
    if (m_bot.getNewMessage(msg)) handleIncomingMessage(msg);
}



// void lnTelegram::broadcast(const char* format, ...) {
//     if (!m_isActive || m_authCount == 0) return;
//     char buffer[256];
//     va_list args;
//     va_start(args, format);
//     vsnprintf(buffer, sizeof(buffer), format, args);
//     va_end(args);
//     for (size_t i = 0; i < m_authCount; i++) {
//         m_bot.sendTo(m_authorizedIDs[i], buffer, "HTML");
//     }
// }

bool lnTelegram::broadcast(const char* format, ...) {
    if (!m_isActive || m_authCount == 0) return false;

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (size_t i = 0; i < m_authCount; i++) {
        // Usa la TUA sendMessage per avere i log e la gestione errori
        this->sendMessage(m_authorizedIDs[i], buffer);
    }
    return true;
}

