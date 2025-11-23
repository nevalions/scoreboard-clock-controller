cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(scoreboard_controller)

# Add radio-common as an extra component
set(EXTRA_COMPONENT_DIRS ../radio-common)