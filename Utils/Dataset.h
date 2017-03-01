#ifndef DATASET_H_
#define DATASET_H_

namespace Dataset {

#if (defined PREFIX) || (defined SUFFIX_TEXT) || (defined SUFFIX_OBJ) || (defined SUFFIX_COLOR) || (defined CONSTRUCT_PATH_TEXT) || (defined CONSTRUCT_PATH_OBJ) || (defined CONSTRUCT_PATH_COLOR) \
    || (defined SUFFIX_COLOR64) || (defined CONSTRUCT_PATH_COLOR64)
# error "names used in Dataset.h is redefined"
#endif

#define PREFIX "Dataset/"
#define SUFFIX_TEXT ".gr"
#define SUFFIX_CO ".co"
#define SUFFIX_OBJ_CO ".obj.co"
#define SUFFIX_OBJ ".obj.gr"
#define SUFFIX_COLOR ".obj.color"
#define SUFFIX_COLOR16 ".obj.color16"
#define SUFFIX_COLORK ".obj.colork"
#define SUFFIX_COLOR180 ".obj.color180"
#define SUFFIX_COLOR3500 ".obj.color3500"
#define SUFFIX_COLOR7100 ".obj.color7100"
#define SUFFIX_COLOR64 ".obj.color64"

#define CONSTRUCT_PATH_TEXT(NAME) (PREFIX NAME SUFFIX_TEXT)
#define CONSTRUCT_PATH_OBJ(NAME) (PREFIX NAME SUFFIX_OBJ)
#define CONSTRUCT_PATH_CO(NAME) (PREFIX NAME SUFFIX_CO)
#define CONSTRUCT_PATH_OBJ_CO(NAME) (PREFIX NAME SUFFIX_OBJ_CO)
#define CONSTRUCT_PATH_COLOR(NAME) (PREFIX NAME SUFFIX_COLOR)
#define CONSTRUCT_PATH_COLOR16(NAME) (PREFIX NAME SUFFIX_COLOR16)
#define CONSTRUCT_PATH_COLOR64(NAME) (PREFIX NAME SUFFIX_COLOR64)
#define CONSTRUCT_PATH_COLORK(NAME) (PREFIX NAME SUFFIX_COLORK)
#define CONSTRUCT_PATH_COLOR180(NAME) (PREFIX NAME SUFFIX_COLOR180)
#define CONSTRUCT_PATH_COLOR3500(NAME) (PREFIX NAME SUFFIX_COLOR3500)
#define CONSTRUCT_PATH_COLOR7100(NAME) (PREFIX NAME SUFFIX_COLOR7100)

#define VAR_FOR_NAME(NAME) \
    const char * const DATA_##NAME##_TEXT = CONSTRUCT_PATH_TEXT("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_OBJ = CONSTRUCT_PATH_OBJ("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_CO = CONSTRUCT_PATH_CO("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_OBJ_CO = CONSTRUCT_PATH_OBJ_CO("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLOR = CONSTRUCT_PATH_COLOR("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLOR16 = CONSTRUCT_PATH_COLOR16("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLORK = CONSTRUCT_PATH_COLORK("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLOR180 = CONSTRUCT_PATH_COLOR180("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLOR64 = CONSTRUCT_PATH_COLOR64("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLOR3500 = CONSTRUCT_PATH_COLOR3500("USA-road-d."#NAME); \
    const char * const DATA_##NAME##_COLOR7100 = CONSTRUCT_PATH_COLOR7100("USA-road-d."#NAME);

    VAR_FOR_NAME(USA)
    VAR_FOR_NAME(CTR)
    VAR_FOR_NAME(W)
    VAR_FOR_NAME(E)
    VAR_FOR_NAME(LKS)
    VAR_FOR_NAME(CAL)
    /*
     const char * const DATA_USA_TEXT = CONSTRUCT_PATH_TEXT("USA-road-d.USA");
     const char * const DATA_USA_OBJ = CONSTRUCT_PATH_OBJ("USA-road-d.USA");
     const char * const DATA_USA_COLOR = CONSTRUCT_PATH_COLOR("USA-road-d.USA");
     */

    const char * const DATA_SMALL_TEXT = CONSTRUCT_PATH_TEXT("smallData");
    const char * const DATA_SMALL2_TEXT = CONSTRUCT_PATH_TEXT("smallData2");

#undef PREFIX
#undef SUFFIX_TEXT
#undef SUFFIX_OBJ
#undef SUFFIX_CO
#undef SUFFIX_COLOR
#undef SUFFIX_COLOR64
#undef CONSTRUCT_PATH_TEXT
#undef CONSTRUCT_PATH_OBJ
#undef CONSTRUCT_PATH_COLOR
#undef CONSTRUCT_PATH_COLOR64

}

#endif
