#ifndef BLOCKLIST_H
#define BLOCKLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

typedef struct
{
    uint32_t file_signature;
    uint32_t version;
    uint32_t count;
} blocklist_header_t;

typedef struct
{
    uint64_t hash;
} blocklist_entry_t;

esp_err_t blocklist_load(void);

bool blocklist_check(const char *domain);

size_t blocklist_count(void);

#endif
