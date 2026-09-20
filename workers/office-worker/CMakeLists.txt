add_executable(vestra-document-worker
  main.cpp
  worker_server.cpp
  worker_server.h
)
target_link_libraries(vestra-document-worker PRIVATE
  Qt6::Core Qt6::Network vestra_core vestra_ipc vestra_security)


