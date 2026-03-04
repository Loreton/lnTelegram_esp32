/*
// updated by ...: Loreto Notarantonio
// Date .........: 21-02-2026 15.10.55
*/

#pragma once
    #include <WiFi.h>
    #include <time.h>

    bool initNetwork(const char* ssid,
                     const char* password,
                     uint32_t timeoutMs = 15000);

    void maintainNetwork();

    bool isTimeValid();