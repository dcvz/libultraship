#=================== TPCircularBuffer ===================

set(TPCIRCULARBUFFER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/TPCircularBuffer)
add_library(TPCircularBuffer STATIC)

target_sources(TPCircularBuffer PRIVATE ${TPCIRCULARBUFFER_DIR}/TPCircularBuffer.c)

target_include_directories(TPCircularBuffer PUBLIC ${TPCIRCULARBUFFER_DIR})

#=================== OpenEmuHelpers ===================

set(OPENEMUHELPERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/OpenEmuHelpers)
add_library(OpenEmuHelpers STATIC)

target_sources(OpenEmuHelpers
    PRIVATE
    ${OPENEMUHELPERS_DIR}/OEAudioUnit.mm
    ${OPENEMUHELPERS_DIR}/OEGameAudio.mm
    ${OPENEMUHELPERS_DIR}/OERingBuffer.m
)

target_include_directories(OpenEmuHelpers PUBLIC ${OPENEMUHELPERS_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extern/spdlog/include)

target_link_libraries(OpenEmuHelpers PUBLIC TPCircularBuffer)

set_property(TARGET OpenEmuHelpers PROPERTY OBJCXX_STANDARD 11)
set_target_properties(OpenEmuHelpers PROPERTIES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES 
)


