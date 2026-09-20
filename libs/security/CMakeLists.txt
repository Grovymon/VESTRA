add_library(vestra_security STATIC src/document_scanner.cpp)
target_include_directories(vestra_security PUBLIC include)
target_link_libraries(vestra_security PUBLIC Qt6::Core vestra_core)


