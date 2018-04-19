# -*- Mode: CMake -*-
# 
# CMake build file for the 'libIO' library
# 

# Production specification for libIO
#
add_library(IOBase SHARED 
            CDRStreamable.cc
            Channel.cc
            ControlMessage.cc
            Decoder.cc
            GatherWriter.cc
            Growl.cc
            IOTask.cc
            LineBuffer.cc
            MessageManager.cc
            Module.cc
            ParametersChangeRequest.cc
            Preamble.cc
            ProcessingState.cc
            ProcessingStateChangeRequest.cc
            Readers.cc
            RecipientList.cc
            RecordingStateChangeRequest.cc
            RecordIndex.cc
            StateEmitter.cc
            Stats.cc
            StatusBase.cc
            StatusEmitterBase.cc
            Stream.cc
            StreamStatus.cc
            Task.cc
            TaskStatus.cc
            TimeIndex.cc
            Writers.cc
            ZeroconfRegistry.cc
            )

set_target_properties(IOBase PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})
target_link_libraries(IOBase MessagesBase Parameter Zeroconf)

# Production specification for libIO
#
add_tested_library(IO
                   SOURCES
                   ClientSocketReaderTask.cc
                   ClientSocketWriterTask.cc
                   DataPublisher.cc
                   DataSubscriber.cc
                   FileReaderTask.cc
                   FileWriterTask.cc
                   IndexMaker.cc
                   MulticastVMEReaderTask.cc
                   MulticastDataPublisher.cc
                   MulticastDataSubscriber.cc
                   ServerSocketReaderTask.cc
                   ServerSocketWriterTask.cc
                   TCPConnector.cc
                   TCPDataPublisher.cc
                   TCPDataSubscriber.cc
                   TCPInputHandler.cc
                   TSPIReaderTask.cc
                   UDPSocketReaderTask.cc
                   UDPSocketWriterTask.cc
                   VMEReaderTask.cc

                   DEPS IOBase Messages Configuration ${CMAKE_THREAD_LIBS_INIT}
                   
                   TEST ControlMessageTests.cc
                   TEST FileModuleTests.cc
                   TEST FileTaskTests.cc
                   TEST GrowlTests.cc
                   TEST IOTests.cc
                   TEST LineBufferTests.cc
                   TEST MessageManagerTests.cc
                   TEST PubSubTests.cc
                   TEST RecordIndexTests.cc
                   # TEST SocketModuleTests.cc
                   TEST TimeIndexTests.cc
            )

install(TARGETS IOBase IO LIBRARY DESTINATION lib)

# Build some utilities that work with SideCar files
#
set(CMDS converter
    extinfo
    gradients
    mcast
    poser
    prichunker
    priinfo
    rawpriinfo
    rings
    sccut
    sc2xml
    sines
    truthgen
    xml2sc
    )

foreach(CMD ${CMDS})
    add_executable(${CMD} ${CMD}.cc)
    target_link_libraries(${CMD} IO)
endforeach(CMD ${CMDS})

install(TARGETS ${CMDS} RUNTIME DESTINATION bin)
