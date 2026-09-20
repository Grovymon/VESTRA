add_library(vestra_ipc STATIC
  src/frame_codec.cpp
  src/messages.cpp
)
target_include_directories(vestra_ipc PUBLIC include)
target_link_libraries(vestra_ipc PUBLIC Qt6::Core Qt6::Network)


