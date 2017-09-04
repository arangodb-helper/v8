// Copyright 2006-2008 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base/logging.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "src/base/debug/stack_trace.h"
#include "src/base/platform/platform.h"

#ifdef __linux__
#include <syslog.h>
#endif

namespace v8 {
namespace base {

namespace {

void (*g_print_stack_trace)() = nullptr;

void PrettyPrintChar(std::ostream& os, int ch) {
  switch (ch) {
#define CHAR_PRINT_CASE(ch) \
  case ch:                  \
    os << #ch;              \
    break;

    CHAR_PRINT_CASE('\0')
    CHAR_PRINT_CASE('\'')
    CHAR_PRINT_CASE('\\')
    CHAR_PRINT_CASE('\a')
    CHAR_PRINT_CASE('\b')
    CHAR_PRINT_CASE('\f')
    CHAR_PRINT_CASE('\n')
    CHAR_PRINT_CASE('\r')
    CHAR_PRINT_CASE('\t')
    CHAR_PRINT_CASE('\v')
#undef CHAR_PRINT_CASE
    default:
      if (std::isprint(ch)) {
        os << '\'' << ch << '\'';
      } else {
        auto flags = os.flags(std::ios_base::hex);
        os << "\\x" << static_cast<unsigned int>(ch);
        os.flags(flags);
      }
  }
}

}  // namespace

void SetPrintStackTrace(void (*print_stack_trace)()) {
  g_print_stack_trace = print_stack_trace;
}

// Define specialization to pretty print characters (escaping non-printable
// characters) and to print c strings as pointers instead of strings.
#define DEFINE_PRINT_CHECK_OPERAND_CHAR(type)                                \
  template <>                                                                \
  void PrintCheckOperand<type>(std::ostream & os, type ch) {                 \
    PrettyPrintChar(os, ch);                                                 \
  }                                                                          \
  template <>                                                                \
  void PrintCheckOperand<type*>(std::ostream & os, type * cstr) {            \
    os << static_cast<void*>(cstr);                                          \
  }                                                                          \
  template <>                                                                \
  void PrintCheckOperand<const type*>(std::ostream & os, const type* cstr) { \
    os << static_cast<const void*>(cstr);                                    \
  }

DEFINE_PRINT_CHECK_OPERAND_CHAR(char)
DEFINE_PRINT_CHECK_OPERAND_CHAR(signed char)
DEFINE_PRINT_CHECK_OPERAND_CHAR(unsigned char)
#undef DEFINE_PRINT_CHECK_OPERAND_CHAR

// Explicit instantiations for commonly used comparisons.
#define DEFINE_MAKE_CHECK_OP_STRING(type)                           \
  template std::string* MakeCheckOpString<type, type>(type, type,   \
                                                      char const*); \
  template void PrintCheckOperand<type>(std::ostream&, type);
DEFINE_MAKE_CHECK_OP_STRING(int)
DEFINE_MAKE_CHECK_OP_STRING(long)       // NOLINT(runtime/int)
DEFINE_MAKE_CHECK_OP_STRING(long long)  // NOLINT(runtime/int)
DEFINE_MAKE_CHECK_OP_STRING(unsigned int)
DEFINE_MAKE_CHECK_OP_STRING(unsigned long)       // NOLINT(runtime/int)
DEFINE_MAKE_CHECK_OP_STRING(unsigned long long)  // NOLINT(runtime/int)
DEFINE_MAKE_CHECK_OP_STRING(void const*)
#undef DEFINE_MAKE_CHECK_OP_STRING


// Explicit instantiations for floating point checks.
#define DEFINE_CHECK_OP_IMPL(NAME)                                            \
  template std::string* Check##NAME##Impl<float, float>(float lhs, float rhs, \
                                                        char const* msg);     \
  template std::string* Check##NAME##Impl<double, double>(                    \
      double lhs, double rhs, char const* msg);
DEFINE_CHECK_OP_IMPL(EQ)
DEFINE_CHECK_OP_IMPL(NE)
DEFINE_CHECK_OP_IMPL(LE)
DEFINE_CHECK_OP_IMPL(LT)
DEFINE_CHECK_OP_IMPL(GE)
DEFINE_CHECK_OP_IMPL(GT)
#undef DEFINE_CHECK_OP_IMPL

}  // namespace base
}  // namespace v8


// Contains protection against recursive calls (faults while handling faults).
void V8_Fatal(const char* file, int line, const char* format, ...) {
  fflush(stdout);
  fflush(stderr);
  v8::base::OS::PrintError("\n\n#\n# Fatal error in %s, line %d\n# ", file,
                           line);
#ifdef __linux__
  ::syslog(LOG_CRIT, "V8 fatal error in %s:%d", file, line);
#endif

  va_list arguments;
  va_start(arguments, format);

  v8::base::OS::VPrintError(format, arguments);
  va_end(arguments);
  v8::base::OS::PrintError("\n#\n");

  if (v8::base::g_print_stack_trace) v8::base::g_print_stack_trace();

  fflush(stderr);
  v8::base::OS::Abort();
}
