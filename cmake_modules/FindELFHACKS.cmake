FIND_PATH(ELFHACKS_INCLUDE_DIR elfhacks.h /usr/include /usr/local/include)

FIND_LIBRARY(ELFHACKS_LIBRARY NAMES elfhacks
			      PATH
			      /usr/lib
			      /usr/lib32
			      /usr/lib64
			      /usr/local/lib)

IF (ELFHACKS_INCLUDE_DIR AND ELFHACKS_LIBRARY)
   SET(ELFHACKS_FOUND TRUE)
ENDIF (ELFHACKS_INCLUDE_DIR AND ELFHACKS_LIBRARY)


IF (ELFHACKS_FOUND)
   IF (NOT ELFHACKS_FIND_QUIETLY)
      MESSAGE(STATUS "Found elfhacks: ${ELFHACKS_LIBRARY}")
   ENDIF (NOT ELFHACKS_FIND_QUIETLY)
ELSE (ELFHACKS_FOUND)
   IF (ELFHACKS_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find elfhacks")
   ENDIF (ELFHACKS_FIND_REQUIRED)
ENDIF (ELFHACKS_FOUND)