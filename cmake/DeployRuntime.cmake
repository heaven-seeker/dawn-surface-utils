foreach(runtime_file IN LISTS RUNTIME_FILES)
    file(COPY "${runtime_file}" DESTINATION "${OUTPUT_DIR}")
endforeach()
