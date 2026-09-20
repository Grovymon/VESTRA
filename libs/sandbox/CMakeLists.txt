add_library(vestra_sandbox STATIC
  src/worker_session.cpp
  include/vestra/sandbox/worker_session.h
)
target_include_directories(vestra_sandbox PUBLIC include)
target_link_libraries(vestra_sandbox PUBLIC Qt6::Core Qt6::Network vestra_core vestra_ipc)

