add_library(vestra_settings STATIC src/app_settings.cpp)
target_include_directories(vestra_settings PUBLIC include)
target_link_libraries(vestra_settings PUBLIC Qt6::Core vestra_core)


