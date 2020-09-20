cmake_minimum_required(VERSION 3.3)

project(RevilMax VERSION 1.5)

set (RevilLibLibraryPath ../RevilLib)

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
		gdiplus bmm core RevilLib flt mesh maxutil maxscrpt paramblk2 geom MaxSDKTarget
	INCLUDES
		3rd_party/RevilLib/include
	AUTHOR "Lukas Cone"
	DESCR "3DS Max Plugin for formats used by RE Engine/MTF"
	NAME "RevilLib"
	START_YEAR 2019
	PROPERTIES
		SUFFIX .dlu
		${MaxProperties}
)

set_precore_sources(directory_scanner)
