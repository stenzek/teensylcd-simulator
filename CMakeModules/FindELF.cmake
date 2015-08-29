# - Find Dwarf
# Find the dwarf.h header from elf utils
#
#  DWARF_INCLUDE_DIR - where to find dwarf.h, etc.
#  DWARF_LIBRARIES   - List of libraries when using elf utils.
#  DWARF_FOUND       - True if fdo found.

message(STATUS "Checking availability of ELF development libraries")

INCLUDE(CheckLibraryExists)

if (ELF_INCLUDE_DIR AND ELF_LIBRARY)
	set(DWARF_FIND_QUIETLY TRUE)
endif (ELF_INCLUDE_DIR AND ELF_LIBRARY)

if (DEFINED LIBELF_PATH)
  set (USER_LIBELF_INCLUDE_PATH ${LIBELF_PATH}/include)
  set (USER_LIBELF_LIB_PATH ${LIBELF_PATH})
else ()
  set (USER_LIBELF_INCLUDE_PATH "")
  set (USER_LIBELF_LIB_PATH "")
endif ()

find_path(ELF_INCLUDE_DIR elf.h
  HINTS ${USER_LIBELF_INCLUDE_PATH}
)

find_library(ELF_LIBRARY
  NAMES elf
  HINTS ${USER_LIBELF_LIB_PATH}
  PATH_SUFFIXES lib lib64
)

if (ELF_INCLUDE_DIR AND ELF_LIBRARY)
    set(ELF_FOUND TRUE)
    set(CMAKE_REQUIRED_LIBRARIES ${ELF_LIBRARIES})
endif (ELF_INCLUDE_DIR AND ELF_LIBRARY)

if (ELF_FOUND)
	if (NOT DWARF_FIND_QUIETLY)
		message(STATUS "Found elf.h header: ${ELF_INCLUDE_DIR}")
		message(STATUS "Found libelf library: ${ELF_LIBRARY}")
	endif (NOT DWARF_FIND_QUIETLY)
else (ELF_FOUND)
    if (ELF_FIND_REQUIRED)
		# Check if we are in a Red Hat (RHEL) or Fedora system to tell
		# exactly which packages should be installed. Please send
		# patches for other distributions.
		find_path(FEDORA fedora-release /etc)
		find_path(REDHAT redhat-release /etc)
		if (FEDORA OR REDHAT)
			if (NOT ELF_INCLUDE_DIR)
				message(STATUS "Please install the elfutils-devel package")
			endif (ELF_INCLUDE_DIR)
			if (NOT ELF_LIBRARY)
				message(STATUS "Please install the elfutils-libelf package")
			endif (NOT ELF_LIBRARY)
		else (FEDORA OR REDHAT)
			if (NOT ELF_INCLUDE_DIR)
				message(STATUS "Could NOT find elf include dir")
			endif (NOT ELF_INCLUDE_DIR)
			if (NOT ELF_LIBRARY)
				message(STATUS "Could NOT find libelf library")
			endif (NOT ELF_LIBRARY)
		endif (FEDORA OR REDHAT)
		message(FATAL_ERROR "Could NOT find some ELF libraries, please install the missing packages")
	endif (DWARF_FIND_REQUIRED)
endif (ELF_FOUND)

mark_as_advanced(ELF_INCLUDE_DIR ELF_LIBRARY)
#include_directories(${ELF_INCLUDE_DIR})

message(STATUS "Checking availability of ELF development libraries - done")
