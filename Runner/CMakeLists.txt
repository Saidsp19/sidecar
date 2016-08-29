# -*- Mode: CMake -*-
#
# CMake build file for the runner program
# 

include_directories(${QT_INCLUDES})

# Production specification for runner
#
add_executable(runner 
               main.cc 
               App.cc 
               LogCollector.cc 
               RemoteController.cc 
               RunnerStatus.cc
               StatusEmitter.cc
               StreamBuilder.cc)

# Linkage dependencies for runner
#
target_link_libraries(runner Algorithm Configuration ${QT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS runner RUNTIME DESTINATION bin)
