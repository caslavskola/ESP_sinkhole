#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "list_manager.h"
#include "blocklist.h"
#include "storage.h"

#include "hash.h"

#define TAG "LIST_MANAGER"

#define BLOCKLIST_FILE_SIGNATURE   0x424C4B4CU
#define BLOCKLIST_VERSION 1

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t count;
} blocklist_header_t;

typedef struct
{
    uint64_t hash;
} blocklist_entry_t;

static int compare_hash(
    const void *left,
    const void *right)
{
    uint64_t a = ((const blocklist_entry_t *)left)->hash;
    uint64_t b = ((const blocklist_entry_t *)right)->hash;

    if (a < b)
    {
        return -1;
    }

    if (a > b)
    {
        return 1;
    }

    return 0;
}

static size_t remove_duplicates(
    blocklist_entry_t *entries,
    size_t count)
{
    size_t read;
    size_t write;

    if (count == 0)
    {
        return 0;
    }

    write = 1;

    for (read = 1; read < count; read++)
    {
        if (entries[read].hash != entries[write - 1].hash)
        {
            entries[write++] = entries[read];
        }
    }

    return write;
}

static esp_err_t write_database(
    blocklist_entry_t *entries,
    size_t count)
{
    storage_handle_t storage;
    blocklist_header_t header;
    esp_err_t result;

    result = storage_open(
        "blocklist",
        &storage);

    if (result != ESP_OK)
    {
        return result;
    }

    result = storage_erase(&storage);

    if (result != ESP_OK)
    {
        return result;
    }

    header.magic = BLOCKLIST_FILE_SIGNATURE;
    header.version = BLOCKLIST_VERSION;
    header.count = count;

    result = storage_write(
        &storage,
        0,
        &header,
        sizeof(header));

    if (result != ESP_OK)
    {
        return result;
    }

    return storage_write(
        &storage,
        sizeof(header),
        entries,
        count * sizeof(blocklist_entry_t));
}

esp_err_t list_manager_update(
    const char *url)
{
    blocklist_entry_t *entries = NULL;
    size_t count = 0;
    esp_err_t result;

    if (url == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(
        TAG,
        "Downloading blocklist: %s",
        url);

    /*
     * TODO
     *
     * Download the list.
     * Parse each line.
     * Hash each domain.
     * Store hashes in entries[].
     * Set count.
     */

    if (entries == NULL)
    {
        return ESP_FAIL;
    }

    qsort(
        entries,
        count,
        sizeof(blocklist_entry_t),
        compare_hash);

    count = remove_duplicates(
        entries,
        count);

    result = write_database(
        entries,
        count);

    free(entries);

    if (result != ESP_OK)
    {
        return result;
    }

    return blocklist_load();
}
