cmake_minimum_required(VERSION 3.20)
project(def-type LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakePackageConfigHelpers)

option(DEF_TYPE_ENABLE_PFR "Enable PFR backend for automatic reflection" ON)

find_package(fmt CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(nameof CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if(DEF_TYPE_ENABLE_PFR)
    find_package(pfr_non_boost CONFIG REQUIRED)
endif()

add_library(def-type INTERFACE)

target_include_directories(def-type INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/def_type/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(def-type INTERFACE
    fmt::fmt
    unordered_dense::unordered_dense
    magic_enum::magic_enum
    nameof::nameof
    nlohmann_json::nlohmann_json
)

if(DEF_TYPE_ENABLE_PFR)
    target_link_libraries(def-type INTERFACE pfr_non_boost::pfr_non_boost)
    target_compile_definitions(def-type INTERFACE DEF_TYPE_HAS_PFR)
endif()

install(TARGETS def-type EXPORT def-type-targets)
install(DIRECTORY lib/def_type/include/ DESTINATION include)
install(EXPORT def-type-targets
    FILE def-type-targets.cmake
    NAMESPACE def_type::
    DESTINATION share/def-type
)

configure_package_config_file(
    cmake/def-type-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/def-type-config.cmake"
    INSTALL_DESTINATION share/def-type
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/def-type-config.cmake"
    DESTINATION share/def-type
)
