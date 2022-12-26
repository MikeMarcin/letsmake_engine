file(GLOB SOURCES CONFIGURE_DEPENDS "*.ixx")

add_library(engine STATIC ${SOURCES})
target_enable_cxx_modules(engine)
