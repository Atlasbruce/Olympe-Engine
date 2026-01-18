#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #ifdef MINIZ_EXPORTS
    #define MINIZ_EXPORT __declspec(dllexport)
  #else
    #define MINIZ_EXPORT __declspec(dllimport)
  #endif
#else
  #define MINIZ_EXPORT
#endif