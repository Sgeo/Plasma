include_directories(${LibOVR_INCLUDE_DIR})
include_directories(../../CoreLib)
include_directories(../../FeatureLib)
include_directories(../../NucleusLib)
include_directories(../../NucleusLib/inc)
include_directories(../../PubUtilLib)

if(WIN32)
    add_definitions(-DWIN32)
endif(WIN32)

set(plPostPipeline_SOURCES
    plPostPipeline.cpp
)

set(plPostPipeline_HEADERS
	plPostPipeline.h
    plPostPipelineCreatable.h
)

add_library(plPostPipeline STATIC ${plPostPipeline_SOURCES} ${plPostPipeline_HEADERS})

source_group("Source Files" FILES ${plPostPipeline_SOURCES})
source_group("Header Files" FILES ${plPostPipeline_HEADERS})
