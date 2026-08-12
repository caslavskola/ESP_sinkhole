#include "statistics.h"
#include "storage.h"
#define BLOCK_COUNTER_PARTITION "statistics"
#define BLOCK_COUNTER_FILE_SIGNATURE 0x424C4B43U

static storage_handle_t s_storage;
static uint64_t s_block_count;

esp_err_t block_counter_init(void)
{
    block_counter_data_t data;
    esp_err_t result;

    result = storage_open(BLOCK_COUNTER_PARTITION, &s_storage);

    if (result != ESP_OK)
        return result;

    result = storage_read(&s_storage, 0, &s_block_count, sizeof(s_block_count));

    if (result != ESP_OK){
        s_block_count = 0;
        return ESP_OK;
    }

    if (data.file_signature != BLOCK_COUNTER_FILE_SIGNATURE){
        s_block_count = 0;
        return ESP_OK;
    }

    s_block_count = data.count;
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
    block_counter_data_t data;
    esp_err_t result;

    data.count = s_block_count;

    data.file_signature = BLOCK_COUNTER_FILE_SIGNATURE;

    result = storage_erase(&s_storage);

    if (result != ESP_OK)
        return result;

    return storage_write(&s_storage, 0, &data, sizeof(data));
}
