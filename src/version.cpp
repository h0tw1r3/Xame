// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    version.c

    Version string source file for MAME.

***************************************************************************/

#define BARE_BUILD_VERSION "0.168"

extern const char bare_build_version[];
extern const char build_version[];
const char bare_build_version[] = BARE_BUILD_VERSION;
const char build_version[] = BARE_BUILD_VERSION " (" SUBTARGETNAME " " __DATE__")";

// MEWUI version
extern const char mewui_version[];
const char mewui_version[] = BARE_BUILD_VERSION " (" SUBTARGETNAME " " __DATE__")";
