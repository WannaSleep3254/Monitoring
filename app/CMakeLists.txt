cmake_minimum_required(VERSION 3.16)

# Qt 찾기 (Widgets 필수)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Qt 자동 처리 (ui/moc/rcc)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 소스 목록
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/mainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/mainWindow.cpp
)

# UI 파일(선택)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gui/mainwindow.ui")
    list(APPEND APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/gui/mainWindow.ui")
endif()

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
    )
endif()

# include 경로 (gui에서 include 편하게)
target_include_directories(MonitoringApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    #${FAIRINO_INCLUDE_DIR} # 일시적으로 core에서 필요한 경로도 추가 (core이외에 app에서도 직접 include할 경우 대비
)

# Qt + core 링크
target_link_libraries(MonitoringApp PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
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
