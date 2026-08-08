#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_partition.h"

typedef struct
{
    const esp_partition_t *partition;
} storage_handle_t;

esp_err_t storage_init(void);


esp_err_t storage_open( 
    const char *label, 
    storage_handle_t *handle);

esp_err_t storage_read(
    const storage_handle_t *handle,
    uint32_t offset,
    void *buffer,
    size_t length);

esp_err_t storage_write(
    const storage_handle_t *handle,
    uint32_t offset,
    const void *buffer,
    size_t length);

esp_err_t storage_erase(
    const storage_handle_t *handle);

size_t storage_size(
    const storage_handle_t *handle);

#endif
