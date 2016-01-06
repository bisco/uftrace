/*
 * libftrace.h
 */

#define FUNC_ENTER (0)
#define FUNC_EXIT  (1)

void __cyg_profile_func_enter(void *func_address, void *call_site )
__attribute__ ((no_instrument_function));

void __cyg_profile_func_exit(void *func_address, void *call_site)
__attribute__ ((no_instrument_function));

