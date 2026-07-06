#include "wifi.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

/* Configurações da Rede */
#define WIFI_SSID     "brisa-2789486"
#define WIFI_PASSWORD "ytokjmue"

static bool wifi_connected = false;
static bool wifi_got_ip = false;

static struct net_if *wifi_iface;

/* Estruturas independentes para não colidir máscaras de L2 (Wi-Fi) e L3 (IP) */
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

/* Sincronização via Semáforos */
static struct k_sem wifi_connected_sem;
static struct k_sem wifi_got_ip_sem;

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t mgmt_event,
                               struct net_if *iface);


static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t mgmt_event,
                               struct net_if *iface)
{
    switch (mgmt_event) {

    case NET_EVENT_WIFI_CONNECT_RESULT:
        printk("Wi-Fi conectado ao AP\n");
        wifi_connected = true;
        k_sem_give(&wifi_connected_sem);
        break;

    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        printk("Wi-Fi desconectado\n");
        wifi_connected = false;
        break;

    case NET_EVENT_IPV4_ADDR_ADD:
        printk("IP obtido\n");
        wifi_got_ip = true;
        k_sem_give(&wifi_got_ip_sem);
        break;

    default:
        break;
    }
}

int wifi_init(void)
{
    printk("\n=====================================\n");
    printk(" Inicializando Wi-Fi\n");
    printk("=====================================\n");

    wifi_iface = net_if_get_default();

    if (!wifi_iface) {
        printk("Erro: interface não encontrada\n");
        return -1;
    }

    k_sem_init(&wifi_connected_sem, 0, 1);
    k_sem_init(&wifi_got_ip_sem, 0, 1);

    /* Registra os eventos da camada Wi-Fi (L2) */
    net_mgmt_init_event_callback(
        &wifi_cb,
        wifi_event_handler,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    /* Registra os eventos da camada IP (L3) separadamente */
    net_mgmt_init_event_callback(
        &ipv4_cb,
        wifi_event_handler,
        NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_cb);

    printk("Wi-Fi pronto para conexão\n");

    return 0;
}


int wifi_connect(void)
{
    struct wifi_connect_req_params params;

    printk("\nConectando ao Wi-Fi...\n");

    memset(&params, 0, sizeof(params));

    params.ssid = (const uint8_t *)WIFI_SSID;
    params.ssid_length = strlen(WIFI_SSID);

    params.psk = WIFI_PASSWORD;
    params.psk_length = strlen(WIFI_PASSWORD);

    /* Mapeamento dos parâmetros validados no shell: -k 1 -b 2 -w 0 */
    params.security = WIFI_SECURITY_TYPE_PSK;   /* -k 1: Força WPA2-PSK */
    params.band = WIFI_FREQ_BAND_2_4_GHZ;       /* -b 2: Força operar apenas em 2.4GHz */
    params.mfp = WIFI_MFP_DISABLE;              /* -w 0: Desabilita MFP (Crucial para evitar Auth Error) */
    
    params.channel = WIFI_CHANNEL_ANY;

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT,
                       wifi_iface,
                       &params,
                       sizeof(params));

    if (ret) {
        printk("Erro ao iniciar conexão: %d\n", ret);
        return ret;
    }

    printk("Aguardando conexão física...\n");

    /* Trava a thread até o callback liberar o semáforo (Timeout de 15s) */
    if (k_sem_take(&wifi_connected_sem, K_SECONDS(15)) != 0) {
        printk("ERRO: timeout conectando ao Wi-Fi\n");
        return -1;
    }

    printk("Aguardando DHCP (IP)...\n");

    /* Trava a thread aguardando o IP do roteador (Timeout de 15s) */
    if (k_sem_take(&wifi_got_ip_sem, K_SECONDS(15)) != 0) {
        printk("ERRO: timeout aguardando IP\n");
        return -1;
    }

    /* Resgata o IP diretamente da estrutura aninhada do Zephyr 4.x */
    if (wifi_iface->config.ip.ipv4) {
        uint8_t *ip = wifi_iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr.s4_addr;
        
        printk("\nIP obtido: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    } else {
        printk("ERRO: interface sem IP\n");
        return -1;
    }

    printk("Wi-Fi operante com sucesso!\n\n");

    return 0;
}