/* This file is generated from target_rename.def by genrename. */

#ifndef TOPPERS_TARGET_RENAME_H
#define TOPPERS_TARGET_RENAME_H


/*
 * target_kernel_impl.c
 */
#define target_initialize			_kernel_target_initialize
#define target_exit					_kernel_target_exit

/*
 * target_timer.c
 */
#define target_hrt_get_current		_kernel_target_hrt_get_current

#define _kernel_exc_tbl             __Vectors
#define _kernel_svc_handler         SVC_Handler
#define _kernel_pendsv_handler      PendSV_Handler
#define target_systick_handler      SysTick_Handler

#ifdef TOPPERS_LABEL_ASM


/*
 * target_kernel_impl.c
 */
#define _target_initialize			__kernel_target_initialize
#define _target_exit				__kernel_target_exit

/*
 * target_timer.c
 */
#define _target_hrt_get_current		__kernel_target_hrt_get_current

#endif /* TOPPERS_LABEL_ASM */

#include "chip_rename.h"

#endif /* TOPPERS_TARGET_RENAME_H */
