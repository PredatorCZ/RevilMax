cmake_minimum_required(VERSION 3.3)

project(RevilMax VERSION 1.8)

set (RevilLibLibraryPath ../RevilLib)

add_subdirectory(3rd_party/RevilLib ${RevilLibLibraryPath})
include(3dsmax)

build_target(
	NAME RevilMax
	TYPE SHARED
	SOURCES
		src/MTFImport.cpp
		src/REEngineImport.cpp
		src/RevilMax.cpp
		src/DllEntry.cpp
		src/RevilMax.rc
		${MAX_EX_DIR}/win/About.rc
	LINKS
		gdiplus bmm core revil-objects flt mesh maxutil maxscrpt paramblk2 geom MaxSDKTarget
	AUTHOR "Lukas Cone"
	DESCR "3DS Max Plugin for formats used by RE Engine/MTF"
	START_YEAR 2019
	PROPERTIES
		SUFFIX .dlu
		${MaxProperties}
)

set_precore_sources(RevilMax directory_scanner)
