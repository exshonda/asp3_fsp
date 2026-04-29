/* This file is generated from target_rename.def by genrename. */

/* This file is included only when target_rename.h has been included. */
#ifdef TOPPERS_TARGET_RENAME_H
#undef TOPPERS_TARGET_RENAME_H


/*
 * target_kernel_impl.c
 */
#undef target_initialize
#undef target_exit

/*
 * target_timer.c
 */
#undef target_hrt_get_current

#ifdef TOPPERS_LABEL_ASM


/*
 * target_kernel_impl.c
 */
#undef _target_initialize
#undef _target_exit

/*
 * target_timer.c
 */
#undef _target_hrt_get_current

#endif /* TOPPERS_LABEL_ASM */

#include "chip_unrename.h"

#endif /* TOPPERS_TARGET_RENAME_H */
