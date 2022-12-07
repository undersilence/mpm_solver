#ifdef __unix__
  #define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_WIN32) || defined(WIN32)
  #define FUNC_SIG __FUNCSIG__
#endif