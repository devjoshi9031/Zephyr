# Copyright (c) 2021 Google Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BBRAM_NPCX bbram_npcx.c)
zephyr_library_sources_ifdef(CONFIG_BBRAM_EMUL bbram_emul.c)
