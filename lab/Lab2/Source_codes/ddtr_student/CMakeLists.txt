cmake_minimum_required(VERSION 3.16)
project(ddtr LANGUAGES CXX)

add_library(ddtr INTERFACE)
add_library(ddtr::ddtr ALIAS ddtr)
target_compile_features(ddtr INTERFACE cxx_std_17)
include(GNUInstallDirs)
target_include_directories(ddtr INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    file(GLOB examples CONFIGURE_DEPENDS examples/*.cpp)
    foreach(source IN LISTS examples)
        get_filename_component(name ${source} NAME_WE)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE ddtr::ddtr)
    endforeach()

    install(TARGETS ddtr EXPORT ddtrTargets)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT ddtrTargets
        FILE ddtrConfig.cmake
        NAMESPACE ddtr::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/ddtr)
endif()
