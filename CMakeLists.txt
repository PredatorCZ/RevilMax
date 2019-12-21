cmake_minimum_required(VERSION 3.3)

project(RevilMax VERSION 1.2)

set(TARGETEX_LOCATION 3rd_party/RevilLib/3rd_party/PreCore/cmake)
include(${TARGETEX_LOCATION}/3dsmax.cmake)

set (HavokLibLibraryPath ../RevilLib_${CMAKE_GENERATOR_PLATFORM}_${CHAR_TYPE})

add_subdirectory(3rd_party/RevilLib ${RevilLibLibraryPath})

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
		3rd_party/RevilLib/3rd_party/PreCore
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