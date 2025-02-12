# makefile for building Lua
# see ../INSTALL for installation instructions
# see ../Makefile and inluaconf.h for further customization

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

CC= gcc
CFLAGS= -O2 -Wall $(MYCFLAGS)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS= -lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

PLATS= aix ansi bsd freebsd generic linux macosx mingw posix solaris

LUA_A=	libinlua.a
CORE_O=	lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o \
	lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o  \
	lundump.o lvm.o lzio.o
LIB_O=	lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o \
	lstrlib.o loadlib.o linit.o

LUA_T=	inlua
LUA_O=	lua.o

LUAC_T=	inluac
LUAC_O=	luac.o print.o

ALL_O= $(CORE_O) $(LIB_O) $(LUA_O) $(LUAC_O)
ALL_T= $(LUA_A) $(LUA_T) $(LUAC_T)
ALL_A= $(LUA_A)

default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

a:	$(ALL_A)

$(LUA_A): $(CORE_O) $(LIB_O)
	$(AR) $@ $(CORE_O) $(LIB_O)	# DLL needs all object files
	$(RANLIB) $@

$(LUA_T): $(LUA_O) $(LUA_A)
	$(CC) -o $@ $(MYLDFLAGS) $(LUA_O) $(LUA_A) $(LIBS)

$(LUAC_T): $(LUAC_O) $(LUA_A)
	$(CC) -o $@ $(MYLDFLAGS) $(LUAC_O) $(LUA_A) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O) ./*.exe ./*.dll

depend:
	@$(CC) $(CFLAGS) -MM l*.c print.c

echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "MYCFLAGS = $(MYCFLAGS)"
	@echo "MYLDFLAGS = $(MYLDFLAGS)"
	@echo "MYLIBS = $(MYLIBS)"

# convenience targets for popular platforms

none:
	@echo "Please choose a platform:"
	@echo "   $(PLATS)"

aix:
	$(MAKE) all CC="xlc" CFLAGS="-O2 -DINLUA_USE_POSIX -DINLUA_USE_DLOPEN" MYLIBS="-ldl" MYLDFLAGS="-brtl -bexpall"

ansi:
	$(MAKE) all MYCFLAGS=-DLUA_ANSI

bsd:
	$(MAKE) all MYCFLAGS="-DINLUA_USE_POSIX -DINLUA_USE_DLOPEN" MYLIBS="-Wl,-E"

freebsd:
	$(MAKE) all MYCFLAGS="-DINLUA_USE_LINUX" MYLIBS="-Wl,-E -lreadline"

generic:
	$(MAKE) all MYCFLAGS=

linux:
	$(MAKE) all MYCFLAGS=-DINLUA_USE_LINUX MYLIBS="-Wl,-E -ldl -lreadline -lhistory -lncurses"

macosx:
	$(MAKE) all MYCFLAGS=-DINLUA_USE_LINUX MYLIBS="-lreadline"
# use this on Mac OS X 10.3-
#	$(MAKE) all MYCFLAGS=-DINLUA_USE_MACOSX

mingw:
	$(MAKE) "LUA_A=inlua51.dll" "LUA_T=inlua.exe" \
	"AR=$(CC) -shared -o" "RANLIB=strip --strip-unneeded" \
	"MYCFLAGS=-DINLUA_BUILD_AS_DLL" "MYLIBS=" "MYLDFLAGS=-s" inlua.exe
	$(MAKE) "LUAC_T=inluac.exe" inluac.exe

posix:
	$(MAKE) all MYCFLAGS=-DINLUA_USE_POSIX

solaris:
	$(MAKE) all MYCFLAGS="-DINLUA_USE_POSIX -DINLUA_USE_DLOPEN" MYLIBS="-ldl"

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) default o a clean depend echo none

# DO NOT DELETE

lapi.o: lapi.c inlua.h inluaconf.h lapi.h lobject.h llimits.h ldebug.h \
  lstate.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h lstring.h ltable.h \
  lundump.h lvm.h
lauxlib.o: lauxlib.c inlua.h inluaconf.h inlauxlib.h
lbaselib.o: lbaselib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
lcode.o: lcode.c inlua.h inluaconf.h lcode.h llex.h lobject.h llimits.h \
  lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h ldo.h lgc.h \
  ltable.h
ldblib.o: ldblib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
ldebug.o: ldebug.c inlua.h inluaconf.h lapi.h lobject.h llimits.h lcode.h \
  llex.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h ldo.h \
  lfunc.h lstring.h lgc.h ltable.h lvm.h
ldo.o: ldo.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h ldo.h lfunc.h lgc.h lopcodes.h lparser.h lstring.h \
  ltable.h lundump.h lvm.h
ldump.o: ldump.c inlua.h inluaconf.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h lundump.h
lfunc.o: lfunc.c inlua.h inluaconf.h lfunc.h lobject.h llimits.h lgc.h lmem.h \
  lstate.h ltm.h lzio.h
lgc.o: lgc.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h ldo.h lfunc.h lgc.h lstring.h ltable.h
linit.o: linit.c inlua.h inluaconf.h inlualib.h inlauxlib.h
liolib.o: liolib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
llex.o: llex.c inlua.h inluaconf.h ldo.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h llex.h lparser.h lstring.h lgc.h ltable.h
lmathlib.o: lmathlib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
lmem.o: lmem.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h ldo.h
loadlib.o: loadlib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
lobject.o: lobject.c inlua.h inluaconf.h ldo.h lobject.h llimits.h lstate.h \
  ltm.h lzio.h lmem.h lstring.h lgc.h lvm.h
lopcodes.o: lopcodes.c lopcodes.h llimits.h inlua.h inluaconf.h
loslib.o: loslib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
lparser.o: lparser.c inlua.h inluaconf.h lcode.h llex.h lobject.h llimits.h \
  lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h ldo.h \
  lfunc.h lstring.h lgc.h ltable.h
lstate.o: lstate.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h llex.h lstring.h ltable.h
lstring.o: lstring.c inlua.h inluaconf.h lmem.h llimits.h lobject.h lstate.h \
  ltm.h lzio.h lstring.h lgc.h
lstrlib.o: lstrlib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
ltable.o: ltable.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h ldo.h lgc.h ltable.h
ltablib.o: ltablib.c inlua.h inluaconf.h inlauxlib.h inlualib.h
ltm.o: ltm.c inlua.h inluaconf.h lobject.h llimits.h lstate.h ltm.h lzio.h \
  lmem.h lstring.h lgc.h ltable.h
lua.o: lua.c inlua.h inluaconf.h inlauxlib.h inlualib.h
luac.o: luac.c inlua.h inluaconf.h inlauxlib.h ldo.h lobject.h llimits.h \
  lstate.h ltm.h lzio.h lmem.h lfunc.h lopcodes.h lstring.h lgc.h \
  lundump.h
lundump.o: lundump.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h \
  llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lstring.h lgc.h lundump.h
lvm.o: lvm.c inlua.h inluaconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h ldo.h lfunc.h lgc.h lopcodes.h lstring.h ltable.h lvm.h
lzio.o: lzio.c inlua.h inluaconf.h llimits.h lmem.h lstate.h lobject.h ltm.h \
  lzio.h
print.o: print.c ldebug.h lstate.h inlua.h inluaconf.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h lopcodes.h lundump.h

# (end of Makefile)
