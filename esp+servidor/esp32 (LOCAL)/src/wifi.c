#include "wifi.h"
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

static struct net_mgmt_event_callback dhcp_cb;
static bool network_ready = false;

/* ====================================================================
 * Callback acionado pelo net_mgmt quando o IP é recebido
 * ==================================================================== */
static void net_event_handler(struct net_mgmt_event_callback *cb, 
                              uint64_t mgmt_event, 
                              struct net_if *iface) 
{
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        network_ready = true;
        printk("\n[WIFI] Rede conectada! IP obtido via DHCP.\n");
        
        if (iface->config.ip.ipv4 != NULL) {
            
            /* 1. Procura o IP do ESP32 */
            for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
                
                /* CORREÇÃO: Adicionado o acesso ao membro interno '.ipv4' */
                if (iface->config.ip.ipv4->unicast[i].ipv4.is_used) {
                    struct in_addr *ip_addr = &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr;
                    
                    /* CORREÇÃO: Transforma o ponteiro da estrutura num array de 4 bytes (Octetos do IP) */
                    uint8_t *ip = (uint8_t *)ip_addr;
                    printk("  -> IP do ESP32: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
                    
                    break;
                }
            }

            /* 2. Pega o endereço IP do Roteador (Gateway) */
            struct in_addr *gw_addr = &iface->config.ip.ipv4->gw;
            uint8_t *gw = (uint8_t *)gw_addr;
            printk("  -> IP do Roteador (Gateway): %d.%d.%d.%d\n\n", gw[0], gw[1], gw[2], gw[3]);
        }
    }
}

void wifi_init(void) 
{
    printk("[WIFI] Inicializando gerenciador de rede...\n");
    
    net_mgmt_init_event_callback(&dhcp_cb, net_event_handler, NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&dhcp_cb);
}

bool wifi_is_ready(void) 
{
    return network_ready;
}


/* ====================================================================
 * Comando Customizado da Shell para Conectar ao Wi-Fi
 * ==================================================================== */
static int cmd_conectar_wifi(const struct shell *sh, size_t argc, char **argv)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {0};

    if (argc < 2) {
        shell_error(sh, "Uso incorreto. Tente: conectar_wifi <SSID> [PALAVRA_PASSE]");
        return -EINVAL;
    }

    params.ssid = (const uint8_t *)argv[1];
    params.ssid_length = strlen(argv[1]);

    if (argc > 2) {
        params.psk = (const uint8_t *)argv[2];
        params.psk_length = strlen(argv[2]);
        params.security = WIFI_SECURITY_TYPE_PSK;
    } else {
        params.security = WIFI_SECURITY_TYPE_NONE;
    }

    params.channel = WIFI_CHANNEL_ANY;

    shell_print(sh, "Enviando solicitacao de conexao para o SSID: %s...", argv[1]);

    int err = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(struct wifi_connect_req_params));
    
    if (err) {
        shell_error(sh, "Erro ao repassar comando ao driver Wi-Fi: %d", err);
    } else {
        shell_print(sh, "Comando aceito. Aguarde a resposta DHCP...");
    }

    return err;
}

SHELL_CMD_REGISTER(conectar_wifi, NULL, "Conecta ao Wi-Fi: conectar_wifi <SSID> [SENHA]", cmd_conectar_wifi);