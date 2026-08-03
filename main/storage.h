#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct
{
    const esp_partition_t *partition;
} storage_handle_t;

esp_err_t storage_init(void);

esp_err_t storage_read(
    const char *label,
    uint32_t offset,
    void *buffer,
    size_t length);

esp_err_t storage_write(
    storage_object_t object,
    uint32_t offset,
    const void *buffer,
    size_t length);

esp_err_t storage_erase(
    storage_object_t object);

size_t storage_size(
    storage_object_t object);

#endif
