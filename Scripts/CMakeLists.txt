# -*- CMake -*-
#
# Build / install utility scripts
#
add_scripts(algxml.py
			doinstall.sh
			dotest.sh
			extcsv.py
			install-build.sh
			install-gui.sh
			makeidx.sh
			newalg.py
			process.sh
			startup.sh
)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/bin/Logger.py
     	           COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Logger.py "${PROJECT_BINARY_DIR}/bin"
      	           DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Logger.py)
add_custom_target(Logger-install ALL DEPENDS ${PROJECT_BINARY_DIR}/bin/Logger.py)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/bin/optargs.py
     	           COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/optargs.py "${PROJECT_BINARY_DIR}/bin"
      	           DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/optargs.py)
add_custom_target(optargs-install ALL DEPENDS ${PROJECT_BINARY_DIR}/bin/optargs.py)
