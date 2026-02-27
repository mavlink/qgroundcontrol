target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Bluetooth)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        BluetoothBleWorker.cc
        BluetoothBleWorker.h
        BluetoothClassicWorker.cc
        BluetoothClassicWorker.h
        BluetoothConfiguration.cc
        BluetoothConfiguration.h
        BluetoothLink.cc
        BluetoothLink.h
        BluetoothWorker.cc
        BluetoothWorker.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
