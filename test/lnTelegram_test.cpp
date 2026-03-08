//
// updated by ...: Loreto Notarantonio
// Date .........: 04-03-2026 10.38.50
//

#include "lnTelegram.h"

TelegramModule telegram;

/* ID in flash */
const char id1[] PROGMEM = "123456789";
const char id2[] PROGMEM = "987654321";

const char* const allowedIDs[] PROGMEM = {
    id1,
    id2
};

/* Comandi in flash */
const char cmd1[] PROGMEM = "/start";
const char cmd2[] PROGMEM = "/stop";
const char cmd3[] PROGMEM = "/status";

const char* const allowedCommands[] PROGMEM = {
    cmd1,
    cmd2,
    cmd3
};

void myCallback(const char* chat_id, const char* cmd, const char* payload) {
    if (strcmp(cmd, "/start") == 0) {
        telegram.sendMsg(chat_id, "Start OK");
    }
    else if (strcmp(cmd, "/stop") == 0) {
        telegram.sendMsg(chat_id, "Stop OK");
    }
    else if (strcmp(cmd, "/status") == 0) {
        telegram.sendMsg(chat_id, "Sistema OK");
    }
}

void setup() {
    Serial.begin(115200);

    telegram.init("BOT_TOKEN",
                  allowedIDs,
                  2,
                  allowedCommands,
                  3,
                  myCallback);
}

void loop() {
    telegram.loop();
}