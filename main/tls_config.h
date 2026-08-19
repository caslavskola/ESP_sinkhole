#ifndef TLS_CONFIG_H
#define TLS_CONFIG_H

#include "esp_http_client.h"

void tls_configure_http_client(
    esp_http_client_config_t *config);

#endif /* TLS_CONFIG_H */
