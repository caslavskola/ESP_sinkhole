#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include "xxhash.h"

uint64_t hash_domain(const char *domain);

#endif
