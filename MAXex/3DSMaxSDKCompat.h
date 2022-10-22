

#ifndef BUILDVERSION
#pragma message(                                                               \
    "Please add BUILDVERSION=$(Configuration) into preprocessor definitions.")
#endif

//! 3ds Max R12 (2010)
#define MAX_RELEASE_2010 12000
//! 3ds Max R13 (2011)
#define MAX_RELEASE_2011 13000
//! 3ds Max R14 (2012)
#define MAX_RELEASE_2012 14000
//! 3ds Max R15 (2013)
#define MAX_RELEASE_2013 15000
//! 3ds Max R16 (2014)
#define MAX_RELEASE_2014 16000
//! 3ds Max R17 (2015)
#define MAX_RELEASE_2015 17000
//! 3ds Max R18 (2016)
#define MAX_RELEASE_2016 18000
//! 3ds Max R19 (2017)
#define MAX_RELEASE_2017 19000
//! 3ds Max R20 (2018)
#define MAX_RELEASE_2018 20000
//! 3ds Max R21 (2019)
#define MAX_RELEASE_2019 21000
//! 3ds Max R22 (2020)
#define MAX_RELEASE_2020 22000
//! 3ds Max R23 (2021)
#define MAX_RELEASE_2021 23000
//! 3ds Max R24 (2022)
#define MAX_RELEASE_2022 24000

//! 3ds Max R12 (2010) SDK
#define MAX_API_2010 33
//! 3ds Max R13 (2011) SDK
#define MAX_API_2011 35
//! 3ds Max R14 (2012) SDK
#define MAX_API_2012 38
//! 3ds Max R15 (2013) SDK
#define MAX_API_2013 40
//! 3ds Max R16 (2014) SDK
#define MAX_API_2014 42
//! 3ds Max R17 (2015) SDK
#define MAX_API_2015 44
//! 3ds Max R18 (2016) SDK
#define MAX_API_2016 46
//! 3ds Max R19 (2017) SDK
#define MAX_API_2017 48
//! 3ds Max R20 (2018) SDK
#define MAX_API_2018 50
//! 3ds Max R21 (2019) SDK
#define MAX_API_2019 52
//! 3ds Max R22 (2020) SDK
#define MAX_API_2020 55
//! 3ds Max R23 (2021) SDK
#define MAX_API_2021 57
//! 3ds Max R24 (2022) SDK
#define MAX_API_2022 60

#define _MAX_RELEASE_EVAL(x) MAX_RELEASE_##x
#define _MAX_API_EVAL(x) MAX_API_##x
#define MAX_API_EVAL(x) _MAX_API_EVAL(x)
#define MAX_RELEASE_EVAL(x) _MAX_RELEASE_EVAL(x)

#define VERSION_3DSMAX_E(Version)                                              \
  ((MAX_RELEASE_EVAL(Version) << 16) + (MAX_API_EVAL(Version) << 8))
#define VERSION_3DSMAX_B VERSION_3DSMAX_E(BUILDVERSION)

#if VERSION_3DSMAX_B <= VERSION_3DSMAX_E(2014)
#pragma warning(disable : 4996)
#endif

#if VERSION_3DSMAX_B <= VERSION_3DSMAX_E(2012)
#define p_end end
#endif

#if VERSION_3DSMAX_B <= VERSION_3DSMAX_E(2010)
#include "max.h"
#endif

#if VERSION_3DSMAX_B <= VERSION_3DSMAX_E(2012)
#define ToBoneName(_tstring) const_cast<TCHAR *>(_tstring.c_str())
#else
#define ToBoneName(_tstring) _tstring.c_str()
#endif
