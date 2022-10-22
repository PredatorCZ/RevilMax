cmake_minimum_required(VERSION 3.3)

project(RevilMax VERSION 1.9)

set(CMAKE_CXX_STANDARD 17)

set (RevilLibLibraryPath ../RevilLib)

add_subdirectory(3rd_party/RevilLib ${RevilLibLibraryPath})

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR};${CMAKE_MODULE_PATH}
    CACHE STRING "" FORCE)

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
	INCLUDES
		${MAX_EX_DIR}
	LINKS
		gdiplus bmm core revil-objects flt mesh maxutil maxscrpt paramblk2 geom MaxSDKTarget pugixml-objects
	AUTHOR "Lukas Cone"
	DESCR "3DS Max Plugin for formats used by RE Engine/MTF"
	START_YEAR 2019
	PROPERTIES
		SUFFIX .dlu
		${MaxProperties}
)

set_precore_sources(RevilMax directory_scanner uni)
