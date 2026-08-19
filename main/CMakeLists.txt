idf_component_register(
    SRCS 
        "ESP_sinkhole.c"
        "wifi.c"
        "dns.c"
        "blocklist.c"
        "statistics.c"
        "storage.c"
        "list_manager.c"
        "hash.c"
        "http_api.c"
        "tls_config.c"

    INCLUDE_DIRS "."
)
