#ifndef DNS_H
#define DNS_H

#include "esp_err.h"

esp_err_t dns_init(void);

static int dns_forward(uint8_t *request, int request_length, uint8_t *response)

static bool dns_get_domain(uint8_t *packet, int length, char *domain, size_t domain_size)

static void dns_make_block_response(uint8_t *packet)

static void dns_task(void *parameter)

#endif
