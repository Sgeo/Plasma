include_directories(../../CoreLib)
include_directories(../../FeatureLib)
include_directories(../../NucleusLib)
include_directories(../../NucleusLib/inc)
include_directories(../../PubUtilLib)

if(LibOVR_FOUND)
    include_directories(${LibOVR_INCLUDE_DIR})
endif(LibOVR_FOUND)  

if(WIN32)
    add_definitions(-DWIN32)
endif(WIN32)

set(pfOculusRift_SOURCES
    plRiftCamera.cpp
)

set(pfOculusRift_HEADERS
    plRiftCamera.h
    pfRiftCreatable.h
)

add_library(pfOculusRift STATIC ${pfOculusRift_SOURCES} ${pfOculusRift_HEADERS})

source_group("Source Files" FILES ${pfOculusRift_SOURCES})
source_group("Header Files" FILES ${pfOculusRift_HEADERS})
