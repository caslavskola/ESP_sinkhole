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


esp_err_t blocklist_load(void)
{
    const void *image;

    image = storage_blocklist_data();

    if (image == NULL){
        ESP_LOGW(TAG, "No blocklist");
        return ESP_ERR_NOT_FOUND;
    }

    header = image;

    if (header->magic != BLOCKLIST_MAGIC){
        header = NULL;
        return ESP_ERR_INVALID_RESPONSE;
    }

    entries = (const blocklist_entry_t *)(header + 1);

    ESP_LOGI(
        TAG,
        "Loaded %lu domains",
        (unsigned long)header->count);

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
