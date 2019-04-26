set(INCLUDE_DIRS             
    ${INCLUDE_DIRS}          
    #${LIBUDEV_INCLUDE_DIRS}  
    PARENT_SCOPE         
)
                     
set(LIBS                     
    ${LIBS}                  
    #${LIBUDEV_LDFLAGS}       
    PARENT_SCOPE         
)                  

set(SOURCE
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/videoLeapMotion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/videoLeapMotion.c
    PARENT_SCOPE
)

set(DEFINES
    ${DEFINES}
    ARVIDEO_INPUT_LEAPMOTION
    PARENT_SCOPE
)
