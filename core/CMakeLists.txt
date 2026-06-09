cmake_minimum_required(VERSION 3.16)

add_library(monitoring_core STATIC)

target_sources(monitoring_core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FairinoMonitorService.cpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/monitoring/FairinoMonitorService.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/monitoring/RobotSnapshot.h
)

target_include_directories(monitoring_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        "${FAIRINO_INCLUDE_DIR}"
)

# ✅ 가장 안정적인 링크 (Ninja+MSVC에서 link_directories보다 안전)
target_link_libraries(monitoring_core PRIVATE
    "${FAIRINO_LIB_DIR}/fairino.lib"
)

# 플랫폼 define
if (WIN32)
    target_compile_definitions(monitoring_core PRIVATE WINDOWS_OPTION=1)
    target_link_libraries(monitoring_core PUBLIC ws2_32)
else()
    target_compile_definitions(monitoring_core PRIVATE LINUX_OPTION=1)
endif()

# ----------------------------
# Optimize (MSVC)
# ----------------------------
if (MSVC)
    # 공통 경고/호환
    target_compile_options(monitoring_core PRIVATE /W4 /wd4251)

    # ✅ Release 계열: 컴파일 최적화 + 함수/데이터 섹션 분리(죽은코드 제거 준비)
    target_compile_options(monitoring_core PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG /GL /Gy /Gw>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Ob2 /DNDEBUG /GL /Gy /Gw>
        $<$<CONFIG:MinSizeRel>:/O1 /Ob2 /DNDEBUG /GL /Gy /Gw>
    )

    # ✅ Debug: 디버깅 우선
    target_compile_options(monitoring_core PRIVATE
        $<$<CONFIG:Debug>:/Od /Zi>
    )

    # ⚠️ 링크 최적화(/LTCG, /OPT:REF, /OPT:ICF)는
    # "최종 실행파일 링크 단계(app)"에서 주는 게 더 확실합니다.
    # core에 넣어도 되지만, 전파/효과가 불명확해질 수 있어 app으로 옮기는 것을 권장.
endif()
