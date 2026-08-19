#include "esp_crt_bundle.h"

#include "tls_config.h"

void tls_configure_http_client(
    esp_http_client_config_t *config)
{
    if (config == NULL)
    {
        return;
    }

    config->crt_bundle_attach = esp_crt_bundle_attach;
}
