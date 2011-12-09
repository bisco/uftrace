/*
 * libftrace.h
 */

void __cyg_profile_func_enter(void *func_address, void *call_site )
__attribute__ ((no_instrument_function));

void __cyg_profile_func_exit(void *func_address, void *call_site)
__attribute__ ((no_instrument_function));

