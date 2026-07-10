add_subdirectory("ExternalProcesses")
# The LinuxRemoteMaster sample links LinuxRemoteMasterCore, which is Linux-only.
if(UNIX AND NOT APPLE)
    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        add_subdirectory("LinuxRemoteMaster")
    endif()
endif()
