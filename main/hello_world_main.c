#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* =========================================================
 * FREERTOS
 * ========================================================= */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* =========================================================
 * ESP-IDF
 * ========================================================= */
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

/* =========================================================
 * TAG
 * ========================================================= */
static const char *TAG = "SNIFFER";

/* =========================================================
 * UART CONFIG
 * ========================================================= */
#define UART_PORT           UART_NUM_2
#define UART_TX_PIN         17
#define UART_RX_PIN         16
#define UART_BUFFER_SIZE    2048

/* =========================================================
 * WIFI CHANNEL CONFIG
 * ========================================================= */
#define WIFI_CHANNEL_MIN    1
#define WIFI_CHANNEL_MAX    13
#define CHANNEL_HOP_DELAY   1000

/* =========================================================
 * JSON BUFFER
 * ========================================================= */
#define JSON_MESSAGE_SIZE   512

static uint32_t total_pacotes = 0;
static uint32_t total_uart = 0;
static uint32_t json_truncado = 0;

/* =========================================================
 * IEEE 802.11 HEADER
 * ========================================================= */
typedef struct
{
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t  addr1[6];
    uint8_t  addr2[6];
    uint8_t  addr3[6];
    uint16_t seq_ctrl;

} wifi_ieee80211_hdr_t;

/* =========================================================
 * MAC -> STRING
 * ========================================================= */
static void mac_to_str(const uint8_t *mac,char *dest)
{
    snprintf(dest,18,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

/* =========================================================
 * VALIDATE MAC
 * ========================================================= */
static bool is_invalid_mac(const char *mac)
{
    return (mac == NULL ||strcmp(mac, "FF:FF:FF:FF:FF:FF") == 0 ||strcmp(mac, "00:00:00:00:00:00") == 0 ||strncmp(mac, "01:00:5E", 8) == 0 ||strncmp(mac, "33:33", 5) == 0);
}

/* =========================================================
 * UART INIT
 * ========================================================= */
static void uart_init_custom(void)
{
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT,UART_BUFFER_SIZE,0,0,NULL,0));

    ESP_ERROR_CHECK(uart_param_config(UART_PORT,&uart_config));

    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,UART_TX_PIN,UART_RX_PIN,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
}

/* =========================================================
 * WIFI SNIFFER CALLBACK
 * ========================================================= */
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_DATA)
    {
        return;
    }

    int64_t inicio_total = esp_timer_get_time();

    total_pacotes++;

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *) buf;

    wifi_ieee80211_hdr_t *hdr = (wifi_ieee80211_hdr_t *) pkt->payload;

    uint16_t frame_control = hdr->frame_ctrl;

    uint8_t to_ds   = (frame_control & 0x0100) >> 8;
    uint8_t from_ds = (frame_control & 0x0200) >> 9;

    /* ignora tráfego inválido */
    if ((to_ds == 0 && from_ds == 0) ||
        (to_ds == 1 && from_ds == 1))
    {
        return;
    }

    char client_mac[18];
    char bssid_mac[18];

    /* cliente -> AP */
    if (to_ds == 1 && from_ds == 0)
    {
        mac_to_str(hdr->addr2, client_mac);
        mac_to_str(hdr->addr1, bssid_mac);
    }
    /* AP -> cliente */
    else
    {
        mac_to_str(hdr->addr1, client_mac);
        mac_to_str(hdr->addr2, bssid_mac);
    }

    if (is_invalid_mac(client_mac) || is_invalid_mac(bssid_mac))
    {
        return;
    }

    const char *direction =
        (to_ds == 1 && from_ds == 0) ?
        "CLIENTE_AP" : "AP_CLIENTE";

    char json_message[JSON_MESSAGE_SIZE];

    int64_t inicio_json = esp_timer_get_time();

    int json_size = snprintf(
        json_message,
        sizeof(json_message),

        "{"
        "\"type\":%d,"
        "\"subtype\":%d,"
        "\"rssi\":%d,"
        "\"canal\":%d,"
        "\"tamanho\":%d,"
        "\"to_ds\":%d,"
        "\"from_ds\":%d,"
        "\"direcao\":\"%s\","
        "\"cliente\":\"%s\","
        "\"bssid\":\"%s\","
        "\"timestamp\":%lu"
        "}\n",

        type,
        (frame_control & 0x00F0) >> 4,
        pkt->rx_ctrl.rssi,
        pkt->rx_ctrl.channel,
        pkt->rx_ctrl.sig_len,
        to_ds,
        from_ds,
        direction,
        client_mac,
        bssid_mac,
        (unsigned long)xTaskGetTickCount());

    int64_t fim_json = esp_timer_get_time();

    if (json_size <= 0 || json_size >= sizeof(json_message))
    {
        json_truncado++;

        ESP_LOGW(TAG,
                 "JSON truncado (%lu)",
                 json_truncado);

        return;
    }

    printf("%s", json_message);

    int64_t inicio_uart = esp_timer_get_time();

    uart_write_bytes(
        UART_PORT,
        json_message,
        json_size);

    int64_t fim_uart = esp_timer_get_time();

    total_uart++;

    int64_t fim_total = esp_timer_get_time();

    ESP_LOGI(TAG,"====================================================");
    ESP_LOGI(TAG,"PACOTE #%lu", total_pacotes);
    ESP_LOGI(TAG,"Cliente            : %s", client_mac);
    ESP_LOGI(TAG,"BSSID              : %s", bssid_mac);
    ESP_LOGI(TAG,"RSSI               : %d dBm",pkt->rx_ctrl.rssi);
    ESP_LOGI(TAG,"Canal              : %d",pkt->rx_ctrl.channel);
    ESP_LOGI(TAG,"Tamanho            : %d bytes",pkt->rx_ctrl.sig_len);
    ESP_LOGI(TAG,"Direção            : %s",direction);
    ESP_LOGI(TAG,"Tempo JSON         : %lld us",fim_json - inicio_json);
    ESP_LOGI(TAG,"Tempo UART         : %lld us",fim_uart - inicio_uart);
    ESP_LOGI(TAG,"Tempo total        : %lld us",fim_total - inicio_total);
    ESP_LOGI(TAG,"Pacotes capturados : %lu",total_pacotes);
    ESP_LOGI(TAG,"UART enviados      : %lu",total_uart);
    ESP_LOGI(TAG,"JSON truncados     : %lu",json_truncado);
    ESP_LOGI(TAG,"Heap Livre         : %u bytes",heap_caps_get_free_size(MALLOC_CAP_8BIT));
    ESP_LOGI(TAG,"====================================================");
}

/* =========================================================
 * CHANNEL HOPPING TASK
 * ========================================================= */
static void channel_hopping_task(void *pvParameter)
{
    uint8_t channel = WIFI_CHANNEL_MIN;

    while (1)
    {
        esp_wifi_set_channel(channel,WIFI_SECOND_CHAN_NONE);

        channel++;

        if (channel > WIFI_CHANNEL_MAX)
        {
            channel = WIFI_CHANNEL_MIN;
        }

        vTaskDelay(pdMS_TO_TICKS(CHANNEL_HOP_DELAY));
    }
}

/* =========================================================
 * WIFI INIT
 * ========================================================= */
static void wifi_init_sniffer(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK( esp_netif_init());

    ESP_ERROR_CHECK( esp_event_loop_create_default());

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler));
}

/* =========================================================
 * APP MAIN
 * ========================================================= */
void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando sniffer");

    uart_init_custom();

    wifi_init_sniffer();

    xTaskCreate(channel_hopping_task,"channel_hopping_task",2048,NULL,5,NULL);
}