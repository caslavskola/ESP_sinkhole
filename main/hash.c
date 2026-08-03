#include <string.h>

#include "xxhash.h"
#include "hash.h"

static size_t to_lower_case(
    const char *input,
    char *output,
    size_t output_size)
{
    size_t i = 0;

    while (input[i] != '\0' && i < output_size - 1){
        char c = input[i];
        if (c >= 'A' && c <= 'Z'){
            c = (char)(c + ('a' - 'A'));
        }
        output[i] = c;
        i++;
    }

    output[i] = '\0';

    return i;
}

uint64_t hash_domain(const char *domain)
{
    char buffer[256];
    size_t i;

    size_t length = to_lower_case(
        domain,
        normalized,
        sizeof(normalized));
    buffer[i] = '\0';

    return XXH3_64bits(buffer, i);
}
