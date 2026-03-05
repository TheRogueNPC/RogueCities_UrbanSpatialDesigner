cmake_minimum_required(VERSION 3.20)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/.Temp/RogueOpenDRIVE-main/src/" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/rc_opendrive/src")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/.Temp/RogueOpenDRIVE-main/include/" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/rc_opendrive/include/RogueOpenDRIVE")
message(STATUS "Files copied.")
