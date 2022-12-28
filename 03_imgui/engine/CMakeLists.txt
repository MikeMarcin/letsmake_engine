file(GLOB SOURCES CONFIGURE_DEPENDS "*.ixx")

add_library(engine STATIC ${SOURCES})
target_enable_cxx_modules(engine)

add_subdirectory(external)
target_link_libraries(engine PUBLIC imgui dx12)
