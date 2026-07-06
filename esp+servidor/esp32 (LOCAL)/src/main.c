#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/net/socket.h> /* Fornece as estruturas zsock_ e AF_INET */

#include "wifi.h"

/* ====================================================================
 * NOVO: Variável global para armazenar o estado da irrigação
 * ==================================================================== */
static volatile bool esta_irrigando = false;


/* NOVO: Guarda a última leitura do Pot1. Inicializado em 2000 (Úmido) 
   para segurança, evitando ligar o LED antes da primeira leitura do ADC */
   
static volatile uint16_t ultima_umidade = 2000; 


/* =========================================
 * MODIFICAÇÃO: Estrutura agora leva o estado
 * ========================================= */
struct pots_data {
    uint16_t pot1;
    uint16_t pot2;
    bool irrigando; /* NOVO CAMPO */
};

/* =========================================
 * DEFINIÇÃO DO ZBUS
 * ========================================= */
/* Inicializa o canal limpando também o novo campo booleano */
ZBUS_CHAN_DEFINE(adc_data_chan, struct pots_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(.pot1 = 0, .pot2 = 0, .irrigando = false));

ZBUS_SUBSCRIBER_DEFINE(http_subscriber, 4);
ZBUS_CHAN_ADD_OBS(adc_data_chan, http_subscriber, 3);

/* =========================================
 * CONFIGURAÇÕES DO SERVIDOR HTTP
 * ========================================= */
#define HTTP_SERVER_IP "192.168.0.5"
#define HTTP_SERVER_PORT 8080

/* =========================================
 * THREAD DO LED (Atualiza a variável global)
 * ========================================= */
#define SLEEP_TIME_ON_MS   10000
#define SLEEP_TIME_OFF_MS  10000
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void led_thread_function(void) {
    if (!gpio_is_ready_dt(&led)) return;
    
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    while (1) {

        if (ultima_umidade < 1600){

            
            gpio_pin_set_dt(&led, 1);
            esta_irrigando = true;
            k_msleep(SLEEP_TIME_ON_MS);


            gpio_pin_set_dt(&led, 0);
            esta_irrigando = false;
            k_msleep(SLEEP_TIME_OFF_MS);

        }else{

            gpio_pin_set_dt(&led, 0);
            esta_irrigando = false;
            k_msleep(SLEEP_TIME_OFF_MS);
        }


    }

        
        


}
K_THREAD_DEFINE(led_thread_id, 512, led_thread_function, NULL, NULL, NULL, 7, 0, 0);



/* =========================================
 * THREAD DO HTTP (Envia o estado no JSON)
 * ========================================= */
static void send_http_post(struct pots_data data) {
    int sock;
    struct sockaddr_in server_addr;
    char payload[160]; /* Aumentado ligeiramente para a nova chave */
    char request[350];

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

    /* MODIFICAÇÃO: Inserção da chave "irrigando" passando true/false como string para o JSON */
    snprintf(payload, sizeof(payload), 
             "{\"potenciometro1\": %d, \"potenciometro2\": %d, \"irrigando\": %s}", 
             data.pot1, data.pot2, data.irrigando ? "true" : "false");
    
    snprintf(request, sizeof(request),
             "POST /api/dados HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s", HTTP_SERVER_IP, strlen(payload), payload);

    zsock_send(sock, request, strlen(request), 0);
    printk("[HTTP] ENVIADO -> Pot1: %d | Pot2: %d | Irrigando: %s\n", 
           data.pot1, data.pot2, data.irrigando ? "SIM" : "NAO");
    
    zsock_close(sock);
}

void http_client_thread(void) {
    const struct zbus_channel *chan;
    struct pots_data received_data;
    
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
 * THREAD PRINCIPAL (Adiciona o estado no ZBUS)
 * ========================================= */
static const struct adc_dt_spec pot1 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct adc_dt_spec pot2 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

int main(void) {
    int err1, err2;
    uint16_t buf_pot1;
    uint16_t buf_pot2;
    struct pots_data msg;

    wifi_init();

    struct adc_sequence seq1 = {
        .buffer = &buf_pot1,
        .buffer_size = sizeof(buf_pot1),
    };
    struct adc_sequence seq2 = {
        .buffer = &buf_pot2,
        .buffer_size = sizeof(buf_pot2),
    };

    if (!device_is_ready(pot1.dev) || adc_channel_setup_dt(&pot1) < 0 || adc_sequence_init_dt(&pot1, &seq1) < 0) {
        printk("Erro ao iniciar ADC no Potenciometro 1\n");
        return 0;
    }

    if (!device_is_ready(pot2.dev) || adc_channel_setup_dt(&pot2) < 0 || adc_sequence_init_dt(&pot2, &seq2) < 0) {
        printk("Erro ao iniciar ADC no Potenciometro 2\n");
        return 0;
    }

    while (1) {
        err1 = adc_read(pot1.dev, &seq1);
        err2 = adc_read(pot2.dev, &seq2);
        
        if (err1 == 0 && err2 == 0) {
            msg.pot1 = buf_pot1;
            msg.pot2 = buf_pot2;
            
            /* MODIFICAÇÃO: Captura o estado atualizado da irrigação antes de enviar */
            msg.irrigando = esta_irrigando;

            /* MODIFICAÇÃO: Atualiza a variável global que a Thread do LED consulta */
            ultima_umidade = buf_pot1;
            
            printk("[ADC] Lido Pot1: %d, Pot2: %d, Irrigando: %s | Publicando...\n", 
                   msg.pot1, msg.pot2, msg.irrigando ? "SIM" : "NAO");
                   
            zbus_chan_pub(&adc_data_chan, &msg, K_NO_WAIT);
        } else {
            printk("[ADC] Falha ao ler um dos sensores analógicos.\n");
        }
        
        k_msleep(3500); 
    }

    return 0;
}