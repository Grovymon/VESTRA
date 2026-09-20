add_executable(vestra WIN32
  main.cpp
  launcher_window.cpp
  launcher_window.h
)
target_link_libraries(vestra PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets
  vestra_core vestra_logging vestra_settings vestra_security vestra_sandbox vestra_ui)

install(TARGETS vestra RUNTIME DESTINATION .)


