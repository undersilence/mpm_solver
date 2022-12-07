#pragma once
#if defined(__unix__) || defined(__APPLE__)
  #define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_WIN32) || defined(WIN32)
  #define FUNC_SIG __FUNCSIG__
#endif