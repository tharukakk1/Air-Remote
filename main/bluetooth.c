#include "bluetooth.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

/* NimBLE Includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "BLUETOOTH";
static uint8_t bluetooth_addr_type;

// Forward declaration
static void bluetooth_advertise(void);

// GAP Event Handler (Handles connection & pairing events)
static int bluetooth_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Device connected! Status: %d", event->connect.status);
            if (event->connect.status != 0) {
                // Connection failed, resume advertising
                bluetooth_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Device disconnected! Reason: %d", event->disconnect.reason);
            // Device disconnected, start advertising again so it's discoverable
            bluetooth_advertise();
            break;

        default:
            break;
    }
    return 0;
}

// Setup and start BLE Advertising
static void bluetooth_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));

    // General Discoverable Mode & BLE Only
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Include TX Power Level
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    // Device Name
    const char *name = "ESP32-C3 Mouse";
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    // KEY PART: Set Appearance to HID Mouse (0x03C2)
    // This tells your phone/PC to display a mouse icon
    fields.appearance = 0x03C2; 
    fields.appearance_is_present = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement fields; rc=%d", rc);
        return;
    }

    // Begin advertising
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Undirected connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // General discoverable

    rc = ble_gap_adv_start(bluetooth_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, bluetooth_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error enabling advertisement; rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "Advertising started! Search for 'ESP32-C3 Mouse' on your host device.");
}

// Host task required by NimBLE
static void bluetooth_host_task(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run(); // This blocks until nimble_port_stop() is called
    nimble_port_freertos_deinit();
}

// NimBLE Stack Sync Callback
static void bluetooth_on_sync(void) {
    int rc = ble_hs_id_infer_auto(0, &bluetooth_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error determining address type; rc=%d", rc);
        return;
    }
    bluetooth_advertise();
}

void bluetooth_init(void) {
    // 1. Initialize NVS (Required for storing Bluetooth pairing keys)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize NimBLE Stack
    ESP_ERROR_CHECK(nimble_port_init());
    
    // Set device name inside GAP service
    ble_svc_gap_device_name_set("ESP32-C3 Mouse");

    // Configure sync callback
    ble_hs_cfg.sync_cb = bluetooth_on_sync;

    // 3. Start the FreeRTOS task to run NimBLE
    xTaskCreate(bluetooth_host_task, "bluetooth_host_task", 4096, NULL, 5, NULL);
}