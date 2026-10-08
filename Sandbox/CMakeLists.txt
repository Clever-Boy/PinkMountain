add_executable(Sandbox src/SandboxApp.cpp)

target_link_libraries(Sandbox PRIVATE Pink)

set_target_properties(Sandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Lesson 8+: shaders (and later textures) are data, not code — they live in
# Sandbox/assets/ and are copied next to the binary so relative "assets/..."
# paths resolve no matter which generator builds the project.
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Sandbox>/assets"
)
