add_executable(vestra-pdf WIN32
  main.cpp
  pdf_window.cpp
  pdf_window.h
)
target_link_libraries(vestra-pdf PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets
  vestra_core vestra_document vestra_logging vestra_sandbox vestra_settings vestra_ui)

install(TARGETS vestra-pdf RUNTIME DESTINATION .)


