# RemoteStartServer is a Linux HiL daemon; not built on macOS.
if(UNIX AND NOT APPLE)
    add_subdirectory(RemoteStartServer)
endif()
