add_library(vestra_ui STATIC
  src/office_ribbon.cpp
  src/vestra_design.cpp
  src/theme_manager.cpp
  src/translation_manager.cpp
  src/settings_dialog.cpp
  include/vestra/ui/office_ribbon.h
  include/vestra/ui/vestra_design.h
  include/vestra/ui/settings_dialog.h
  include/vestra/ui/translation_manager.h
  resources/vestra_resources.qrc
)
target_include_directories(vestra_ui PUBLIC include)
target_link_libraries(vestra_ui PUBLIC Qt6::Core Qt6::Gui Qt6::Svg Qt6::Widgets vestra_core vestra_settings)

