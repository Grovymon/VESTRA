add_executable(vestra-sheets WIN32
  main.cpp
  sheets_window.cpp
  sheets_window.h
)
target_link_libraries(vestra-sheets PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets
  vestra_core vestra_logging vestra_settings vestra_ui vestra_sandbox vestra_document
)
target_compile_definitions(vestra-sheets PRIVATE VESTRA_VERSION="${PROJECT_VERSION}")
set_target_properties(vestra-sheets PROPERTIES OUTPUT_NAME "vestra-sheets")

