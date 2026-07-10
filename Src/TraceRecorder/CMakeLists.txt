if(UNIX AND NOT APPLE) # LinuxRemoteMasterCore is Linux-only
    target_sources(LinuxRemoteMasterCore PRIVATE
        ReadFromBlackboardPipe.c)
endif()
    
set(CommonFileList
    ReadFromBlackboardPipe.c
    TraceRecorder.c
    TraceWriteFile.c
    TraceWriteMdfFile.c
    TraceWriteMdf4File.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})
