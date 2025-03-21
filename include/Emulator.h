#pragma once

#include <unordered_map>

#ifdef EMULATION
#include "nanovg.h"
#include <GL/gl.h>
#define NANOVG_GL3
#include "nanovg_gl.h"
std::unordered_map<NVGcontext *, void *> g_ContextUserPtrMap;

inline NVGcontext *nvgCreateInternal(NVGparams *params) {
  NVGcontext *ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (ctx) {
    g_ContextUserPtrMap[ctx] = (void *)params->userPtr;
  }
  return ctx;
}
inline void *getUserPtr(NVGcontext *ctx) {
  if (const auto it = g_ContextUserPtrMap.find(ctx);
      it != g_ContextUserPtrMap.end()) {
    return it->second;
      }
  return nullptr;
}

#endif