add_executable(vestra_core_tests core/file_type_test.cpp)
target_link_libraries(vestra_core_tests PRIVATE Qt6::Test vestra_core)
add_test(NAME vestra_core_tests COMMAND vestra_core_tests)

add_executable(vestra_ipc_tests ipc/frame_codec_test.cpp)
target_link_libraries(vestra_ipc_tests PRIVATE Qt6::Test vestra_ipc)
add_test(NAME vestra_ipc_tests COMMAND vestra_ipc_tests)

add_executable(vestra_security_tests security/document_scanner_test.cpp)
target_link_libraries(vestra_security_tests PRIVATE Qt6::Test vestra_security)
add_test(NAME vestra_security_tests COMMAND vestra_security_tests)

add_executable(vestra_worker_integration_tests sandbox/worker_session_test.cpp)
target_link_libraries(vestra_worker_integration_tests PRIVATE Qt6::Test vestra_sandbox)
add_dependencies(vestra_worker_integration_tests vestra-document-worker)
add_test(NAME vestra_worker_integration_tests COMMAND vestra_worker_integration_tests)

add_executable(vestra_settings_tests settings/app_settings_test.cpp)
target_link_libraries(vestra_settings_tests PRIVATE Qt6::Test vestra_settings vestra_ui)
add_test(NAME vestra_settings_tests COMMAND vestra_settings_tests)

if(VESTRA_ENABLE_QT_PDF OR VESTRA_ENABLE_PDFIUM)
  add_executable(vestra_pdf_engine_tests document/pdf_engine_test.cpp)
  target_link_libraries(vestra_pdf_engine_tests PRIVATE
    Qt6::Test Qt6::Gui Qt6::PrintSupport vestra_document)
  add_test(NAME vestra_pdf_engine_tests COMMAND vestra_pdf_engine_tests)
endif()

