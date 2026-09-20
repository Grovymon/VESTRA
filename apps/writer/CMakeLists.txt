add_executable(vestra-writer WIN32
  main.cpp
  page_editor.cpp
  page_editor.h
  writer_window.cpp
  writer_window.h
)
target_link_libraries(vestra-writer PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport
  vestra_core vestra_document vestra_logging vestra_sandbox vestra_settings vestra_ui)

install(TARGETS vestra-writer RUNTIME DESTINATION .)

