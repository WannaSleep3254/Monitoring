cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(manual_monitor_test
    manual_monitor_test.cpp
)

target_link_libraries(manual_monitor_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    monitoring_core
)

add_executable(manual_jog_test
    manual_jog_test.cpp
)

target_link_libraries(manual_jog_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    monitoring_core
)

if (WIN32)
    add_custom_command(TARGET manual_monitor_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FAIRINO_LIB_DIR}/fairino.dll"
        $<TARGET_FILE_DIR:manual_monitor_test>
    )

    add_custom_command(TARGET manual_jog_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FAIRINO_LIB_DIR}/fairino.dll"
        $<TARGET_FILE_DIR:manual_jog_test>
    )
endif()