cmake_minimum_required(VERSION 3.3)

project(RevilMax VERSION 1.3)

set (RevilLibLibraryPath ../RevilLib_${CMAKE_GENERATOR_PLATFORM})

add_subdirectory(3rd_party/RevilLib ${RevilLibLibraryPath})
include(${PRECORE_SOURCE_DIR}/cmake/3dsmax.cmake)

build_target(
	TYPE SHARED
	SOURCES
		src/MTFImport.cpp
		src/REEngineImport.cpp
		src/RevilMax.cpp
		src/DllEntry.cpp
		src/RevilMax.def
		src/RevilMax.rc
		${MAX_EX_DIR}/win/About.rc
	LINKS
		gdiplus bmm core RevilLib flt mesh maxutil maxscrpt paramblk2 geom
	DEFINITIONS
		${MaxDefinitions}
	INCLUDES
		${MaxSDK}/include
		3rd_party/RevilLib/include
		${PRECORE_SOURCE_DIR}
	LINK_DIRS
		${MaxSDKLibrariesPath}
	AUTHOR "Lukas Cone"
	DESCR "3DS Max Plugin for formats used by RE engines"
	NAME "RevilLib"
	START_YEAR 2019
	PROPERTIES
		SUFFIX .dlu
		${MaxProperties}
)