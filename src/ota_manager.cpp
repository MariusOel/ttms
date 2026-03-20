#include "ota_manager.hpp"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "OTA_MGR";

/* Max size of the buffer for receiving OTA data */
#define OTA_BUF_SIZE 1024

/* Simple HTML form for uploading the firmware */
static const char *upload_html =
    "<!DOCTYPE html><html><head><title>TTMS OTA Update</title>"
    "<style>body { font-family: sans-serif; display: flex; justify-content: "
    "center; align-items: center; height: 100vh; margin: 0; background-color: "
    "#f0f2f5; }"
    ".container { background: white; padding: 2rem; border-radius: 8px; "
    "box-shadow: 0 4px 6px rgba(0,0,0,0.1); text-align: center; }"
    "h2 { color: #1a73e8; } input[type='file'] { margin: 1rem 0; } "
    "button { background: #1a73e8; color: white; border: none; padding: 0.5rem "
    "1rem; border-radius: 4px; cursor: pointer; }"
    "button:hover { background: #1557b0; }</style></head>"
    "<body><div class='container'><h2>TTMS Firmware Update</h2>"
    "<form method='POST' action='/update' enctype='multipart/form-data'>"
    "<input type='file' name='update' required><br>"
    "<button type='submit'>Update Firmware</button></form></div></body></html>";

/* Handler for the root page */
static esp_err_t root_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET / - Serving upload page");
  httpd_resp_send(req, upload_html, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

/* Handler for the firmware update (POST) */
static esp_err_t update_post_handler(httpd_req_t *req) {
  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition = NULL;
  bool binary_started = false;

  ESP_LOGI(TAG, "Starting OTA update...");

  update_partition = esp_ota_get_next_update_partition(NULL);
  if (update_partition == NULL) {
    ESP_LOGE(TAG, "Passive OTA partition not found!");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  esp_err_t err =
      esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  char *buf = (char *)malloc(OTA_BUF_SIZE);
  int remaining = req->content_len;
  int total_size = remaining;
  int written_size = 0;

  ESP_LOGI(TAG, "Receiving %d bytes (including multipart headers)...",
           total_size);

  while (remaining > 0) {
    int recv_len = httpd_req_recv(
        req, buf, (remaining < OTA_BUF_SIZE) ? remaining : OTA_BUF_SIZE);
    if (recv_len <= 0) {
      if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      ESP_LOGE(TAG, "OTA receive failed");
      break;
    }
    remaining -= recv_len;

    char *data_ptr = buf;
    int data_len = recv_len;

    // Skip multipart headers (search for 0xE9 magic byte)
    if (!binary_started) {
      for (int i = 0; i < recv_len; i++) {
        if ((uint8_t)buf[i] == 0xE9) {
          data_ptr = &buf[i];
          data_len = recv_len - i;
          binary_started = true;
          ESP_LOGI(
              TAG,
              "Found binary magic byte 0xE9 at offset %d. Starting flash...",
              i);
          break;
        }
      }
      if (!binary_started)
        continue; // Still searching in this chunk
    }

    err = esp_ota_write(update_handle, data_ptr, data_len);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
      break;
    }
    written_size += data_len;

    // Progress logging
    static int last_log_pct = -1;
    int pct = written_size * 100 / (total_size > 0 ? total_size : 1);
    if (pct % 10 == 0 && pct != last_log_pct) {
      ESP_LOGI(TAG, "Flash Progress: %d%% (%d bytes written)", pct,
               written_size);
      last_log_pct = pct;
    }

    // Yield to prevent Task Watchdog trigger during long flash writes
    vTaskDelay(1);
  }

  free(buf);

  if (binary_started && err == ESP_OK) {
    err = esp_ota_end(update_handle);
    if (err == ESP_OK) {
      err = esp_ota_set_boot_partition(update_partition);
      if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful! Rebooting...");
        httpd_resp_sendstr(req, "Update successful. Rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
        return ESP_OK; // Should not reach here
      } else {
        ESP_LOGE(TAG, "Failed to set boot partition (%s)",
                 esp_err_to_name(err));
      }
    } else {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
    }
  }

  ESP_LOGE(TAG, "OTA failed or interrupted");
  esp_ota_abort(update_handle);
  httpd_resp_send_500(req);
  return ESP_FAIL;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "Station joined, AID=%d", event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "Station left, AID=%d", event->aid);
  }
}

void ota_manager_start(void) {
  // 1. Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // 2. Wi-Fi AP Configuration
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {};
  strcpy((char *)wifi_config.ap.ssid, "TTMS-OTA");
  strcpy((char *)wifi_config.ap.password, "password123");
  wifi_config.ap.ssid_len = strlen("TTMS-OTA");
  wifi_config.ap.channel = 1;
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.ap.max_connection = 4;
  wifi_config.ap.beacon_interval = 100;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Wi-Fi AP started. SSID: %s", wifi_config.ap.ssid);

  // 3. HTTP Server
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;
  config.stack_size = 8192; // Increase stack for OTA

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t root = {.uri = "/",
                        .method = HTTP_GET,
                        .handler = root_get_handler,
                        .user_ctx = NULL};
    httpd_register_uri_handler(server, &root);

    httpd_uri_t update = {.uri = "/update",
                          .method = HTTP_POST,
                          .handler = update_post_handler,
                          .user_ctx = NULL};
    httpd_register_uri_handler(server, &update);

    ESP_LOGI(TAG, "HTTP server started on port 80");
  } else {
    ESP_LOGE(TAG, "Failed to start HTTP server");
  }
}
