#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"

/* disabling OpenCL-specific stuff */
#define global

/* disabling non-MSVC packing */
#ifdef _MSC_VER
#define __attribute__(__ignore__)
#endif

/* MSVC-only packing */
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

#include "agent.h"

/* MSVC-only packing */
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif
