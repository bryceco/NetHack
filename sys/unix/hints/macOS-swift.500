# NetHack 5.0  macOS.500 $NHDT-Date: 1693359574 2023/08/30 01:39:34 $  $NHDT-Branch: keni-crashweb2 $:$NHDT-Revision: 1.138 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2015.
# NetHack may be freely redistributed.  See license for details.
#
#---------------------------------------------------------------------
# MacOS hints file with support for swift windowport
# Tested on:
#     - MacOS Tahoe 26.5 (Apple Silicon)
#
#-PRE xxxx
# macOS hints file
#

WHICH_OS=macOS
HINTSVERSION := 500

WANT_DEFAULT=swift

ifdef MAKEFILE_TOP
LUA_VERSION=5.4.8
ifeq ($(MAKELEVEL),0)
PRECHECK+=checkmakefiles
# all files included from this hints file get listed
# in HINTSINCLNAMES (without suffix and without a path)
HINTSINCLNAMES := dirs-perms \
		compiler cross-pre1 cross-pre2 cross-post \
		gbdates-pre gbdates-post \
		multiw-1 multiw-2 misc \
		multisnd1-pre multisnd2-pre multisnd-post \
		response
HINTSINCLFILES := $(addsuffix .$(HINTSVERSION), $(HINTSINCLNAMES))
endif
endif

# note: '#-INCLUDE' is not just a comment; multiw-1 contains sections 1 to 2
#-INCLUDE multiw-1.500

# note: '#-INCLUDE' is not just a comment
#        it pulls in the contents of dirs-perms.500
#-INCLUDE dirs-perms.500

# 4. Other

#-----------------------------------------------------------------------------
# You shouldn't need to change anything below here (in the hints file; if
# you're reading this in Makefile augmented by hints, that may not be true).
#

#-INCLUDE cross-pre1.500

#-INCLUDE multiw-2.500

# compiler.500 contains compiler detection and adjustments common
# to both linux and macOS

#-INCLUDE compiler.500

# misc.500 must come after compiler.500

#-INCLUDE misc.500

ifeq "$(USE_ASAN)" "1"
CFLAGS +=-fsanitize=address
LFLAGS +=-fsanitize=address
endif

# NetHack sources control
NHCFLAGS+=-DDLB
NHCFLAGS+=-DHACKDIR=\"$(HACKDIR)\"
NHCFLAGS+=-DDEFAULT_WINDOW_SYS=\"$(WANT_DEFAULT)\"

# SYSCF is deliberately not set here.  config.h defines both SYSCF and
# SYSCF_FILE ("sysconf") together inside a single '#ifndef SYSCF' guard, so
# defining SYSCF alone would suppress the SYSCF_FILE default rather than just
# duplicating it -- they must be set together or not at all.  The defaults are
# what we want: sysconf is read before the path machinery is initialised (it
# can set paths itself) so it never goes through fqname(), and the bare name
# resolves against the working directory, which the app sets before startup.
# An absolute path would be wrong anyway, as the bundle location is not
# knowable at build time.

# use a timer for the delay while performing animations
NHCFLAGS+=-DTIMED_DELAY

NHCFLAGS+=-DDUMPLOG
#NHCFLAGS+=-DCONFIG_ERROR_SECURE=FALSE
NHCFLAGS+=-DGREPPATH=\"/usr/bin/grep\"
NHCFLAGS+=-DNOMAIL
#NHCFLAGS+=-DEXTRA_SANITY_CHECKS
#NHCFLAGS+=-DEDIT_GETLIN
#NHCFLAGS+=-DSCORE_ON_BOTL
#NHCFLAGS+=-DMSGHANDLER
#NHCFLAGS+=-DTTY_TILES_ESCCODES
#NHCFLAGS+=-DTTY_SOUND_ESCCODES
#NHCFLAGS+=-DNO_CHRONICLE
#NHCFLAGS+=-DLIVELOG

# not NHCFLAGS - needed for makedefs
CFLAGS+=-DCRASHREPORT=\"/usr/bin/open\"

# No curses support: this build produces libnh.a for the Swift windowport
# only (WANT_LIBNH), with -DNOTTYGRAPHICS.  The upstream ncurses/pkg-config/
# homebrew/macports probing has been removed along with the tty, X11 and Qt
# windowports -- WINLIB and LFLAGS only matter for linking the game binary,
# and $(GAME) is overridden empty below.

ifdef WANT_MACSOUND
HAVE_SNDLIB = 1
NEEDS_SND_USERSOUNDS = 1
NEEDS_SND_SEAUTOMAP = 1
NEEDS_WAV = 1
SNDCFLAGS+= -DSND_LIB_MACSOUND
SNDLIBSRC = ../sound/macsound/macsound.m
SNDLIBOBJ = macsound.o
LFLAGS += -framework AppKit
#ifdef WANT_SPEECH
LFLAGS += -framework AVFoundation
#endif
endif

ifdef MAKEFILE_SRC
ifndef NO_NHUUID
# macOS doesn't need any extra prerequisite libraries
# so do this unconditionally, unless NO_NHUUID=1 is
# passed on the Make command line.
WANT_NHUUID=1
endif  #NO_NHUUID

# NHUUID: macuuid.m wraps NSUUID to provide get_macos_uuid(); the engine wraps
# that as get_nhuuid()/free_nhuuid() and exposes wiz_show_nhuuid() as a
# wizard-mode command.  Kept because it costs four lines and Foundation is
# already linked.
#
# Safe for libnh.a: Makefile.src lists $(UUIDOBJ) as a direct member of $(HOBJ),
# a sibling of $(SYSOBJ) and $(WINOBJ) rather than a member of either, so the
# `filter-out $(SYSOBJ) $(WINOBJ)` in the libnh.a rule below leaves macuuid.o in
# the archive.
#
# The -framework Foundation here is inert for this build -- LFLAGS only applies
# to linking the game binary, and GAME is overridden empty.  libnh.a therefore
# ships with _OBJC_CLASS_$_NSUUID and the objc_msgSend stubs unresolved; the
# Xcode target must link Foundation or the app will fail to link.
#
# Note UUIDOBJ lacks $(TARGETPFX) while the build rule below applies it; these
# coincide only when TARGETPFX is empty (native builds).  That rule also uses
# $(CC)/$(CFLAGS) rather than $(TARGET_CC)/$(TARGET_CFLAGS).  Both would need
# fixing before cross-compiling (e.g. for iOS or the simulator).
ifdef WANT_NHUUID
CFLAGS += -DNHUUID
UUIDOBJ = macuuid.o
LFLAGS += -framework Foundation
endif
endif  #MAKEFILE_SRC

#
#-INCLUDE multisnd1-pre.500
#

# although not currently doing anything for macOS, this is
# kept in for consistency with layout of linux.500
ifeq "$(CCISCLANG)" "1"
# clang-specific starts
# clang-specific ends
else
# gcc-specific starts
# LIBCFLAGS+=
# gcc-specific ends
endif

# WINCFLAGS set from multiw-2.500
# SNDCFLAGS set from multisnd-pre.500
CFLAGS+= $(WINCFLAGS)
CFLAGS+= $(SNDCFLAGS)
CFLAGS+= $(NHCFLAGS)
CFLAGS+= $(LIBCFLAGS)

# WINCFLAGS set from multiw-2.500
# SNDCFLAGS set from multisnd-pre.500
CCXXFLAGS+= $(WINCFLAGS)
CCXXFLAGS+= $(SNDCFLAGS)
CCXXFLAGS+= $(NHCFLAGS)
CCXXFLAGS+= $(LIBCFLAGS)

VARDATND =
VARDATND0 =

WINOBJ = $(WINOBJ0)
# prevent duplicates in VARDATND if both X11 and Qt are being supported
VARDATND += $(sort $(VARDATND0))

GIT_HASH := $(shell echo `git rev-parse --verify HEAD` 2>&1)
GIT_BRANCH := $(shell echo `git rev-parse --abbrev-ref HEAD` 2>&1)
GIT_PREFIX := $(shell echo `git config nethack.substprefix` 2>&1)

ifdef GIT_HASH
GITHASH = -DNETHACK_GIT_SHA=\"$(GIT_HASH)\"
endif
ifdef GIT_BRANCH
GITBRANCH = -DNETHACK_GIT_BRANCH=\"$(GIT_BRANCH)\"
endif
ifdef GIT_PREFIX
GITPREFIX = -DNETHACK_GIT_PREFIX=\"$(GIT_PREFIX)\"
endif

ifdef WANT_LIBNH
CFLAGS += -DSWIFT_GRAPHICS -DNOTTYGRAPHICS -DNOSHELL -DLIBNH -DTILES_IN_GLYPHMAP

# We need to specify all paths explicitly, because we're in a sandbox
NHCFLAGS += -DNOCWD_ASSUMPTIONS
NHCFLAGS += -DSELF_RECOVER

# we run in a sandbox so we can't call out to gzip
NHCFLAGS += -DZLIB_COMP

# The objects that replace the game's own startup and windowport.  There is no
# LIBNHSYSSRC counterpart: nothing reads it, and upstream's version named
# sys/libnh/unixmain.c, a file that does not exist here.  Upstream supplies a
# separate sys/libnh/libnhmain.c holding a library-entry variant of main(); we
# instead build the normal sys/unix/unixmain.c, which carries a
# `#define main nhmain` so the entry point is callable as a library function.
#
# These are the same five objects Makefile.src lists in $(SYSOBJ), so the
# filter-out in the libnh.a rule below is de-duplication, not substitution.
LIBNHSYSOBJ = $(TARGETPFX)unixmain.o $(TARGETPFX)ioctl.o \
		$(TARGETPFX)unixtty.o $(TARGETPFX)unixunix.o \
		$(TARGETPFX)unixres.o  \
		$(TARGETPFX)winswift.o \
		$(TARGETPFX)tile.o

# Don't build the game executable: the swift windowport has no standalone
# entry point, and unixmain.c's main() is renamed to nhmain() for library use.
override GAME=
MOREALL += ( cd src ; $(MAKE) pregame ; $(MAKE) $(TARGETPFX)libnh.a )
# With $(GAME) empty, the regular `recover: $(GAME)` chain no longer
# triggers lua_support, but recover.c still pulls in hack.h -> nhlua.h
# transitively.  Make recover depend on lua_support explicitly.
ifdef MAKEFILE_TOP
recover: lua_support
endif
endif  # WANT_LIBNH

# Lua
# when building liblua.a, avoid warning that use of tmpnam() should be
# replaced by mkstemp(); the lua code doesn't use nethack's config.h so
# this needs to be passed via make rather than defined in unixconf.h
ifdef GITSUBMODULES
LUAFLAGS=CC='$(CC)' MYCFLAGS=' -DLUA_USE_MACOSX'
ifeq "$(CCISCLANG)" "1"
# clang
LUAFLAGS +=CWARNGCC=''
endif   # clang
override LUAHEADERS = submodules/lua
override LUA2NHTOP = ../..
override LUAMAKEFLAGS=$(LUAFLAGS)
endif   # GITSUBMODULES

# No install step.  Upstream's three install modes (WANT_SHARED_INSTALL,
# WANT_SOURCE_INSTALL, and the private default) are all removed, along with
# PREINSTALL/POSTINSTALL and the SYSCONFCREATE/SYSCONFINSTALL/SYSCONFENSURE
# helpers.  Nothing here runs `make install`: MOREALL builds libnh.a and the
# Xcode project assembles the app around it, shipping its own read-only
# sysconf inside the bundle rather than editing one into HACKDIR.
#
#-INCLUDE cross-pre2.500
#
#
#-INCLUDE gbdates-pre.500
#
#
#-INCLUDE multisnd2-pre.500
#
#
#-INCLUDE response.500
#


#
#-POST

#
#-INCLUDE gbdates-post.500
#
#
#-INCLUDE multisnd-post.500
#

ifdef MAKEFILE_TOP
ifeq ($(MAKELEVEL),0)
.PHONY: checkmakefiles
checkmakefiles:
	@$(MAKE) -f sys/unix/Makefile.check \
		HINTSFILE="$(HINTSFILE)" HINTSINCLFILES="$(HINTSINCLFILES)"
endif
endif

# Native-only: uses $(CC)/$(CFLAGS), not $(TARGET_CC)/$(TARGET_CFLAGS) as the
# winswift.o rule below does.  See the NHUUID notes above.
ifdef WANT_NHUUID
$(TARGETPFX)macuuid.o: ../sys/unix/macuuid.m
	$(CC) $(CFLAGS) -c -o$@ $<
endif

$(TARGETPFX)winswift.o: ../win/swift/winswift.c $(HACK_H) ../win/swift/winswift.h
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ ../win/swift/winswift.c
$(TARGETPFX)swiftglyph.o: ../win/swift/swiftglyph.c $(HACK_H) ../win/swift/winswift.h
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ ../win/swift/swiftglyph.c

ifdef WANT_LIBNH
# Build libnh.a by merging the engine objects, the swift windowport, date.o
# (separate from $(HOBJ) per Makefile.src), hacklib.a, and Lua's static
# archive.  Use libtool -static rather than ar so hacklib.a and
# liblua-$(LUA_VERSION).a have their members merged in -- ar would archive
# them as opaque .a members and macOS ld can't dereference those.
# Depending on $(LUALIB) triggers the lua_support build (which generates
# include/nhlua.h); without it, $(HOBJ) won't compile.
#
# The two filter-out arguments do different jobs:
#   $(SYSOBJ)  lists the same five objects as $(LIBNHSYSOBJ), which is re-added
#              below -- filtering it out just avoids naming them twice.
#   $(WINOBJ)  is the real exclusion.  multiw-2.500 forces WANT_WIN_TTY=1 when
#              no other windowport is requested (WANT_DEFAULT=swift does not
#              satisfy it), so $(WINTTYOBJ) puts getline.o, termcap.o, topl.o
#              and wintty.o into $(HOBJ).  They are compiled -- harmlessly, as
#              -DNOTTYGRAPHICS guts them -- and then dropped here.  Setting
#              WANT_WIN_TTY= would avoid the wasted compiles, but keep the
#              filter regardless so a future windowport can't slip duplicate
#              objects into the archive.
$(TARGETPFX)libnh.a: $(LUALIB) $(HOBJ) $(LIBNHSYSOBJ) $(DATE_O) $(TARGET_HACKLIB)
	libtool -static -o $@ \
		$(filter-out $(SYSOBJ) $(WINOBJ),$(HOBJ)) \
		$(LIBNHSYSOBJ) $(DATE_O) $(TARGET_HACKLIB) $(LUALIB)
	@echo "$@ built."
$(TARGETPFX)tile.o : $(SRCDIR)/tile.c $(HACK_H)
	$(CC) $(CFLAGS) -c -o $@ $(SRCDIR)/tile.c
endif  # WANT_LIBNH

# The upstream 'bundle' and packaging sections are removed.  Both built a
# standalone Terminal-based .app or a signed .pkg/.dmg around the game binary,
# reachable only via `make bundle` / `make build_tty_pkg` / `make build_qt_pkg`,
# and gated on WANT_BUNDLE / WANT_WIN_TTY / WANT_WIN_QT -- none of which this
# build sets.  The Xcode project builds and signs the app around libnh.a, so
# none of it applies.  Removing them also drops $(NHTOP), the CLEANMORE rule
# for ../bundle, and the ABSBUNDLEPATH $(info) that printed on every src parse.

#
#-INCLUDE cross-post.500
#

#
