# A 3ds Max CMAKE Module
# Optional variables
#  MAX_VERSION = set version of 3ds max
#  MaxDirectory = a directory, where "3ds max yyyy" and it's SDK are located
# Output variables
#  MaxSDKTarget = interface target
#  build_morpher() = building a morpher library
#  MaxProperties = target properties
#  MaxSDK = a path to a SDK
#  MaxPlugins = a path to a "3ds max yyyy/plugins" folder

include(targetex)

if (NOT DEFINED MAX_VERSION)
	set (MAX_VERSION 2017)
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_MAX_PLATFORM X64)
else()
    set(_MAX_PLATFORM X86)
endif()

set(MaxSDK $ENV{ADSK_3DSMAX_SDK_${MAX_VERSION}})

if (NOT MaxDirectory)
    set(MaxDirectory $ENV{ADSK_3DSMAX_${_MAX_PLATFORM}_${MAX_VERSION}})
    if (NOT MaxDirectory)
        set(MaxDirectory "C:/Program Files/Autodesk/3ds Max ${MAX_VERSION}")
    else()
        get_filename_component(MaxDirectory ${MaxDirectory} ABSOLUTE)
    endif()
endif()

if(NOT MaxSDK)
    set (MaxSDK "${MaxDirectory} SDK/maxsdk")
endif()

set (MaxPlugins "${MaxDirectory}/plugins")

if (MAX_VERSION LESS 2014)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set (_SDK_libs_path_prefix /x64)
	endif()
else()
	set (_SDK_libs_path_suffix /x64/Release)
endif()

set (MAX_EX_DIR ${CMAKE_SOURCE_DIR}/MAXex/)

if (MAX_VERSION GREATER 2012)
    add_definitions(-D_UNICODE -DUNICODE)
    set(CHAR_TYPE UNICODE)
else()
    set(CHAR_TYPE CHAR)
endif()

if(NOT CHAR_TYPE)
  if(WIN32)
    set(CHAR_TYPE UNICODE)
  else()
    set(CHAR_TYPE CHAR)
  endif()
endif()

if(CHAR_TYPE STREQUAL UNICODE)
  message(STATUS "Compiling with wchar_t.")
  add_definitions(-D_UNICODE -DUNICODE)
else()
  message(STATUS "Compiling with char.")
endif()

if (RELEASEVER EQUAL TRUE)
	set (WPO /GL) #Whole program optimalization
endif()

add_library(MaxSDKTarget INTERFACE)

target_include_directories(MaxSDKTarget INTERFACE ${MaxSDK}/include)
link_directories(${MaxSDK}${_SDK_libs_path_prefix}/lib${_SDK_libs_path_suffix})

target_compile_definitions(MaxSDKTarget INTERFACE
    #Defines
    -D_USRDLL -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -D_WIN32_WINDOWS=0x0601
    -D_WIN32_IE=0x0800 -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE
    -D_SCL_SECURE_NO_DEPRECATE -DISOLATION_AWARE_ENABLED=1 -DBUILDVERSION=${MAX_VERSION} 
    -D_WINDLL

    #Optimalization:
    /Gy #Function level linking (COMDAT)
    /Zc:inline #Cleanup
    /Oi #Intrinsic methods
    /fp:fast #Fast floating point
    /Zc:forScope #For loop conformance
    ${WPO}

    #Checks:
    /GS #Security check
    /sdl

    #Other:
    /Zc:wchar_t #Built-in type
    /Gm- #No minimal rebuild
    /Gd #cldecl
    /errorReport:prompt #Compiler error reporting
    /nologo

    /w34996 /we4706 /we4390 /we4557 /we4546 /we4545 /we4295 /we4310
    /we4130 /we4611 /we4213 /we4121 /we4715 /w34701 /w34265 /wd4244 /wd4018 /wd4819
)

set(MaxProperties
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${MaxPlugins}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${MaxPlugins}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/${CMAKE_GENERATOR_PLATFORM}_${MAX_VERSION}
)

function(build_morpher)
    add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK COMMAND lib 
	    ARGS /def:"${CMAKE_SOURCE_DIR}/MAXex/Morpher${_MAX_PLATFORM}.def" /out:Morpher.lib /machine:${_MAX_PLATFORM})
endfunction()
