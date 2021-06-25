#ifndef entorama_h
#define entorama_h


typedef int (*ENTORAMA_INIT)(struct TayState *);

typedef struct EntoramaModelInfo {
    ENTORAMA_INIT init;
} EntoramaModelInfo;

typedef int (*ENTORAMA_MAIN)(EntoramaModelInfo *);

#endif
