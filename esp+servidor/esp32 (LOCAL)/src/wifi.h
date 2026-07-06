#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

/**
 * @brief Inicializa os callbacks de gerenciamento de rede.
 */
void wifi_init(void);

/**
 * @brief Verifica se a placa obteve um endereço IP (DHCP concluído).
 * @return true se conectada e pronta, false caso contrário.
 */
bool wifi_is_ready(void);

#endif /* WIFI_H */