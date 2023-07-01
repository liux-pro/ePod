idf_component_register(
        SRC_DIRS .
        INCLUDE_DIRS .
        EMBED_FILES img.jpg
)
target_compile_options(${COMPONENT_LIB} PRIVATE -w)

#
#add_custom_target(
#        go
#        COMMAND cmd /k "${IDFTOOL} build flash monitor"
#        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
#        USES_TERMINAL
#        VERBATIM
#)