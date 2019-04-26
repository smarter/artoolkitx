set(INCLUDE_DIRS
    ${INCLUDE_DIRS}
    ${LeapSDK_DIR}/../../../include
    PARENT_SCOPE
)

set(LIBS
    ${LIBS}
    LeapSDK::LeapC
    PARENT_SCOPE
)

set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/videoLeapMotion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/videoLeapMotion.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ExampleConnection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ExampleConnection.c
    PARENT_SCOPE
)

set(DEFINES
    ${DEFINES}
    ARVIDEO_INPUT_LEAPMOTION
    PARENT_SCOPE
)
