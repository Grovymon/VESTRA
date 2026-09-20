add_library(vestra_logging STATIC src/logger.cpp)
target_include_directories(vestra_logging PUBLIC include)
target_link_libraries(vestra_logging PUBLIC Qt6::Core vestra_core)


