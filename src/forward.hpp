#pragma once
#if defined(__unix__) || defined(__APPLE__)
  #define FUNCTION_SIG __PRETTY_FUNCTION__
#elif defined(_WIN32) || defined(WIN32)
  #define FUNCTION_SIG __FUNCSIG__
#endif