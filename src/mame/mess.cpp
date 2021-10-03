// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    mess.cpp

    Specific (per target) constants

****************************************************************************/

#include "emu.h"

#define APPNAME                 "UXSS"
#define APPNAME_LOWER           "uxss"
#define CONFIGNAME              "uxss"
#define COPYRIGHT               "Copyright Nicola Salmoria\nand the MAME team\nhttps://github.com/h0tw1r3/uxme"
#define COPYRIGHT_INFO          "Copyright Nicola Salmoria and the MAME team"

const char * emulator_info::get_appname() { return APPNAME;}
const char * emulator_info::get_appname_lower() { return APPNAME_LOWER;}
const char * emulator_info::get_configname() { return CONFIGNAME;}
const char * emulator_info::get_copyright() { return COPYRIGHT;}
const char * emulator_info::get_copyright_info() { return COPYRIGHT_INFO;}
