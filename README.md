# ESP_sinkhole
# ESP32 DNS Sinkhole

A small DNS sinkhole for the ESP32.

The device receives DNS requests from local clients. It checks each domain against a local blocklist.

If the domain is blocked, the device returns a sinkhole response.

If the domain is not blocked, the device sends the request to an upstream DNS server.

The device also provides a small HTTP API for status and blocklist updates.

## Status

The project is in the prototype stage.

The current prototype provides:

* DNS request handling.
* Domain blocklist lookup.
* Upstream DNS forwarding.
* Blocked request counting.
* Persistent block counter storage.
* Blocklist storage in flash.
* Blocklist download.
* Blocklist parsing.
* HTTP API.
* Plain text API responses.

The project does not provide:

* An allow list.
* Custom lists.
* Multiple list sources.
* HTTPS.
* Flash memory mapping for the blocklist.
* A web interface.
* Automatic list update scheduling.

## Hardware

The project targets the ESP32-D0WD-V3.

The device has limited RAM and flash. The project uses ESP-IDF native APIs.

## Software

The project uses:

* ESP-IDF.
* FreeRTOS.
* `esp_http_server`.
* `esp_http_client`.
* ESP-IDF partition APIs.
* XXH3 from xxHash.

## Architecture

### DNS

The DNS module handles DNS requests.

It checks each domain with the blocklist module.

Blocked requests receive a sinkhole response.

Allowed requests are sent to the configured upstream DNS server.

The DNS module does not know how the blocklist is created.

### Blocklist

The blocklist module manages the compiled blocklist database.

The database stores domain hashes instead of full domain names.

The module uses binary search on sorted hashes.

The module reads the database from flash through the storage module.

The current prototype uses flash reads for each search operation.

The project can later use `esp_partition_mmap()`.

### List manager

The list manager downloads a blocklist.

It supports simple domain lists and hosts files.

Examples of accepted entries are:

```text
example.com
ads.example.com
```

and:

```text
0.0.0.0 ads.example.com
127.0.0.1 tracker.example.com
```

The list manager:

1. Downloads the list.
2. Parses domains.
3. Creates domain hashes.
4. Sorts the hashes.
5. Removes duplicate hashes.
6. Creates the blocklist database.
7. Stores the database in flash.
8. Reloads the blocklist.

The list manager does not perform DNS checks.

### Storage

The storage module provides access to ESP-IDF flash partitions.

Other modules use storage handles.

### Statistics

The statistics module stores the blocked request counter.

The counter is updated when the DNS module blocks a request.

The counter is saved to flash after a time interval.

The counter does not write to flash for every blocked request.

## Blocklist database

The blocklist database has a simple binary format.

```text
Header
Hash entry
Hash entry
Hash entry
...
```

The header contains:

```c
typedef struct
{
    uint32_t file_signature;
    uint32_t version;
    uint32_t count;
} blocklist_header_t;
```

Each entry contains one 64-bit hash:

```c
typedef struct
{
    uint64_t hash;
} blocklist_entry_t;
```

The project uses XXH3 to create the domain hash.

```c
XXH3_64bits(domain, length);
```

The hashes are stored in sorted order.

This allows the blocklist module to use binary search.

## HTTP API

The prototype provides a small HTTP API.

The API uses plain text.

### Get status

Request:

```text
GET /api/status
```

Example:

```bash
curl http://<ESP32-IP>/api/status
```

Response:

```text
blocked=0
domains=0
```

`blocked` is the number of blocked DNS requests.

`domains` is the number of domains in the current blocklist.

### Update the blocklist

Request:

```text
POST /api/blocklist/update
```

The request body contains the list URL.

Example:

```bash
curl -X POST \
  -d 'http://<SERVER-IP>:8000/list.txt' \
  http://<ESP32-IP>/api/blocklist/update
```

A successful update returns:

```text
result=ok
```
## Build

Set up ESP-IDF before building the project.

Then run:

```bash
idf.py build
```

Flash the device with:

```bash
idf.py flash
```

Open the serial monitor with:

```bash
idf.py monitor
```

You can do all of the above in one command:

```bash
idf.py build flash monitor
```

## Testing

Connect the ESP32 to the local network.

Start the serial monitor:

```bash
idf.py monitor
```

Find the IP address assigned to the ESP32.

Test the HTTP API:

```bash
curl http://<ESP32-IP>/api/status
```

Create a small test blocklist:

```text
example.com
ads.example.com
0.0.0.0 tracker.example.com
127.0.0.1 bad.example.com
```

Serve the file from a computer on the same network:

```bash
python3 -m http.server 8000
```

Update the ESP32:

```bash
curl -X POST \
  -d 'http://<COMPUTER-IP>:8000/test-list.txt' \
  http://<ESP32-IP>/api/blocklist/update
```

Check the blocklist count:

```bash
curl http://<ESP32-IP>/api/status
```

Test DNS:

```bash
nslookup example.com <ESP32-IP>
```

Blocked requests should receive the sinkhole response.

Other requests should be sent to the upstream DNS server.


## Planned work

The next planned work includes:

* Improve blocklist storage access.
* Add flash memory mapping.
* Add list configuration.
* Add custom block lists.
* Add allow lists.
* Add HTTPS.
