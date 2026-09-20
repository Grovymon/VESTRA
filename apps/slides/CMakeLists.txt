add_executable(vestra-slides WIN32
  main.cpp
  slides_window.cpp
  slides_window.h
)
target_link_libraries(vestra-slides PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport
  vestra_core vestra_logging vestra_settings vestra_ui vestra_sandbox vestra_document
)
target_compile_definitions(vestra-slides PRIVATE VESTRA_VERSION="${PROJECT_VERSION}")
set_target_properties(vestra-slides PROPERTIES OUTPUT_NAME "vestra-slides")

