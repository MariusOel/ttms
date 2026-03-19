#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the OTA Access Point and Webserver.
 *
 * This will:
 * 1. Initialize NVS (if not already done).
 * 2. Set up a Wi-Fi Access Point (SSID: TTMS-OTA, Pass: password123).
 * 3. Start an HTTP server on port 80.
 * 4. Serve an upload form at "/" and handle firmware updates at "/update".
 */
void ota_manager_start(void);

#ifdef __cplusplus
}
#endif
