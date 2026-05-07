cmake_minimum_required(VERSION 3.16)

project(Monitoring VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ✅ 빌드 속도 최적화 (MSVC 병렬 컴파일)
if (MSVC)
    add_compile_options(/MP)
endif()

# ----------------------------
# Fairino SDK (vendor)
# ----------------------------
set(FAIRINO_SDK_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/fairino")
set(FAIRINO_INCLUDE_DIR "${FAIRINO_SDK_ROOT}/include")

# ⚠ 공백 포함 경로는 반드시 "" 사용
set(FAIRINO_LIB_BASE_DIR "${FAIRINO_SDK_ROOT}/lib/vs2019 x86-64")

# 멀티/싱글 config 대응
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if (NOT _isMultiConfig)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
        # 원하면 기본을 Release로:
        # set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
    set(_cfg "${CMAKE_BUILD_TYPE}")
else()
    set(_cfg "$<CONFIG>")
endif()

set(FAIRINO_LIB_DIR "${FAIRINO_LIB_BASE_DIR}/${_cfg}")

message(STATUS "FAIRINO_INCLUDE_DIR = ${FAIRINO_INCLUDE_DIR}")
message(STATUS "FAIRINO_LIB_DIR     = ${FAIRINO_LIB_DIR}")

# ----------------------------
# Subdirectories
# ----------------------------
add_subdirectory(core)
add_subdirectory(app)
