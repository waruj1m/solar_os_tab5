file(GLOB_RECURSE MICROPYTHON_EMBED_SRCS
    "micropython_embed/*.c"
)

idf_component_register(
    SRCS ${MICROPYTHON_EMBED_SRCS}
    INCLUDE_DIRS
        "."
        "micropython_embed"
        "micropython_embed/port"
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
)
