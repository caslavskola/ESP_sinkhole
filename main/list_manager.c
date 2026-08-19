#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "list_manager.h"
#include "blocklist.h"
#include "storage.h"
#include "esp_http_client.h"
#include "tls_config.h"

#include "hash.h"

#define BLOCKLIST_PARTITION "blocklist"
#define TAG "LIST_MANAGER"
#define BLOCKLIST_FILE_SIGNATURE 0x424C4B4CU
#define BLOCKLIST_VERSION 1
#define LIST_URL_MAX_LENGTH 256
#define LIST_LINE_MAX_LENGTH 512
#define HASH_ARRAY_INITIAL_CAPACITY 1024

static int compare_hash(const void *left, const void *right)
{
    uint64_t a = ((const blocklist_entry_t *)left)->hash;
    uint64_t b = ((const blocklist_entry_t *)right)->hash;

    if (a < b)
        return -1;

    if (a > b)
        return 1;

    return 0;
}

static size_t remove_duplicates(blocklist_entry_t *entries, size_t count)
{
    size_t read;
    size_t write;

    if (count == 0)
        return 0;

    write = 1;

    for (read = 1; read < count; read++)
    {
        if (entries[read].hash != entries[write - 1].hash)
            entries[write++] = entries[read];
    }

    return write;
}

static char *skip_whitespace( char *text) 
{ 
    while (*text != '\0' 
            && (*text == ' ' || *text == ' ')) 
	text++; 
    return text; 
}

static void trim_whitespace( char *text) 
{ 
    size_t length; 
    length = strlen(text); 
    while (length > 0 
           && (text[length-1] == ' ' || text[length-1] == '\t'))
    { 
        text[length - 1] = '\0'; 
        length--; 
    } 
}

static bool valid_domain( const char *domain) 
{ 
    size_t length; 
    size_t label_length; 
    size_t i; 
    length = strlen(domain); 
    if (length == 0 
        || length > 253 
        || domain[0] == '.' 
        || domain[0] == '-' 
        || domain[length - 1] == '-') 
        return false;

    label_length = 0;
    for (i = 0; i < length; i++) 
    { 
        char c = domain[i]; 
        if (c == '.') { 
            if (label_length == 0 
                || label_length > 63) 
                return false; 
            if (domain[i - 1] == '-')  
                return false;  
            label_length = 0; 
            continue; 
        } 

        if (c == '-' 
            || (c >= 'a' && c <= 'z') 
            || (c >= 'A' && c <= 'Z') 
            || (c >= '0' && c <= '9')) { 
            
            if (label_length == 0 
                && c == '-')
                return false; 
            label_length++;

            if (label_length > 63) 
                return false;

            continue; 
        } 
        return false; 
    } 

    if (label_length == 0 
        || label_length > 63 
        || domain[length - 1] == '-') 
        return false;

    return true; 
}

static bool parse_domain( char *line, char *domain, size_t domain_size) 
{ 
    char *start;
    char *comment;
    char *first;
    char *second;
    char *extra;
    char *saveptr;
    
    trim_whitespace(line);

    start = skip_whitespace(line);

    if (*start == '\0' 
        ||  *start == '#')
        return false;

    comment = strchr(start, '#');

    if (comment != NULL){
        *comment = '\0';
        trim_whitespace(start);
    }

    if (*start == '\0')
        return false;

    first = strtok_r(start, " \t",&saveptr);

    if (first == NULL)
        return false;

    second = strtok_r(NULL, " \t", &saveptr);

    /*
     * A single token is a domain list entry.
     */
    if (second == NULL)
    {
        if (!valid_domain(first))
            return false;
        

        if (strlen(first) >= domain_size)
            return false;
        

        strcpy(domain, first);

        return true;
    }

    /*
     * Two tokens are treated as a hosts file entry.
     */
    if (strcmp(first, "0.0.0.0") != 0 
        && strcmp(first, "127.0.0.1") != 0)
        return false;

    if (!valid_domain(second))
        return false;
    
    if (strlen(second) >= domain_size)
        return false;

    /*
     * Ignore any extra fields.
     */
    extra = strtok_r(NULL, " \t", &saveptr);

    (void)extra;
    strcpy(domain, second);
    return true; 
}

static esp_err_t add_hash(
    blocklist_entry_t **entries,
    size_t *count, 
    size_t *capacity, 
    uint64_t hash)
{
    blocklist_entry_t *new_entries;
    size_t new_capacity;

    if (*count >= *capacity) {
        new_capacity = *capacity * 2;

        new_entries = realloc(*entries, new_capacity * sizeof(blocklist_entry_t));

        if (new_entries == NULL)
            return ESP_ERR_NO_MEM;
        
        *entries = new_entries;
        *capacity = new_capacity;
    }

    (*entries)[*count].hash = hash;
    (*count)++;

    return ESP_OK;
}

static esp_err_t parse_line(
    char *line, 
    blocklist_entry_t **entries, 
    size_t *count, 
    size_t *capacity)
{
    char domain[254];
    uint64_t hash;

    esp_err_t result;

    if (!parse_domain(line, domain, sizeof(domain))) 
        return ESP_OK;


    hash = XXH3_64bits(domain, strlen(domain));

    result = add_hash(
        entries,
        count,
        capacity,
        hash);

    return result;
}

static esp_err_t process_data(
    const char *data,
    size_t length,
    char *line,
    size_t *line_length,
    blocklist_entry_t **entries,
    size_t *count,
    size_t *capacity)
{
    size_t i;
    esp_err_t result;

    for (i = 0; i < length; i++) {
        char c = data[i];

        if (c == '\n') {
            line[*line_length] = '\0';

            result = parse_line(line, entries, count, capacity);

            if (result != ESP_OK)
                return result;

            *line_length = 0;
            continue;
        }

        if (c == '\r')
            continue;

        if (*line_length >= LIST_LINE_MAX_LENGTH - 1)
            return ESP_ERR_INVALID_SIZE;

        line[*line_length] = c;
        (*line_length)++;
    }

    return ESP_OK;
}


static esp_err_t write_database(blocklist_entry_t *entries, size_t count)
{
    storage_handle_t storage;
    blocklist_header_t header;
    size_t database_size;
    esp_err_t result;

    result = storage_open(BLOCKLIST_PARTITION, &storage);

    if (result != ESP_OK)
        return result;

    database_size =sizeof(blocklist_header_t) + (count * sizeof(blocklist_entry_t));

    if (database_size > storage_size(&storage)){
        ESP_LOGE(TAG, "Blocklist is too large");
        return ESP_ERR_INVALID_SIZE;
    }

    header.file_signature= BLOCKLIST_FILE_SIGNATURE;
    header.version = BLOCKLIST_VERSION;
    header.count = count;

    result = storage_erase(&storage);

    if (result != ESP_OK)
        return result;

    result = storage_write(&storage, 0, &header, sizeof(header));

    if (result != ESP_OK)
        return result;

    if (count == 0)
        return ESP_OK;

    result = storage_write(&storage, sizeof(header), entries, count * sizeof(blocklist_entry_t));

    return result;
}

static void list_cleanup(
    esp_http_client_handle_t client,
    char *line,
    blocklist_entry_t *entries)
{
    if (client != NULL)
    {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    }

    free(line);
    free(entries);
}

static esp_err_t list_update_internal(
    esp_http_client_handle_t client,
    char *line,
    blocklist_entry_t **entries,
    size_t *count,
    size_t *capacity)
{
    uint8_t buffer[1024];
    size_t line_length = 0;
    esp_err_t result;

    result = esp_http_client_open(client, 0);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open list");
        return result;
    }

    result = esp_http_client_fetch_headers(client);

    if (result < 0)
    {
        ESP_LOGE(TAG, "Failed to read HTTP headers");
        return ESP_FAIL;
    }

    if (esp_http_client_get_status_code(client) != 200)
    {
        ESP_LOGE(TAG, "HTTP status %d", esp_http_client_get_status_code(client));
        return ESP_FAIL;
    }

    int received;
    do{
        received = esp_http_client_read(client, (char *)buffer, sizeof(buffer));

        if (received < 0)
        {
            ESP_LOGE(TAG, "Failed to read list");
            return ESP_FAIL;
        }

        result = process_data(
            (const char *)buffer,
            received,
            line,
            &line_length,
            entries,
            count,
            capacity);

        if (result != ESP_OK)
            return result;
    }while (received!=0);


    /*
     * Process the last line if the file does not
     * end with a newline.
     */
    if (line_length > 0){
        line[line_length] = '\0';

        result = parse_line(line, entries, count, capacity);

        if (result != ESP_OK)
            return result;
    }

    ESP_LOGI(
    TAG,
    "Internal update count: %lu",
    (unsigned long)*count);

    return ESP_OK;
}

esp_err_t list_update(
    const char *url)
{
    esp_http_client_config_t config = {
        .url = url, 
        .timeout_ms = 15000,
        .timeout_ms = 15000, 
        .disable_auto_redirect = false, 
        .max_redirection_count = 5,};

    esp_http_client_handle_t client = NULL;
    blocklist_entry_t *entries = NULL;
    char *line = NULL;
    size_t count = 0;
    size_t capacity = HASH_ARRAY_INITIAL_CAPACITY;
    esp_err_t result;

    if (url == NULL)
        return ESP_ERR_INVALID_ARG;

    if (strlen(url) >= LIST_URL_MAX_LENGTH)
        return ESP_ERR_INVALID_SIZE;  

    entries = malloc(capacity * sizeof(blocklist_entry_t));

    if (entries == NULL)
        return ESP_ERR_NO_MEM;

    line = malloc(LIST_LINE_MAX_LENGTH);

    if (line == NULL)
    {
        list_cleanup(NULL, NULL, entries);
        return ESP_ERR_NO_MEM;
    }

    tls_configure_http_client(&config);

    client = esp_http_client_init(&config);

    if (client == NULL)
    {
        list_cleanup(NULL, line, entries);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Downloading blocklist");

    result = list_update_internal(
        client,
        line,
        &entries,
        &count,
        &capacity);

    if (result != ESP_OK)
    {
        list_cleanup(client, line, entries);
        return result;
    }

    ESP_LOGI(TAG, "Parsed %lu domains", (unsigned long)count);

    qsort(entries, count, sizeof(blocklist_entry_t), compare_hash);

    count = remove_duplicates(entries, count);

    ESP_LOGI(TAG, "Unique domains: %lu", (unsigned long)count);

    result = write_database(entries, count);

    if (result == ESP_OK)
        result = blocklist_load();

    list_cleanup(client, line, entries);

    return result;
}
