cmake_minimum_required(VERSION 3.0.0)
project(logan CXX)
include(CheckCXXCompilerFlag)

#Let's build shared libs (*.so)
OPTION (BUILD_SHARED_LIBS "Build shared libraries." ON)

#Add any other header files
include_directories( ${CMAKE_SOURCE_DIR}/include )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/gitrev/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

#Detect compiler and act accordingly
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    option(CMAKE_CXX_VERBOSE "Verbose compilation output?" OFF)
    option(CMAKE_LD_VERBOSE "Verbose linking output?" OFF)

    if(CMAKE_CXX_VERBOSE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --verbose")
    endif()
    if(CMAKE_LD_VERBOSE)
        set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--trace -Wl,--threads -Wl,--stats ${CMAKE_EXE_LINKER_FLAGS}")
    endif()
    check_cxx_compiler_flag(-Wall HAVE_WALL)
    if (HAVE_WALL)
        #enable all warnings
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    endif()

    #First check for C++17; if not check for C++14 as fallback
    check_cxx_compiler_flag(-std=c++17 COMPILER_SUPPORTS_CXX17)
    if(COMPILER_SUPPORTS_CXX17)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    elseif(NOT COMPILER_SUPPORTS_CXX17)
        check_cxx_compiler_flag(-std=c++14 COMPILER_SUPPORTS_CXX14)
        if(COMPILER_SUPPORTS_CXX14)
              set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
        endif()
    else()
      message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 or 14 support. Please use a different C++ compiler.")
    endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
      # using Intel C++
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      # using Visual Studio C++
endif()

#find_package(package_name version REQUIRED)
find_package( Boost 1.58 COMPONENTS system filesystem program_options REQUIRED )
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories( ${Boost_INCLUDE_DIR} )
add_subdirectory(src)
