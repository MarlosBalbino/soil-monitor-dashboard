#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/net/socket.h> /* Fornece as estruturas zsock_ e AF_INET */

#include "wifi.h"

/* =========================================
 * NOVO: Estrutura para unificar os sensores
 * ========================================= */
struct pots_data {
    uint16_t pot1;
    uint16_t pot2;
};

/* =========================================
 * DEFINIÇÃO DO ZBUS (Atualizado para struct)
 * ========================================= */
/* 1. Altera o tipo do canal de uint16_t para struct pots_data */
ZBUS_CHAN_DEFINE(adc_data_chan, struct pots_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(.pot1 = 0, .pot2 = 0));

/* 2. Define o Assinante (Subscriber) com um tamanho de fila */
ZBUS_SUBSCRIBER_DEFINE(http_subscriber, 4);

/* 3. Conecta o Assinante ao Canal estaticamente */
ZBUS_CHAN_ADD_OBS(adc_data_chan, http_subscriber, 3);

/* =========================================
 * CONFIGURAÇÕES DO SERVIDOR HTTP
 * ========================================= */
#define HTTP_SERVER_IP "192.168.0.5"
#define HTTP_SERVER_PORT 8080

/* =========================================
 * THREAD DO LED (Mantida estável)
 * ========================================= */
#define SLEEP_TIME_ON_MS   5000
#define SLEEP_TIME_OFF_MS  25000
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void led_thread_function(void) {
    if (!gpio_is_ready_dt(&led)) return;
    
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    while (1) {
        gpio_pin_set_dt(&led, 1);
        k_msleep(SLEEP_TIME_ON_MS);
        gpio_pin_set_dt(&led, 0);
        k_msleep(SLEEP_TIME_OFF_MS);
    }
}
K_THREAD_DEFINE(led_thread_id, 512, led_thread_function, NULL, NULL, NULL, 7, 0, 0);


/* =========================================
 * THREAD DO HTTP (Modificada para 2 sensores)
 * ========================================= */
static void send_http_post(struct pots_data data) {
    int sock;
    struct sockaddr_in server_addr;
    char payload[128]; /* Aumentado para suportar o JSON maior */
    char request[320];

    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(HTTP_SERVER_PORT);
    zsock_inet_pton(AF_INET, HTTP_SERVER_IP, &server_addr.sin_addr);

    if (zsock_connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printk("[HTTP] Falha ao conectar no servidor.\n");
        zsock_close(sock);
        return;
    }

    /* MODIFICAÇÃO: Payload agora envia duas chaves no JSON */
    snprintf(payload, sizeof(payload), "{\"potenciometro1\": %d, \"potenciometro2\": %d}", data.pot1, data.pot2);
    
    snprintf(request, sizeof(request),
             "POST /api/dados HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s", HTTP_SERVER_IP, strlen(payload), payload);

    zsock_send(sock, request, strlen(request), 0);
    printk("[HTTP] DADOS ENVIADOS -> Pot1: %d | Pot2: %d\n", data.pot1, data.pot2);
    
    zsock_close(sock);
}

void http_client_thread(void) {
    const struct zbus_channel *chan;
    struct pots_data received_data; /* Tipo atualizado */
    
    while (1) {
        if (zbus_sub_wait(&http_subscriber, &chan, K_FOREVER) == 0) {
            if (chan == &adc_data_chan) {
                zbus_chan_read(chan, &received_data, K_NO_WAIT);
                
                if (wifi_is_ready()) {
                    send_http_post(received_data);
                } else {
                    printk("[HTTP] Rede não pronta. Ignorando envio.\n");
                }
            }
        }
    }
}
K_THREAD_DEFINE(http_thread_id, 2048, http_client_thread, NULL, NULL, NULL, 5, 0, 0);


/* =========================================
 * THREAD PRINCIPAL (Modificada para ler index 0 e 1)
 * ========================================= */
/* Obtém as especificações usando as macros indexadas do Devicetree (_BY_IDX) */
static const struct adc_dt_spec pot1 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct adc_dt_spec pot2 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

int main(void) {
    int err1, err2;
    uint16_t buf_pot1;
    uint16_t buf_pot2;
    struct pots_data msg;

    wifi_init();

    /* Configura sequências independentes para os buffers locais */
    struct adc_sequence seq1 = {
        .buffer = &buf_pot1,
        .buffer_size = sizeof(buf_pot1),
    };
    struct adc_sequence seq2 = {
        .buffer = &buf_pot2,
        .buffer_size = sizeof(buf_pot2),
    };

    /* Inicializa e valida o Potenciômetro 1 */
    if (!device_is_ready(pot1.dev) || adc_channel_setup_dt(&pot1) < 0 || adc_sequence_init_dt(&pot1, &seq1) < 0) {
        printk("Erro ao iniciar ADC no Potenciometro 1\n");
        return 0;
    }

    /* Inicializa e valida o Potenciômetro 2 */
    if (!device_is_ready(pot2.dev) || adc_channel_setup_dt(&pot2) < 0 || adc_sequence_init_dt(&pot2, &seq2) < 0) {
        printk("Erro ao iniciar ADC no Potenciometro 2\n");
        return 0;
    }

    while (1) {
        /* Realiza as duas leituras consecutivas */
        err1 = adc_read(pot1.dev, &seq1);
        err2 = adc_read(pot2.dev, &seq2);
        
        if (err1 == 0 && err2 == 0) {
            msg.pot1 = buf_pot1;
            msg.pot2 = buf_pot2;
            
            printk("[ADC] Lido Pot1: %d e Pot2: %d | Publicando struct no ZBUS...\n", msg.pot1, msg.pot2);
            zbus_chan_pub(&adc_data_chan, &msg, K_NO_WAIT);
        } else {
            printk("[ADC] Falha ao ler um dos sensores analógicos.\n");
        }
        
        k_msleep(10000); 
    }

    return 0;
}