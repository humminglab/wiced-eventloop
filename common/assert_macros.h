#pragma once

#define SHORT_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __DISP_FILE__ SHORT_FILE

#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG
#define debug_print_assert(A,B,C,D,E,F, ...) wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "[%s:%s:%4d] **ASSERT** %s""\r\n", D ? D : "", F, E, C ? C : "", ##__VA_ARGS__)
#else
#define debug_print_assert(A,B,C,D,E,F, ...)
#endif

#if !defined(unlikely)
#define unlikely(EXPRESSSION)     __builtin_expect(!!(EXPRESSSION), 0)
#endif

#if !defined(check)
#ifdef DEBUG
#define check(X)							\
	do {								\
                if (unlikely(!(X))) {					\
			debug_print_assert( 0, #X, NULL, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
                }							\
	} while (0)
#else
#define check(X)
#endif
#endif

#if !defined(require)
#define require(X, LABEL)						\
        do {								\
		if (unlikely(!(X))) {					\
			debug_print_assert( 0, #X, NULL, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if (!defined(require_string))
#define require_string(X, LABEL, STR)					\
        do {								\
		if (unlikely(!(X))) {					\
			debug_print_assert(0, #X, STR, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__);	\
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_quiet)
#define require_quiet(X, LABEL)			\
        do {					\
		if (unlikely(!(X))) {		\
			goto LABEL;		\
		}				\
        } while (0)
#endif

#if !defined(require_noerr)
#define require_noerr(ERR, LABEL)					\
        do {								\
		wiced_result_t localErr = ERR;				\
		if (unlikely((localErr) != WICED_SUCCESS)) {		\
			debug_print_assert(localErr, NULL, NULL, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_noerr_string)
#define require_noerr_string(ERR, LABEL, STR)				\
        do {								\
		wiced_result_t localErr = ERR;				\
		if (unlikely((localErr) != WICED_SUCCESS)) {		\
			debug_print_assert(localErr, NULL, STR, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_noerr_action_string)
#define require_noerr_action_string(ERR, LABEL, ACTION, STR)		\
        do {								\
		wiced_result_t localErr = ERR;				\
		if (unlikely((localErr) != WICED_SUCCESS)) {			\
			debug_print_assert(localErr, NULL, STR, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			{ ACTION; }					\
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_noerr_quiet)
#define require_noerr_quiet(ERR, LABEL)				\
        do {							\
		if (unlikely((ERR) != WICED_SUCCESS)) {		\
			goto LABEL;				\
		}						\
        } while (0)
#endif

#if !defined(require_noerr_action)
#define require_noerr_action(ERR, LABEL, ACTION)			\
        do {								\
		wiced_result_t localErr = ERR;				\
w		if (unlikely((localErr) != WICED_SUCCESS)) {		\
			debug_print_assert(localErr, NULL, NULL, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			{ ACTION; }					\
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_noerr_action_quiet)
#define require_noerr_action_quiet(ERR, LABEL, ACTION)			\
        do {								\
		wiced_result_t localErr = ERR;				\
		if (unlikely((localErr) != WICED_SUCCESS)) {		\
			{ ACTION; }					\
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_action)
#define require_action(X, LABEL, ACTION)				\
        do {								\
		if (unlikely(!(X))) {					\
			debug_print_assert(0, #X, NULL, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			{ ACTION; }					\
			goto LABEL;					\
		}							\
        } while (0)
#endif

#if !defined(require_action_string)
#define require_action_string(X, LABEL, ACTION, STR)			\
        do {								\
		if (unlikely(!(X))) {					\
			debug_print_assert(0, #X, STR, __DISP_FILE__, __LINE__, __PRETTY_FUNCTION__); \
			{ ACTION; }					\
			goto LABEL;					\
		}							\
        } while (0)
#endif

#define check_ptr_overlap(P1, P1_SIZE, P2, P2_SIZE)			\
	do {								\
		check(!(((uintptr_t)(P1) >=     (uintptr_t)(P2)) &&	\
			((uintptr_t)(P1) <  (((uintptr_t)(P2)) + (P2_SIZE))))); \
		check(!(((uintptr_t)(P2) >=     (uintptr_t)(P1)) &&	\
			((uintptr_t)(P2) <  (((uintptr_t)(P1)) + (P1_SIZE))))); \
									\
	} while (0)
