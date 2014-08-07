if(LibOVR_INCLUDE_DIR AND LibOVR_LIBRARY AND ATL_INCLUDE_DIR AND ATL_LIBRARY)
    set(LibOVR_FIND_QUIETLY TRUE)
endif()


find_path(LibOVR_INCLUDE_DIR Include/OVR.h
          /usr/local/include
          /usr/include
)

find_path(ATL_INCLUDE_DIR inc/atl71/atlbase.h
          /usr/local/include
          /usr/include
)


find_library(LibOVR_LIBRARY NAMES libOVR
             PATHS /usr/local/lib /usr/lib
)

find_library(LibOVR_DEBUG_LIBRARY NAMES libOVR
             PATHS /usr/local/lib /usr/lib
)

find_library(ATL_LIBRARY NAMES atls
             PATHS /usr/local/lib /usr/lib
)

find_library(ATL_LIBRARY_DEBUG NAMES atlsd
             PATHS /usr/local/lib /usr/lib
)


set(LibOVR_LIBRARY ${LibOVR_LIBRARY})
set(LibOVR_DEBUG_LIBRARY ${LibOVR_DEBUG_LIBRARY})
set(ATL_LIBRARY ${ATL_LIBRARY})
set(ATL_LIBRARY_DEBUG ${ATL_LIBRARY_DEBUG})

set(LibOVR_LIBRARIES 
	${LibOVR_LIBRARY}
	${LibOVR_DEBUG_LIBRARY}
  ${ATL_LIBRARY}
  ${ATL_LIBRARY_DEBUG}
)

if(LibOVR_INCLUDE_DIR AND LibOVR_LIBRARY AND LibOVR_DEBUG_LIBRARY AND ATL_INCLUDE_DIR AND ATL_LIBRARY AND ATL_LIBRARY_DEBUG)
    set(LibOVR_FOUND TRUE)
endif()

if (LibOVR_FOUND)
    if(NOT LibOVR_FIND_QUIETLY)
        message(STATUS "Found LibOVR: ${LibOVR_INCLUDE_DIR}")
    endif()
else()
    if(LibOVR_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find LibOVR")
    endif()
endif()
