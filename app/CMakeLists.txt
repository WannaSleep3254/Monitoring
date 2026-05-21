cmake_minimum_required(VERSION 3.16)

# Qt 찾기 (Widgets 필수)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Quick Qml QuickControls2 Charts Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Quick Qml QuickControls2 Charts Sql Network)

# Qt 자동 처리 (ui/moc/rcc)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ----------------------------
# Optional ZeroMQ Transport
# ----------------------------
option(ENABLE_ZEROMQ_TRANSPORT "Enable ZeroMQ remote transport" ON) #OFF)

# 소스 목록
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
 #   ${CMAKE_CURRENT_SOURCE_DIR}/gui/mainwindow.h
 #   ${CMAKE_CURRENT_SOURCE_DIR}/gui/mainwindow.cpp
 )

set(RUNTIME_SOURCES
    # 로봇 게이트웨이 관련
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/IRobotGateway.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RobotRuntimeTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/MultiChannelRobotGateway.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/MultiChannelRobotGateway.cpp
    # 원격 통신 인터페이스
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/IRemoteTransportClient.h
    # DryRunRemoteTransportClient는 실제 네트워크 통신 없이 로봇 상태를 시뮬레이션하는 테스트용 구현체
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/DryRunRemoteTransportClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/DryRunRemoteTransportClient.cpp
    # ZeroMQRemoteTransportClient는 ZeroMQ를 이용해 실제 로봇과 통신하는 구현체
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/ZeroMqRemoteTransportClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/ZeroMqRemoteTransportClient.cpp
    # 메시지 타입
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteMessageTypes.h
    # 로봇 상태 스냅샷 파싱
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteSnapshotParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteSnapshotParser.cpp
    # 원격 명령 생성 및 응답 파싱
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteCommandBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteCommandBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteCommandResponseParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/RemoteCommandResponseParser.cpp
)

set(IOT_SOURCES
    # iot 관련
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/viewmodel/IotViewModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/viewmodel/IotViewModel.cpp
    #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotDatabase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotDatabase.cpp
    #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotSchema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotSchema.cpp
    #IotThresholdRepository
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotThresholdRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotThresholdRepository.cpp
    #IotHistoryRepository
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotHistoryRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/iot/storage/IotHistoryRepository.cpp
)

list(APPEND APP_SOURCES ${RUNTIME_SOURCES} ${IOT_SOURCES})
# UI 파일(선택)
#if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gui/mainwindow.ui")
#    list(APPEND APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gui/mainwindow.ui")
#endif()

# 리소스 qrc(선택)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.qrc")
    list(APPEND APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.qrc")
endif()

# 실행파일 생성 (Qt6는 qt_add_executable 권장)
if (QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(MonitoringApp
        MANUAL_FINALIZATION
        ${APP_SOURCES}
    )
else()
    add_executable(MonitoringApp
        ${APP_SOURCES}
#        resources/app.qrc
#        qml/RobotPanelPage.qml
    )
endif()

# include 경로 (gui에서 include 편하게)
target_include_directories(MonitoringApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
    #${FAIRINO_INCLUDE_DIR} # 일시적으로 core에서 필요한 경로도 추가 (core이외에 app에서도 직접 include할 경우 대비
)

# Qt + core 링크
target_link_libraries(MonitoringApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    monitoring_core
)

# ----------------------------
# Optional ZeroMQ Transport
# 기본값 OFF
# - Qt 6.x + MSVC2022 x64: 1차 지원
# - Qt 5.15 + MSVC2019 x64: 유지 가능
# - MinGW: 보류, ON 시 차단
# ----------------------------
if(ENABLE_ZEROMQ_TRANSPORT)
    if(MINGW)
        message(FATAL_ERROR "ZeroMQ transport is not supported for MinGW in this project")
    endif()

    if(NOT MSVC)
        message(FATAL_ERROR "Bundled ZeroMQ transport is currently supported only for MSVC")
    endif()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Bundled ZeroMQ transport currently supports x64 only")
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

    if(NOT EXISTS "${ZEROMQ_INCLUDE_DIR}/zmq.h")
        message(FATAL_ERROR "Missing ZeroMQ header: ${ZEROMQ_INCLUDE_DIR}/zmq.h")
    endif()

    if(NOT EXISTS "${ZEROMQ_INCLUDE_DIR}/zmq.hpp")
        message(FATAL_ERROR "Missing cppzmq header: ${ZEROMQ_INCLUDE_DIR}/zmq.hpp")
    endif()

    if(NOT EXISTS "${ZEROMQ_LIB_PATH}")
        message(FATAL_ERROR "Missing ZeroMQ import library: ${ZEROMQ_LIB_PATH}")
    endif()

    if(NOT EXISTS "${ZEROMQ_DLL_PATH}")
        message(FATAL_ERROR "Missing ZeroMQ runtime DLL: ${ZEROMQ_DLL_PATH}")
    endif()

    target_compile_definitions(MonitoringApp PRIVATE
        ENABLE_ZEROMQ_TRANSPORT
    )

    target_include_directories(MonitoringApp PRIVATE
        "${ZEROMQ_INCLUDE_DIR}"
    )

    target_link_libraries(MonitoringApp PRIVATE
        "${ZEROMQ_LIB_PATH}"
    )

    add_custom_command(TARGET MonitoringApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ZEROMQ_DLL_PATH}"
            "$<TARGET_FILE_DIR:MonitoringApp>"
    )

    message(STATUS "ZeroMQ transport enabled")
    message(STATUS "ZeroMQ platform dir: ${ZEROMQ_PLATFORM_DIR}")
    message(STATUS "ZeroMQ include dir: ${ZEROMQ_INCLUDE_DIR}")
    message(STATUS "ZeroMQ lib path: ${ZEROMQ_LIB_PATH}")
    message(STATUS "ZeroMQ dll path: ${ZEROMQ_DLL_PATH}")
endif()

# ----------------------------
# Optimize (MSVC) - 최종 링크 단계에서 효과 큼
# ----------------------------
if (MSVC)
    # Release 계열: 링크 최적화 + LTCG
    target_link_options(MonitoringApp PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/LTCG /OPT:REF /OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()

# Qt6 마무리
if (QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(MonitoringApp)
endif()

# ----------------------------
# Runtime DLL copy (Fairino)
# ----------------------------
if (WIN32)
    add_custom_command(TARGET MonitoringApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FAIRINO_LIB_DIR}/fairino.dll"
        $<TARGET_FILE_DIR:MonitoringApp>
    )
endif()
