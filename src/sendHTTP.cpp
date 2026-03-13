//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//

#ifdef  __ln_MAIN_TEST_MODULE__
#include <HTTPClient.h>
#include "lnTelegram.h"
#include <lnLogger_Class.h>


const char* tghLogPrefix = "HTTP:";

HTTPClient http;

// #define MAX_MESSAGE_SIZE 512
#define MAX_TELEGRAM_MESSAGE_SIZE   300
#define MAX_TELEGRAM_ENCODED_SIZE   500
#define MAX_TELEGRAM_FULL_MSG_SIZE  600

typedef struct {
    uint16_t pos = 0; // posizione da scrivere nel msg[]
    char msg[MAX_TELEGRAM_MESSAGE_SIZE + 1];
    char encoded[MAX_TELEGRAM_ENCODED_SIZE + 1];
    char fullMsg[MAX_TELEGRAM_FULL_MSG_SIZE + 1];
} telegramBuffers_t;

telegramBuffers_t tgMessage; // crea un'instanza di struct ed un pointer
telegramBuffers_t *tg = &tgMessage; // crea un'instanza di struct ed un pointer

// #######################################################################
// # prima dell'invio passa il messaggio alla urlencode per convertice caratteri speciali
// # comunque ripulisce il messaggio     tg->msg[0] = '\0';
// #######################################################################
bool lnTelegram::sendHTTP(int64_t chat_id, const char* msg) {
    bool fStatus=false;

    // --- HTTPClient http;
    const char* parseMode="HTML";

    // --- encode message
    urlEncode(msg, tg->encoded);

    // --- Costruisce l'URL completo con tutti i parametri
    const char *urlFormat = "https://api.telegram.org/bot%s/sendMessage?chat_id=%lld&parse_mode=%s&text=%s";
    // --- snprintf() scrive al massimo n-1 caratteri più il terminatore nul (\0) in dest.
    snprintf(tg->fullMsg, sizeof(tg->fullMsg), urlFormat, m_token, chat_id, parseMode, tg->encoded);

    lnLOG_DEBUG("%sSending msg: [%ld]: %s", tghLogPrefix, strlen(tg->fullMsg), tg->fullMsg);
    http.begin(tg->fullMsg);
    int httpResponseCode = http.GET();
    const char* statusMsg;
    http.end();
    tg->msg[0] = '\0'; // clear message

    lnLOG_DEBUG("%s [http code: %d]", tghLogPrefix, httpResponseCode);

    switch (httpResponseCode) {
        // Informational responses (100 – 199)
        // Successful responses (200 – 299)
        // Redirection messages (300 – 399)
        // Client error responses (400 – 499)
        // Server error responses (500 – 599)

        case 200 ...299:
            lnLOG_INFO("%s [%d] - Send OK", tghLogPrefix, httpResponseCode);
            fStatus=true;
            statusMsg = nullptr;
            break;

        case 300 ...399:
            // lnLOG_INFO("[%d] - Redirect....????", httpResponseCode);
            statusMsg = "Redirect....????";
            fStatus=true;
            break;

        case 400 ...499:
            // lnLOG_ERROR("[%d] - Bad Request! Client Error URL: %s (size: %ld)", httpResponseCode, tg->fullMsg, strlen(tg->fullMsg));
            statusMsg = "Bad Request! Client Error";
            break;

        case 500 ...599:
            // lnLOG_ERROR("[%d] - Bad Request! Server Error URL: %s (size: %ld)", httpResponseCode, tg->fullMsg, strlen(tg->fullMsg));
            statusMsg = "Bad Request! Server Error";
            break;

        default:
            // lnLOG_ERROR("[%d] - Send ERROR! URL: %s (size: %ld)", httpResponseCode, tg->fullMsg, strlen(tg->fullMsg));
            statusMsg = "Send ERROR!";
            break;
    }

    if (statusMsg) {
        lnLOG_ERROR("%s sendHTTP: rcode: %d - msg: %s URL: %s (size: %ld)", tghLogPrefix, httpResponseCode, statusMsg, tg->fullMsg, strlen(tg->fullMsg));
    }



    return fStatus;
}



// | Carattere                   | Escape C/C++ | Decimale | Esadecimale | URL encoding                                                     |
// | --------------------------- | ------------ | -------- | ----------- | ---------------------------------------------------------------- |
// | **NUL**                     | `\0`         | 0        | 00          | `%00`                                                            |
// | **TAB**                     | `\t`         | 9        | 09          | `%09`                                                            |
// | **LF** (line feed, newline) | `\n`         | 10       | 0A          | `%0A`                                                            |
// | **CR** (carriage return)    | `\r`         | 13       | 0D          | `%0D`                                                            |
// | **SPACE**                   | `' '`        | 32       | 20          | `%20` *(oppure `+` solo in `application/x-www-form-urlencoded`)* |
// | **"** (doppio apice)        | `\"`         | 34       | 22          | `%22`                                                            |
// | **'** (apice singolo)       | `'`          | 39       | 27          | `%27`                                                            |
// | **/** (slash)               | `/`          | 47       | 2F          | `%2F`                                                            |
// | **\\** (backslash)          | `\\`         | 92       | 5C          | `%5C`                                                            |


uint16_t lnTelegram::urlEncode(const char* src, char* dest) {
    uint16_t urlEncode_len;
    const char *p = src;
    char *q = dest;
    while (*p) {
        if (isalnum((unsigned char)*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~') {
            *q++ = *p;
        } else if (*p == ' ') {
            *q++ = '+';
        } else if (*p == '\n') { // <--- Aggiungi questa condizione
            *q++ = '%';
            *q++ = '0';
            *q++ = 'A';
        } else if (*p == '\t') { // <--- Aggiungi questa condizione
            *q++ = '%';
            *q++ = '0';
            *q++ = '9';
        } else {
            // Codifica i caratteri speciali in formato esadecimale %XX
            sprintf(q, "%%%02X", (unsigned char)*p);
            q += 3;
        }
        p++;
        urlEncode_len = q-dest;
        if (urlEncode_len >= MAX_TELEGRAM_ENCODED_SIZE) {
            lnLOG_ERROR("%s urlEncode [len: %ld] is greather than MAX_TELEGRAM_ENCODED_SIZE (%ld)", tghLogPrefix, urlEncode_len, MAX_TELEGRAM_ENCODED_SIZE);
            break;
        }
    }
    *q = '\0';
    lnLOG_DEBUG("%s urlEncode [len: %ld] - %s", tghLogPrefix, urlEncode_len, dest);
    // Serial.print("urlEncode [len: ");
    // Serial.print(urlEncode_len);
    // Serial.print(" - ");
    // Serial.println(dest);
    return urlEncode_len;
}
#endif