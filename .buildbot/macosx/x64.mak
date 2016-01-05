###########################################################################
#
#   makefile
#
#   Core makefile for building MAME and derivatives
#
###########################################################################



###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################

# REGENIE = 1
# VERBOSE = 1
# NOWERROR = 1

# TARGET = mame
# SUBTARGET = tiny
TOOLS = 1
# TESTS = 1
# OSD = sdl

# USE_BGFX = 1
# NO_OPENGL = 1
# USE_DISPATCH_GL = 0
# DIRECTINPUT = 7
# USE_SDL = 1
# SDL_INI_PATH = .;$HOME/.mame/;ini;
# SDL2_MULTIAPI = 1
# NO_USE_MIDI = 1
# DONT_USE_NETWORK = 1
USE_QTDEBUG = 0
# NO_X11 = 1
# NO_USE_XINPUT = 0
# FORCE_DRC_C_BACKEND = 1

# DEBUG = 1
# PROFILER = 1
# SANITIZE = 1

PTR64 = 1
# BIGENDIAN = 1
# NOASM = 1

OPTIMIZE = 3
# SYMBOLS = 1
# SYMLEVEL = 1
# MAP = 1
# PROFILE = 1
ARCHOPTS = -stdlib=libc++ -std=c++1y
# OPT_FLAGS =
#LDOPTS=
CLANG_VERSION = 3.5.0

USE_SYSTEM_LIB_EXPAT = 1
USE_SYSTEM_LIB_ZLIB = 1
# USE_SYSTEM_LIB_JPEG = 1
# USE_SYSTEM_LIB_FLAC = 1
# USE_SYSTEM_LIB_LUA = 1
# USE_SYSTEM_LIB_SQLITE3 = 1
# USE_SYSTEM_LIB_PORTMIDI = 1
# USE_SYSTEM_LIB_PORTAUDIO = 1

# MESA_INSTALL_ROOT = /opt/mesa
# SDL_INSTALL_ROOT = /usr/local
# SDL_FRAMEWORK_PATH = $(HOME)/Library/Frameworks
SDL_LIBVER = sdl2
USE_LIBSDL = 1
# CYGWIN_BUILD = 1

TARGETOS = macosx
CROSS_BUILD = 1
CROSS_PREFIX = x86_64-apple-darwin11-
# OVERRIDE_CC = cc
# OVERRIDE_CXX = c++
# OVERRIDE_LD = ld

# DEPRECATED = 1
# LTO = 1
# SSE2 = 1
# OPENMP = 1
# CPP11 = 1
# FASTDEBUG = 1

FILTER_DEPS = 1
SEPARATE_BIN = 1
# PYTHON_EXECUTABLE = python3
# SHADOW_CHECK = 1
STRIP_SYMBOLS = 1

# QT_HOME = /usr/lib64/qt48/

# DRIVERS = src/mame/drivers/1942.c,src/mame/drivers/cops.c
