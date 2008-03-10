FIND_PATH(PACKETSTREAM_INCLUDE_DIR packetstream.h /usr/include /usr/local/include)

FIND_LIBRARY(PACKETSTREAM_LIBRARY NAMES packetstream
				  PATH
				  /usr/lib
				  /usr/lib64
				  /usr/lib32
				  /usr/local/lib)

IF (PACKETSTREAM_INCLUDE_DIR AND PACKETSTREAM_LIBRARY)
   SET(PACKETSTREAM_FOUND TRUE)
ENDIF (PACKETSTREAM_INCLUDE_DIR AND PACKETSTREAM_LIBRARY)


IF (PACKETSTREAM_FOUND)
   IF (NOT PACKETSTREAM_FIND_QUIETLY)
      MESSAGE(STATUS "Found packetstream: ${PACKETSTREAM_LIBRARY}")
   ENDIF (NOT PACKETSTREAM_FIND_QUIETLY)
ELSE (PACKETSTREAM_FOUND)
   IF (PACKETSTREAM_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find packetstream")
   ENDIF (PACKETSTREAM_FIND_REQUIRED)
ENDIF (PACKETSTREAM_FOUND)