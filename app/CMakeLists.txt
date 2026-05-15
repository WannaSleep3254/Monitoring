cmake_minimum_required(VERSION 3.16)

# Qt 찾기 (Widgets 필수)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Quick Qml QuickControls2 Charts Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Quick Qml QuickControls2 Charts Sql Network)

# Qt 자동 처리 (ui/moc/rcc)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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
