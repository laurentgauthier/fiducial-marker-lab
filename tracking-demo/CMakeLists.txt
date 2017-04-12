cmake_minimum_required(VERSION 2.8)
project( tracking-demo )
find_package( OpenCV 3.0 REQUIRED )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -pg")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pg")

include_directories( ${OpenCV_INCLUDE_DIRS} )

file(GLOB source
    "src/*.h"
    "src/*.cpp"
)
add_executable( tracking-demo ${source} )
target_link_libraries( tracking-demo ${OpenCV_LIBS} -lpthread )
