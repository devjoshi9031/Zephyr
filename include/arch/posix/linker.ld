/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the POSIX (native) platform
 */

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>


SECTIONS
 {

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rom-start.ld>

#include <linker/common-rom.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#include <linker/common-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#include <arch/posix/native_tasks.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

 __data_ram_end = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-noinit.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

 } INSERT AFTER .data;

/*
 * Note that the INSERT command actually changes the meaning of the -T command
 * line switch: The script will now augment the default SECTIONS instead of
 * replacing it.
 */
