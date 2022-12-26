cmake_minimum_required(VERSION 3.24)

# C++ modules are a little new and wonky in the tooling
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

# Build the engine module
add_subdirectory(engine)

# Build an executable for the game
add_executable(game ${SOURCES})
target_enable_cxx_modules(game)
# Add engine to the game
target_link_libraries(game PRIVATE engine)
