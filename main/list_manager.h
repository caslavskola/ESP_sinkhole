#ifndef LIST_MANAGER_H
#define LIST_MANAGER_H

#include "esp_err.h"

esp_err_t list_update(const char *url);

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


#endif 
