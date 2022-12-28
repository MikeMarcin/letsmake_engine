cmake_minimum_required(VERSION 3.24)

add_compile_options(/W4 /WX /wd4996 /wd5050)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS=0)
add_compile_definitions(_SCL_SECURE_NO_WARNINGS=0)
 
# C++ modules are a little new and wonky
# These are just the necessary incantations to enable
# the latest C++ version and std library modules
function (target_enable_cxx_modules TARGET)
    target_compile_features(${TARGET} PRIVATE cxx_std_23)
	if( MSVC )
		target_compile_options(${TARGET} PRIVATE /experimental:module)
		set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 23)
	endif()
endfunction ()

# Define a C++ project
project(LetsMakeEngine VERSION 1.0
            DESCRIPTION "Let's Make an Engine"
            LANGUAGES CXX)

# Include all of the source files
file(GLOB SOURCES CONFIGURE_DEPENDS *.[hic]pp)

add_subdirectory(engine)
# Build an executable
add_executable(game WIN32 ${SOURCES})
target_enable_cxx_modules(game)

if(MSVC)
   set_target_properties(game PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
endif(MSVC)

target_link_libraries(game PRIVATE engine)
