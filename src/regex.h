#ifndef H_PROCMAN_REGEX_1181238154
#define H_PROCMAN_REGEX_1181238154

#if HAVE_PCRECPP
#include <pcrecpp.h>

#else // !HAVE_PCRECPP
#warning get pcrecpp !

#include <string>

namespace pcrecpp
{
  struct RE_Options
  {
    RE_Options set_caseless(bool)
    { return *this; }

    RE_Options set_utf8(bool)
    { return *this; }
  };

  struct RE
  {
    RE(std::string, RE_Options)
    { }

    RE(std::string)
    { }

    bool FullMatch(std::string) const
    { return false; }

    bool PartialMatch(std::string) const
    { return false; }

    bool PartialMatch(std::string, std::string*) const
    { return false; }
  };
}
#endif // HAVE_PCRECPP

#endif // H_PROCMAN_REGEX_1181238154
