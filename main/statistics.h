#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>
#include "esp_err.h"

esp_err_t block_counter_init(void);

void block_counter_increment(void);

uint64_t block_counter_get(void);

esp_err_t block_counter_save(void);

void block_counter_reset(void);


typedef struct
{
    uint32_t file_signature;
    uint64_t count;
} block_counter_data_t;

#endif 
