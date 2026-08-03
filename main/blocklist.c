#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "blocklist.h"
#include "storage.h"
#include "stats.h"

#include "hash.h"

#define BLOCKLIST_FILE_SIGNATURE 0x424C4B4CU //The ASCII for "BLKL" encoded in unsigned HEX

static const char *TAG = "BLOCKLIST";


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

static storage_handle_t storage;
static blocklist_header_t header;

static uint64_t hash_domain(
    const char *domain)
{
    return XXH3_64bits(
        domain,
        strlen(domain));
}

esp_err_t blocklist_init(void)
{
    esp_err_t result;
    result = storage_open("blocklist", &storage);

    if (result != ESP_OK)
        return result;

    return blocklist_load();
}


esp_err_t blocklist_load(void)
{
    esp_err_t result;
    result = storage_read( &storage, 0, &header, sizeof(header));

    if (result != ESP_OK){
        loaded = false;
        return result;
    }

    if (header.magic != BLOCKLIST_MAGIC){
        ESP_LOGW( TAG, "Invalid blocklist");
        loaded = false;
        return ESP_ERR_INVALID_VERSION;
    }

    loaded = true;
    ESP_LOGI(TAG, "Loaded %lu domains",(unsigned long)header->count);

    return ESP_OK;
}


bool blocklist_check(const char *domain)
{
    if (header == NULL){
        return false;
    }

    uint64_t hash = hash_domain(domain);

    size_t left = 0;
    size_t right = header->count;

    while (left < right)
    {
        size_t middle = left + (right - left) / 2;

        uint64_t value = entries[middle].hash;

        if (hash == value)
            return true;

        if (hash < value)
            right = middle;
        else
            left = middle + 1;
    }

    return false;
}
