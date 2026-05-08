set(PROD_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/profile.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/profile_manager.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/event_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/send_action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/delayed_action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/config_loader.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/config_loader_io.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/config_loader_merge.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/key_lookup.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/serial_action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/non_send_action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/pin_action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/macro_action.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/ble_config_reassembler.cpp
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/src/pedal_config.cpp
    ${CMAKE_SOURCE_DIR}/src/esp32/src/button.cpp
    ${CMAKE_SOURCE_DIR}/src/esp32/src/led_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/pedal_app.cpp
    ${CMAKE_SOURCE_DIR}/src/ble_pedal_app.cpp
    ${CMAKE_SOURCE_DIR}/test/fakes/arduino_shim.cpp
    ${CMAKE_SOURCE_DIR}/src/host/src/host_config.cpp
    ${CMAKE_SOURCE_DIR}/src/host/src/host_logger.cpp
    ${CMAKE_SOURCE_DIR}/src/host/src/host_file_system.cpp
)

add_executable(pedal_tests
    unit/test_event_dispatcher.cpp
    unit/test_profile_manager.cpp
    unit/test_send.cpp
    unit/test_button.cpp
    unit/test_config_loader.cpp
    unit/test_action_parsing.cpp
    unit/test_non_keyboard_actions.cpp
    unit/test_pin_action.cpp
    unit/test_independent_actions.cpp
    unit/test_ble_config_service.cpp
    unit/test_hardware_mismatch.cpp
    unit/test_macro_action.cpp
    unit/test_action_json_roundtrip.cpp
    unit/test_host_pedal_app.cpp
    unit/test_app_integration.cpp
    test_config_loader/test_main.cpp
    ${PROD_SOURCES}
)

target_include_directories(pedal_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/PedalLogic/include
    ${CMAKE_SOURCE_DIR}/src/esp32/include
    ${CMAKE_SOURCE_DIR}/src/host/include
    ${CMAKE_SOURCE_DIR}/test/mocks
    ${CMAKE_SOURCE_DIR}/test/fakes
    ${arduinojson_SOURCE_DIR}/src
)

target_link_libraries(pedal_tests GTest::gtest_main GTest::gmock)

include(GoogleTest)
gtest_discover_tests(pedal_tests
    XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-results
)
