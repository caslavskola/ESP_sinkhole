#include <string.h>

#include "esp_log.h"
#include "esp_partition.h"

#include "storage.h"

static const char *TAG = "STORAGE";


esp_err_t storage_init(void)
{
    ESP_LOGI(TAG, "Storage ready");

    return ESP_OK;
}


esp_err_t storage_open(const char *label, storage_handle_t *handle)
{
    if (label == NULL || handle == NULL)
        return ESP_ERR_INVALID_ARG;

    const esp_partition_t *partition;

    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);

    if (partition == NULL)
        ESP_LOGE(TAG, "Partition not found: %s", label);
        return ESP_ERR_NOT_FOUND;

    handle->partition = partition;

    ESP_LOGI(TAG, "Opened partition: %s", label);

    return ESP_OK;
}


esp_err_t storage_read(const storage_handle_t *handle, uint32_t offset, void *buffer, size_t length)
{
    if (handle == NULL ||
        handle->partition == NULL ||
        buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    if ((offset + length) > handle->partition->size)
    {
        return ESP_ERR_INVALID_SIZE;
    }


    return esp_partition_read(handle->partition, offset, buffer, length);
}


esp_err_t storage_write(const storage_handle_t *handle, uint32_t offset, const void *buffer, size_t length)
{
    if (handle == NULL ||
        handle->partition == NULL ||
        buffer == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    if ((offset + length) > handle->partition->size)
    {
        return ESP_ERR_INVALID_SIZE;
    }


    return esp_partition_write(handle->partition, offset, buffer, length);
}


esp_err_t storage_erase(
    const storage_handle_t *handle)
{
    if (handle == NULL ||
        handle->partition == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    return esp_partition_erase_range(handle->partition, 0, handle->partition->size);
}


size_t storage_size(
    const storage_handle_t *handle)
{
    if (handle == NULL ||
        handle->partition == NULL)
    {
        return 0;
    }

    return handle->partition->size;
}
