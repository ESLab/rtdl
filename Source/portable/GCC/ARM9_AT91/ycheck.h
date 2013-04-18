/* ycheck.h internal checking header file. */
/* Copyright 2005-2010 IAR Systems AB. */

/* Note that there is no include guard for this header. This is intentional. */

#ifndef _SYSTEM_BUILD
  #pragma system_include
#endif

/* __INTRINSIC
 *
 * Note: Redefined each time ycheck.h is included, i.e. for each
 * system header, to ensure that intrinsic support could be turned off
 * individually for each file.
 */
#ifdef __INTRINSIC
  #undef __INTRINSIC
#endif /* __INTRINSIC */

#ifndef __NO_INTRINSIC
  #define __INTRINSIC __intrinsic
#else
  #define __INTRINSIC
#endif

/* __AEABI_PORTABILITY_INTERNAL_LEVEL
 *
 * Note: Redefined each time ycheck.h is included, i.e. for each
 * system header, to ensure that ABI support could be turned off/on
 * individually for each file.
 *
 * Possible values for this preprocessor symbol:
 *
 * 0 - ABI portability mode is disabled.
 *
 * 1 - ABI portability mode (version 1) is enabled.
 *
 * All other values are reserved for future use.
 */

#ifdef __AEABI_PORTABILITY_INTERNAL_LEVEL
  #undef __AEABI_PORTABILITY_INTERNAL_LEVEL
#endif


#ifdef _AEABI_PORTABILITY_LEVEL

  /* The user can define this in the source code before including
   * a standard header. */
  #define __AEABI_PORTABILITY_INTERNAL_LEVEL _AEABI_PORTABILITY_LEVEL

#elif defined(__AEABI_PORTABILITY_LEVEL)

  /* The user can define this in the source code before including
   * a standard header. */
  #define __AEABI_PORTABILITY_INTERNAL_LEVEL __AEABI_PORTABILITY_LEVEL

#elif defined(__DEFAULT_AEABI_PORTABILITY_LEVEL)

  /* The tool chain defines this if the project has enabled abi
   * portability. */
  #define __AEABI_PORTABILITY_INTERNAL_LEVEL __DEFAULT_AEABI_PORTABILITY_LEVEL

#else
  #define __AEABI_PORTABILITY_INTERNAL_LEVEL 0
#endif

#ifndef _YCHECK
#define _YCHECK

/* A definiton for a function of what effects it has.
   NS  = no_state, i.e. it uses no internal or external state. It may write
         to errno though
   NE  = no_state, no_errno,  i.e. it uses no internal or external state,
         not even writing to errno. 
   NRx = no_read(x), i.e. it doesn't read through pointer parameter x.
   NWx = no_write(x), i.e. it doesn't write through pointer parameter x.
*/

#define __EFF_NS       _Pragma("function_effects = no_state")
#define __EFF_NSNW1    _Pragma("function_effects = no_state, no_write(1)")
#define __EFF_NE       _Pragma("function_effects = no_state, no_errno")
#define __EFF_NENR1 _Pragma("function_effects = no_state, no_errno, no_read(1)")
#define __EFF_NENR1R1 _Pragma("function_effects = no_state, no_errno, no_read(1), returns 1")
#define __EFF_NENR2 _Pragma("function_effects = no_state, no_errno, no_read(2)")
#define __EFF_NENR1NW2 _Pragma("function_effects = no_state, no_errno, no_read(1), no_write(2)")
#define __EFF_NENR1NW2R1 _Pragma("function_effects = no_state, no_errno, no_read(1), no_write(2), returns 1")
#define __EFF_NENW1 _Pragma("function_effects = no_state, no_errno, no_write(1)")
#define __EFF_NENW2 _Pragma("function_effects = no_state, no_errno, no_write(2)")
#define __EFF_NENW2R1 _Pragma("function_effects = no_state, no_errno, no_write(2), returns 1")
#define __EFF_NENW1NW2 _Pragma("function_effects = no_state, no_errno, no_write(1,2)")
#define __EFF_NR1      _Pragma("function_effects = no_read(1)")
#define __EFF_NR1NW2   _Pragma("function_effects = no_read(1), no_write(2)")
#define __EFF_NR1NW3   _Pragma("function_effects = no_read(1), no_write(3)")
#define __EFF_NW1      _Pragma("function_effects = no_write(1)")
#define __EFF_NW2      _Pragma("function_effects = no_write(2)")
#define __EFF_NW1NR2   _Pragma("function_effects = no_write(1), no_read(2)")
#define __EFF_NW1NW2   _Pragma("function_effects = no_write(1,2)")




#define __ATTRIBUTES             __INTRINSIC __nounwind
#define __ATTRIBUTES_CAN_THROW   __INTRINSIC 
#define __ATTRIBUTES_NORETURN    __INTRINSIC __noreturn __nounwind
#define __ATTRIBUTES_NORETURN_CAN_THROW __INTRINSIC __noreturn

#endif /* _YCHECK */
