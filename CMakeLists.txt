cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(scoreboard_controller)

# Add radio-common and sport_selector as extra components
set(EXTRA_COMPONENT_DIRS ../radio-common)
