aux_source_directory(. SRCS)
list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/YUVPlayer.rc)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/bin)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)

add_executable(YUVPlayer ${SRCS})

target_link_libraries(YUVPlayer duilib
	                            yuv
	                            SDL2
	                            SDL2main)

# set startup
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT YUVPlayer)

add_custom_command(TARGET YUVPlayer
                   POST_BUILD
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/copy_file.bat
                   COMMENT "copy file")