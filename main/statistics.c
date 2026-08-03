#include "block_counter.h"
#define BLOCK_COUNTER_PARTITION "statistics"

static storage_handle_t s_storage;
static uint64_t s_block_count;

esp_err_t block_counter_init(void)
{
    esp_err_t result;

    result = storage_open(BLOCK_COUNTER_PARTITION, &s_storage);

    if (result != ESP_OK)
        return result;

    result = storage_read(&s_storage, 0, &s_block_count, sizeof(s_block_count));

    if (result != ESP_OK)
        s_block_count = 0;

    return ESP_OK;
}


void block_counter_increment(void)
{
    s_block_count++;
}

uint64_t block_counter_get(void)
{
    return s_block_count;
}

void block_counter_reset(void)
{
    s_block_count = 0;
}

esp_err_t block_counter_save(void)
{
    esp_err_t result;
    result = storage_erase(&s_storage);

    if (result != ESP_OK)
        return result;

    return storage_write(&s_storage, 0, &s_block_count, sizeof(s_block_count));
}
