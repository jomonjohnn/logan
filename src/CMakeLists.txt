cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#Add any other src file you may write
set(logan_SRC
    main.cpp
)

#Add any other header files
set(logan_INCLUDES
    include/inputHandler.hpp
)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
	execute_process(
		COMMAND git rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
else()
	set(GIT_BRANCH "Unknown")
endif()
add_definitions(-DGIT_BRANCH=${GIT_BRANCH})
#file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

# Tell CMake to create the logan executable and/or shared libs
add_executable(logan ${logan_SRC})

#If your app, links to an external lib -ie Boost
target_link_libraries( logan ${Boost_LIBRARIES} )
target_link_libraries(logan Threads::Threads)
