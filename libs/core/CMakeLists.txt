add_library(vestra_core STATIC
  src/application_paths.cpp
  src/file_type.cpp
)

target_include_directories(vestra_core PUBLIC include)
target_link_libraries(vestra_core PUBLIC Qt6::Core)
target_compile_definitions(vestra_core PUBLIC VESTRA_VERSION="${PROJECT_VERSION}")


