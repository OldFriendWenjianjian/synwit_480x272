#ifndef SYNWIT_UI_TYPES_H
#define SYNWIT_UI_TYPES_H

#ifdef WIN32
    #if (defined FRAMEWORK_BUILD)
        #define EXPORT_API __declspec(dllexport)
        #define EXPORT_VAR __declspec(dllexport)
    #else
        #define EXPORT_API __declspec(dllimport)
        #define EXPORT_VAR __declspec(dllimport)
    #endif
#else
    #define EXPORT_API
    #define EXPORT_VAR
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long synwit_id_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SYNWIT_UI_TYPES_H*/