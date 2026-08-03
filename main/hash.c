static uint64_t hash_domain(const char *domain)
{
    return XXH3_64bits(
        domain,
        strlen(domain)
    );
}
