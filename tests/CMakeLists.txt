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

# ----------------------------
# ZeroMQ command dry-run server
# ----------------------------
if(ENABLE_ZEROMQ_TRANSPORT)
    if(MINGW)
        message(FATAL_ERROR "ZeroMQ command dry-run server is not supported for MinGW")
    endif()

    if(NOT MSVC)
        message(FATAL_ERROR "ZeroMQ command dry-run server is currently supported only for MSVC")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "ZeroMQ command dry-run server currently supports x64 only")
    endif()

    if(MSVC_VERSION GREATER_EQUAL 1930)
        set(ZEROMQ_PLATFORM_DIR "msvc2022_x64")
    elseif(MSVC_VERSION GREATER_EQUAL 1920)
        set(ZEROMQ_PLATFORM_DIR "msvc2019_x64")
    else()
        message(FATAL_ERROR "Unsupported MSVC version for bundled ZeroMQ")
    endif()

    set(ZEROMQ_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/zeromq")
    set(ZEROMQ_INCLUDE_DIR "${ZEROMQ_ROOT}/include")
    set(ZEROMQ_LIB_NAME "libzmq-mt-4_3_5.lib")
    set(ZEROMQ_DLL_NAME "libzmq-mt-4_3_5.dll")

    set(ZEROMQ_LIB_PATH "${ZEROMQ_ROOT}/lib/${ZEROMQ_PLATFORM_DIR}/${ZEROMQ_LIB_NAME}")
    set(ZEROMQ_DLL_PATH "${ZEROMQ_ROOT}/bin/${ZEROMQ_PLATFORM_DIR}/${ZEROMQ_DLL_NAME}")

    add_executable(zmq_command_dryrun_server
        zmq_command_dryrun_server.cpp
    )

    target_compile_definitions(zmq_command_dryrun_server PRIVATE
        ENABLE_ZEROMQ_TRANSPORT
    )

    target_include_directories(zmq_command_dryrun_server PRIVATE
        "${ZEROMQ_INCLUDE_DIR}"
    )

    target_link_libraries(zmq_command_dryrun_server PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        "${ZEROMQ_LIB_PATH}"
    )

    add_custom_command(TARGET zmq_command_dryrun_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ZEROMQ_DLL_PATH}"
            "$<TARGET_FILE_DIR:zmq_command_dryrun_server>"
    )
    #----------------------------
    add_executable(zmq_snapshot_dryrun_publisher
        zmq_snapshot_dryrun_publisher.cpp
    )

    target_compile_definitions(zmq_snapshot_dryrun_publisher PRIVATE
        ENABLE_ZEROMQ_TRANSPORT
    )

    target_include_directories(zmq_snapshot_dryrun_publisher PRIVATE
        "${ZEROMQ_INCLUDE_DIR}"
    )

    target_link_libraries(zmq_snapshot_dryrun_publisher PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        "${ZEROMQ_LIB_PATH}"
    )

    add_custom_command(TARGET zmq_snapshot_dryrun_publisher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ZEROMQ_DLL_PATH}"
            "$<TARGET_FILE_DIR:zmq_snapshot_dryrun_publisher>"
    )
endif()


