# IMGUI
cmake_path(SET IMGUI_ROOT NORMALIZE "${CMAKE_SOURCE_DIR}/../external/imgui")

file(GLOB IMGUI_SOURCE CONFIGURE_DEPENDS "${IMGUI_ROOT}/*.cpp")
list(APPEND IMGUI_SOURCE "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp" )
list(APPEND IMGUI_SOURCE "${IMGUI_ROOT}/backends/imgui_impl_dx12.cpp" )

add_library(imgui STATIC ${IMGUI_SOURCE})
target_include_directories(imgui PUBLIC "${IMGUI_ROOT}" "${IMGUI_ROOT}/backends")

# DX12
add_library(dx12 INTERFACE)
target_link_libraries(dx12 INTERFACE d3d12.lib dxgi.lib d3dcompiler.lib)
