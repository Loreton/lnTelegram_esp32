/*
// updated by ...: Loreto Notarantonio
// Date .........: 21-02-2026 15.10.55
*/
#include "NetworkUtils.h"

static unsigned long lastReconnectAttempt = 0;

bool initNetwork(const char* ssid,
                 const char* password,
                 uint32_t timeoutMs)
{
    Serial.println("\n=== INIT NETWORK ===");

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    WiFi.begin(ssid, password);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis() - start > timeoutMs) {
            Serial.println("\nWiFi timeout!");
            return false;
        }
    }

    Serial.println("\nWiFi connesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());

    // ---- NTP ----
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Sincronizzazione NTP");

    time_t now = time(nullptr);
    while (now < 100000) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }

    Serial.println("\nOra sincronizzata!");

    return true;
}

void maintainNetwork()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    if (millis() - lastReconnectAttempt < 5000)
        return;

    lastReconnectAttempt = millis();

    Serial.println("WiFi perso. Tentativo riconnessione...");
    WiFi.reconnect();
}

bool isTimeValid()
{
    return (time(nullptr) > 100000);
}


void checkInternet() {
    IPAddress ip;
    if (WiFi.hostByName("api.telegram.org", ip)) {
        Serial.println(ip);
    } else {
        Serial.println("DNS FAIL");
    }
}