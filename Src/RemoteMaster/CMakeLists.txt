# The RemoteMaster Server (RT/SocketCAN) is Linux-only; not available on macOS.
if(UNIX AND NOT APPLE)
add_subdirectory(Server)
endif()
add_subdirectory(Client)
