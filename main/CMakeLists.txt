idf_component_register(
        SRC_DIRS .
        INCLUDE_DIRS .
        EMBED_FILES img.jpg
)
target_compile_options(${COMPONENT_LIB} PRIVATE -w)