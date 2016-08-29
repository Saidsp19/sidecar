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
add_library(IO SHARED 
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
            )

set_target_properties(IO PROPERTIES VERSION ${SIDECAR_VERSION} SOVERSION ${SIDECAR_VERSION})
target_link_libraries(IO IOBase Messages ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS IOBase IO LIBRARY DESTINATION lib)

# Build some utilities that work with SideCar files
#
include_directories(${QT_INCLUDES})
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
    target_link_libraries(${CMD} IO ${QT_LIBRARIES})
endforeach(CMD ${CMDS})

install(TARGETS ${CMDS} RUNTIME DESTINATION bin)

# Unit tests for libIO classes
#
add_unit_test(ControlMessageTests.cc IO)
add_unit_test(FileModuleTests.cc IO)
add_unit_test(FileTaskTests.cc IO)
add_unit_test(GrowlTests.cc IO)
add_unit_test(IOTests.cc IO)
add_unit_test(LineBufferTests.cc IO)
add_unit_test(MessageManagerTests.cc IO)
add_unit_test(PubSubTests.cc IO)
add_unit_test(RecordIndexTests.cc IO)
# add_unit_test(SocketModuleTests.cc IO)
add_unit_test(TimeIndexTests.cc IO)
