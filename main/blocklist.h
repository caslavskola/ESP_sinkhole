#ifndef BLOCKLIST_H
#define BLOCKLIST_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

esp_err_t blocklist_load(void);

bool blocklist_check(const char *domain);

#endif
