idf_component_register(
    SRCS "xxhash.c"
    INCLUDE_DIRS "include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    XXH_NO_INLINE_HINTS=1
)
