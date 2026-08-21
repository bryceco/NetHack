/* NetHack 5.0  winswift.c */
/* NetHack may be freely redistributed.  See license for details. */

/* A windowport for a native Swift front end.
 *
 * Unlike win/shim, which funnels every call through one variadic callback
 * and a format string, this port keeps NetHack's typed calling convention
 * and forwards each proc to a matching member of a callback struct.  All
 * NetHack types are translated here so the Swift side never needs hack.h.
 *
 * This file is compiled by NetHack's own makefiles, so it sees exactly the
 * same -D flags as the core.  That is the entire point: struct layouts
 * cannot drift between this file and libnh.a.
 */

#include "hack.h"
#include "func_tab.h"
#include "winswift.h"
#include <string.h>

#ifdef SWIFT_GRAPHICS

/* ------------------------------------------------------------------ */
/* Compile-time checks that winswift.h still agrees with NetHack.      */
/* If one of these fails to compile, fix the constant in winswift.h.   */
/* ------------------------------------------------------------------ */

#define NHSWIFT_CT_ASSERT(nm, cond) typedef char nhswift_ct_##nm[(cond) ? 1 : -1]

NHSWIFT_CT_ASSERT(win_message, NHSWIFT_WIN_MESSAGE == NHW_MESSAGE);
NHSWIFT_CT_ASSERT(win_status,  NHSWIFT_WIN_STATUS  == NHW_STATUS);
NHSWIFT_CT_ASSERT(win_map,     NHSWIFT_WIN_MAP     == NHW_MAP);
NHSWIFT_CT_ASSERT(win_menu,    NHSWIFT_WIN_MENU    == NHW_MENU);
NHSWIFT_CT_ASSERT(win_text,    NHSWIFT_WIN_TEXT    == NHW_TEXT);
NHSWIFT_CT_ASSERT(pick_none,   NHSWIFT_PICK_NONE   == PICK_NONE);
NHSWIFT_CT_ASSERT(pick_one,    NHSWIFT_PICK_ONE    == PICK_ONE);
NHSWIFT_CT_ASSERT(pick_any,    NHSWIFT_PICK_ANY    == PICK_ANY);
NHSWIFT_CT_ASSERT(maxbl,       NHSWIFT_MAXBLSTATS   == MAXBLSTATS);
NHSWIFT_CT_ASSERT(bufsz,       NHSWIFT_BUFSZ       <= BUFSZ);

/* nhswift_menu_item must be binary-compatible with menu_item so that
 * swift_select_menu() can assign the bridge-allocated array directly to
 * *menu_list without copying. */
NHSWIFT_CT_ASSERT(mi_size,  sizeof(nhswift_menu_item) == sizeof(menu_item));
NHSWIFT_CT_ASSERT(mi_ident, offsetof(nhswift_menu_item, identifier) == offsetof(menu_item, item));
NHSWIFT_CT_ASSERT(mi_count, offsetof(nhswift_menu_item, count)      == offsetof(menu_item, count));
NHSWIFT_CT_ASSERT(mi_flags, offsetof(nhswift_menu_item, itemflags)  == offsetof(menu_item, itemflags));

/* ------------------------------------------------------------------ */
/* Callback table                                                      */
/* ------------------------------------------------------------------ */

static nhswift_callbacks cb;

/* call before nhmain(); paths must outlive the process */
void
nhswift_set_paths(const char *hackdir, const char *playground)
{
	int i;

	if (playground) {
		char *p = dupstr(playground);   /* trailing '/' required */

		for (i = 0; i < PREFIX_COUNT; i++)
			gf.fqn_prefix[i] = p;
		/* then override the read-only ones back to hackdir */
	}
	if (hackdir) {
		char *h = dupstr(hackdir);
		gf.fqn_prefix[HACKPREFIX] = h;
		gf.fqn_prefix[DATAPREFIX] = h;
	}
}

void
nhswift_set_callbacks(const nhswift_callbacks *newcb)
{
	if (newcb)
		cb = *newcb;
	else
		(void) memset((genericptr_t) &cb, 0, sizeof cb);
}

/* ------------------------------------------------------------------ */
/* Validity queries — thin wrappers around NetHack's valid* functions. */
/* Safe to call from any thread while the game thread is blocked.      */
/* ------------------------------------------------------------------ */

int
nhswift_validrole(int role)
{
	return (int)validrole(role);
}

int
nhswift_validrace(int role, int race)
{
	return (int)validrace(role, race);
}

int
nhswift_validgend(int role, int race, int gender)
{
	return (int)validgend(role, race, gender);
}

int
nhswift_validalign(int role, int race, int align)
{
	return (int)validalign(role, race, align);
}

// Convert a glyph value (platform independent) to a tile value (platform specific)
int
nhswift_glyph_to_tile(int glyph)
{
	extern glyph_map glyphmap[];
	if (glyph < 0 || glyph >= MAX_GLYPH)
		return -1;
	return (int)glyphmap[glyph].tileidx;
}


/* ------------------------------------------------------------------ */
/* Layout verification                                                 */
/*                                                                     */
/* nhswift_glyph (in winswift.h) must be identical in layout to        */
/* glyph_info (in wintype.h) so that a glyph_info pointer can be      */
/* passed to the Swift layer without copying.  These assertions catch  */
/* any divergence when NetHack's structs are updated.                  */
/* ------------------------------------------------------------------ */

_Static_assert(sizeof(nhswift_glyph) == sizeof(glyph_info),
               "nhswift_glyph vs glyph_info: size mismatch");
_Static_assert(offsetof(nhswift_glyph, glyph) == offsetof(glyph_info, glyph),
               "nhswift_glyph vs glyph_info: .glyph offset mismatch");
_Static_assert(offsetof(nhswift_glyph, gm.glyphflags) == offsetof(glyph_info, gm.glyphflags),
               "nhswift_glyph vs glyph_info: .gm.glyphflags offset mismatch");
_Static_assert(offsetof(nhswift_glyph, gm.tileidx) == offsetof(glyph_info, gm.tileidx),
               "nhswift_glyph vs glyph_info: .gm.tileidx offset mismatch");

/* Verify NHColor values match CLR_* / NO_COLOR in color.h. */
_Static_assert(NHColorBlack         == CLR_BLACK,          "NHColorBlack mismatch");
_Static_assert(NHColorRed           == CLR_RED,            "NHColorRed mismatch");
_Static_assert(NHColorGreen         == CLR_GREEN,          "NHColorGreen mismatch");
_Static_assert(NHColorBrown         == CLR_BROWN,          "NHColorBrown mismatch");
_Static_assert(NHColorBlue          == CLR_BLUE,           "NHColorBlue mismatch");
_Static_assert(NHColorMagenta       == CLR_MAGENTA,        "NHColorMagenta mismatch");
_Static_assert(NHColorCyan          == CLR_CYAN,           "NHColorCyan mismatch");
_Static_assert(NHColorGray          == CLR_GRAY,           "NHColorGray mismatch");
_Static_assert(NHColorNone          == NO_COLOR,           "NHColorNone mismatch");
_Static_assert(NHColorOrange        == CLR_ORANGE,         "NHColorOrange mismatch");
_Static_assert(NHColorBrightGreen   == CLR_BRIGHT_GREEN,   "NHColorBrightGreen mismatch");
_Static_assert(NHColorYellow        == CLR_YELLOW,         "NHColorYellow mismatch");
_Static_assert(NHColorBrightBlue    == CLR_BRIGHT_BLUE,    "NHColorBrightBlue mismatch");
_Static_assert(NHColorBrightMagenta == CLR_BRIGHT_MAGENTA, "NHColorBrightMagenta mismatch");
_Static_assert(NHColorBrightCyan    == CLR_BRIGHT_CYAN,    "NHColorBrightCyan mismatch");
_Static_assert(NHColorWhite         == CLR_WHITE,          "NHColorWhite mismatch");
_Static_assert(NHSWIFT_CLR_MAX      == CLR_MAX,            "NHSWIFT_CLR_MAX mismatch");

/* Verify NHStatusField values match BL_* / MAXBLSTATS in botl.h.
   Cast both sides to int to avoid -Wenum-compare between NHStatusField
   (an ObjC NS_ENUM) and enum statusfields (a plain C enum). */
#define _BLCHECK(a, b) _Static_assert((int)(a) == (int)(b), #a " mismatch")
_BLCHECK(NHStatusFieldCharacteristics, BL_CHARACTERISTICS);
_BLCHECK(NHStatusFieldReset,           BL_RESET);
_BLCHECK(NHStatusFieldFlush,           BL_FLUSH);
_BLCHECK(NHStatusFieldTitle,           BL_TITLE);
_BLCHECK(NHStatusFieldStr,             BL_STR);
_BLCHECK(NHStatusFieldDex,             BL_DX);
_BLCHECK(NHStatusFieldCon,             BL_CO);
_BLCHECK(NHStatusFieldInt,             BL_IN);
_BLCHECK(NHStatusFieldWis,             BL_WI);
_BLCHECK(NHStatusFieldCha,             BL_CH);
_BLCHECK(NHStatusFieldAlign,           BL_ALIGN);
_BLCHECK(NHStatusFieldScore,           BL_SCORE);
_BLCHECK(NHStatusFieldCap,             BL_CAP);
_BLCHECK(NHStatusFieldGold,            BL_GOLD);
_BLCHECK(NHStatusFieldEnergy,          BL_ENE);
_BLCHECK(NHStatusFieldEnergyMax,       BL_ENEMAX);
_BLCHECK(NHStatusFieldXp,              BL_XP);
_BLCHECK(NHStatusFieldAc,              BL_AC);
_BLCHECK(NHStatusFieldHd,              BL_HD);
_BLCHECK(NHStatusFieldTime,            BL_TIME);
_BLCHECK(NHStatusFieldHunger,          BL_HUNGER);
_BLCHECK(NHStatusFieldHp,              BL_HP);
_BLCHECK(NHStatusFieldHpMax,           BL_HPMAX);
_BLCHECK(NHStatusFieldLevelDesc,       BL_LEVELDESC);
_BLCHECK(NHStatusFieldExp,             BL_EXP);
_BLCHECK(NHStatusFieldCondition,       BL_CONDITION);
_BLCHECK(NHStatusFieldWeapon,          BL_WEAPON);
_BLCHECK(NHStatusFieldArmor,           BL_ARMOR);
_BLCHECK(NHStatusFieldTerrain,         BL_TERRAIN);
_BLCHECK(NHStatusFieldVersion,         BL_VERS);
#undef _BLCHECK
_Static_assert(NHSWIFT_MAXBLSTATS           == MAXBLSTATS,         "NHSWIFT_MAXBLSTATS mismatch");

/* ------------------------------------------------------------------ */
/* window_procs implementations                                        */
/* ------------------------------------------------------------------ */

staticfn void swift_init_nhwindows(int *, char **);
staticfn void swift_player_selection(void);
staticfn void swift_askname(void);
staticfn void swift_get_nh_event(void);
staticfn void swift_exit_nhwindows(const char *);
staticfn void swift_suspend_nhwindows(const char *);
staticfn void swift_resume_nhwindows(void);
staticfn winid swift_create_nhwindow(int);
staticfn void swift_clear_nhwindow(winid);
staticfn void swift_display_nhwindow(winid, boolean);
staticfn void swift_destroy_nhwindow(winid);
staticfn void swift_curs(winid, int, int);
staticfn void swift_putstr(winid, int, const char *);
staticfn void swift_display_file(const char *, boolean);
staticfn void swift_start_menu(winid, unsigned long);
staticfn void swift_add_menu(winid, const glyph_info *, const anything *,
                             char, char, int, int, const char *, unsigned int);
staticfn void swift_end_menu(winid, const char *);
staticfn int  swift_select_menu(winid, int, MENU_ITEM_P **);
staticfn char swift_message_menu(char, int, const char *);
staticfn void swift_mark_synch(void);
staticfn void swift_wait_synch(void);
#ifdef CLIPPING
staticfn void swift_cliparound(int, int);
#endif
#ifdef POSITIONBAR
staticfn void swift_update_positionbar(char *);
#endif
staticfn void swift_print_glyph(winid, coordxy, coordxy,
								const glyph_info *, const glyph_info *);
staticfn void swift_raw_print(const char *);
staticfn void swift_raw_print_bold(const char *);
staticfn int swift_nhgetch(void);
staticfn int swift_nh_poskey(coordxy *, coordxy *, int *);
staticfn void swift_nhbell(void);
staticfn int swift_doprev_message(void);
staticfn char swift_yn_function(const char *, const char *, char);
staticfn void swift_getlin(const char *, char *);
staticfn int swift_get_ext_cmd(void);
staticfn void swift_number_pad(int);
staticfn void swift_delay_output(void);
#ifdef CHANGE_COLOR
staticfn void swift_change_color(int, long, int);
staticfn char *swift_get_color_string(void);
#endif
staticfn void swift_preference_update(const char *);
staticfn char *swift_getmsghistory(boolean);
staticfn void swift_putmsghistory(const char *, boolean);
staticfn void swift_status_init(void);
staticfn void swift_status_update(int, genericptr_t, int, int, int,
								  unsigned long *);
staticfn void swift_fill_inven_slot(nhswift_inven_slot *, NHEquipSlot, struct obj *);
staticfn void swift_update_inventory(int);
staticfn win_request_info *swift_ctrl_nhwindow(winid, int,
											   win_request_info *);

/* --- lifecycle --- */

staticfn void
swift_init_nhwindows(int *argcp, char **argv)
{
	if (!cb.initWindows)
		panic("winswift: nhswift_set_callbacks() was never called");

	if (cb.initWindows)
		(*cb.initWindows)(argcp, argv);

	iflags.window_inited = TRUE;
	iflags.perm_invent = TRUE;
}

staticfn void
swift_exit_nhwindows(const char *lastgasp)
{
	if (cb.exitWindows)
		(*cb.exitWindows)(lastgasp);

	iflags.window_inited = FALSE;
}

staticfn void
swift_suspend_nhwindows(const char *str)
{
	if (cb.suspendWindows)
		(*cb.suspendWindows)(str);
}

staticfn void
swift_resume_nhwindows(void)
{
	if (cb.resumeWindows)
		(*cb.resumeWindows)();
}

/* --- character creation --- */

staticfn void
swift_player_selection(void)
{
	nhswift_playerOptions opts;
	nhswift_playerSelection result;
	int i, use_custom = 0;

	memset(&opts, 0, sizeof opts);
	memset(&result, 0, sizeof result);
	result.roleIndex   = NHSWIFT_ROLE_RANDOM;
	result.raceIndex   = NHSWIFT_ROLE_RANDOM;
	result.genderIndex = NHSWIFT_ROLE_RANDOM;
	result.alignIndex  = NHSWIFT_ROLE_RANDOM;

	/* roles[] is terminated by a NULL name.m pointer. */
	for (i = 0; roles[i].name.m && i < NHSWIFT_MAX_ROLES; i++) {
		opts.roles[i] = roles[i].name.m;
		opts.roleGlyphs[i] = monnum_to_glyph(roles[i].mnum, MALE);
	}
	opts.roleCount = i;

	/* races[] is terminated by a NULL noun pointer. */
	for (i = 0; races[i].noun && i < NHSWIFT_MAX_RACES; i++) {
		opts.races[i] = races[i].noun;
		opts.raceGlyphs[i] = monnum_to_glyph(races[i].mnum, MALE);
	}
	opts.raceCount = i;

	/* ROLE_GENDERS is the count of player-selectable genders (male/female). */
	for (i = 0; i < ROLE_GENDERS && i < NHSWIFT_MAX_GENDERS; i++)
		opts.genders[i] = genders[i].adj;
	opts.genderCount = ROLE_GENDERS;

	/* ROLE_ALIGNS is the count of player-selectable alignments (lawful/neutral/chaotic). */
	for (i = 0; i < ROLE_ALIGNS && i < NHSWIFT_MAX_ALIGNS; i++)
		opts.aligns[i] = aligns[i].adj;
	opts.alignCount = ROLE_ALIGNS;

	if (cb.playerSelection)
		use_custom = (*cb.playerSelection)(&opts, &result);

	if (use_custom) {
		flags.initrole  = result.roleIndex;
		flags.initrace  = result.raceIndex;
		flags.initgend  = result.genderIndex;
		flags.initalign = result.alignIndex;
		if (result.playerName[0])
			Strcpy(svp.plname, result.playerName);
	} else {
		/* Fall back to NetHack's own role/race/gender/alignment dialog,
		   which drives it through our menu procs. */
		genl_player_setup(80);
	}
}

staticfn void
swift_askname(void)
{
	char buf[PL_NSIZ];

	buf[0] = '\0';
	if (cb.askName)
		(*cb.askName)(buf, (int) sizeof buf);

	if (buf[0]) {
		buf[sizeof buf - 1] = '\0';
		Strcpy(svp.plname, buf);
	}
	/* If the callback declined to supply one, the core's existing plname
	   (from config file, -u, or $USER) stands. */
}

/* --- windows --- */

staticfn winid
swift_create_nhwindow(int type)
{
	int w;

	if (!cb.createWindow)
		return WIN_ERR;
	w = (*cb.createWindow)(type);
	return (w >= 0) ? (winid) w : WIN_ERR;
}

staticfn void
swift_clear_nhwindow(winid window)
{
	if (cb.clearWindow)
		(*cb.clearWindow)((int) window);
}

staticfn void
swift_display_nhwindow(winid window, boolean blocking)
{
	if (cb.displayWindow)
		(*cb.displayWindow)((int) window, blocking ? 1 : 0);
}

staticfn void
swift_destroy_nhwindow(winid window)
{
	if (cb.destroyWindow)
		(*cb.destroyWindow)((int) window);
}

staticfn void
swift_curs(winid window, int x, int y)
{
	if (cb.moveCursor)
		(*cb.moveCursor)((int) window, x, y);
}

staticfn void
swift_putstr(winid window, int attr, const char *str)
{
	if (cb.putString)
		(*cb.putString)((int) window, attr, str ? str : "");
}

staticfn void
swift_display_file(const char *name, boolean complain)
{
	if (cb.displayFile)
		(*cb.displayFile)(name, complain ? 1 : 0);
	else
		genl_display_file(name, complain);
}

/* --- map --- */

staticfn void
swift_print_glyph(winid window, coordxy x, coordxy y,
				  const glyph_info *glyphinfo, const glyph_info *bkglyphinfo)
{
	if (!cb.printGlyph)
		return;
	(*cb.printGlyph)((int) window, (int) x, (int) y,
	                 (const nhswift_glyph *) glyphinfo,
	                 (const nhswift_glyph *) bkglyphinfo);
}

#ifdef CLIPPING
staticfn void
swift_cliparound(int x, int y)
{
	if (cb.clipAround)
		(*cb.clipAround)(x, y);
}
#endif

#ifdef POSITIONBAR
staticfn void
swift_update_positionbar(char *posbar)
{
	if (cb.updatePositionBar)
		(*cb.updatePositionBar)(posbar);
}
#endif

/* --- menus --- */

staticfn void
swift_start_menu(winid window, unsigned long mbehavior)
{
	if (cb.startMenu)
		(*cb.startMenu)((int) window, mbehavior);
}

/* Add one item to the menu being built in 'window'.
 *
 * glyphinfo  — optional tile/glyph to display beside the text.  If the item
 *              has no image, glyphinfo->glyph is NO_GLYPH.
 *
 * identifier — opaque value returned by select_menu() when this item is
 *              chosen.  Pass NULL or an anything set to zero to make a
 *              non-selectable header/separator line.
 *
 * ch         — preferred keyboard accelerator in [A-Za-z], or 0 to let the
 *              window port assign one.
 *
 * gch        — group accelerator: selects/deselects every item that shares
 *              this character (e.g. the object-class symbol '$' for all
 *              coins).  0 means the item is not part of any group.
 *
 * attr       — text attribute: ATR_NONE, ATR_BOLD, ATR_DIM, ATR_ITALIC,
 *              ATR_ULINE, ATR_BLINK, or ATR_INVERSE.
 *
 * clr        — foreground color: one of the CLR_* constants, or NO_COLOR.
 *
 * str        — item label text (never NULL from the core, but this function
 *              substitutes "" as a safety measure).
 *
 * itemflags  — bitmask of MENU_ITEMFLAGS_* values:
 *                MENU_ITEMFLAGS_NONE          — normal item
 *                MENU_ITEMFLAGS_SELECTED      — pre-checked in PICK_ANY menus
 *                MENU_ITEMFLAGS_SKIPINVERT    — not toggled by "select all"
 *                MENU_ITEMFLAGS_SKIPMENUCOLORS — ignore menu-color rules
 */
staticfn void
swift_add_menu(winid window, const glyph_info *glyphinfo,
               const anything *identifier, char ch, char gch, int attr,
               int clr, const char *str, unsigned int itemflags)
{
	uintptr_t ident = 0;

	if (!cb.addMenu)
		return;

	/* Reinterpret the anything union as a uintptr_t.  On all supported
	 * platforms sizeof(anything) == sizeof(uintptr_t), so this is lossless.
	 * The Swift side stores it opaquely and hands it back unchanged. */
	if (identifier) {
		size_t copy = sizeof *identifier < sizeof ident
		              ? sizeof *identifier : sizeof ident;
		(void) memcpy((genericptr_t) &ident,
		              (const genericptr_t) identifier, copy);
	}

	(*cb.addMenu)((int) window, (const nhswift_glyph *) glyphinfo, (int) ch, (int) gch, attr,
	               clr, str ? str : "", itemflags, ident);
}

staticfn void
swift_end_menu(winid window, const char *prompt)
{
	if (cb.endMenu)
		(*cb.endMenu)((int) window, prompt);
}

/* Present the menu identified by 'window' and wait for user input.
 *
 * how:
 *   PICK_NONE — display only; user acknowledges, no selection possible.
 *   PICK_ONE  — user taps a hotkey or clicks an item; returns immediately
 *               without an explicit Accept step.  ESC/Close returns -1.
 *   PICK_ANY  — checkboxes; user toggles items then presses Accept/Cancel.
 *
 * Return value (per doc/window.txt):
 *   > 0  — number of items selected; *menu_list points to a malloc'd
 *           menu_item array owned by the core (core calls free()).
 *   0    — no items selected (PICK_NONE acknowledged, or Accept with
 *           nothing checked).
 *   -1   — explicitly cancelled (ESC or Cancel).
 *
 * The bridge returns NHSWIFT_MENU_CANCELLED (-1) when the Swift UI calls
 * completion(nil), which maps cleanly to the -1 case below.
 * nhswift_menu_item is binary-compatible with menu_item (verified by
 * compile-time asserts near the top of this file), so the bridge-allocated
 * array can be handed directly to the core without copying.
 */
staticfn int
swift_select_menu(winid window, int how, MENU_ITEM_P **menu_list)
{
	nhswift_menu_item *sitems = (nhswift_menu_item *) 0;
	int n;

	*menu_list = (menu_item *) 0;

	if (!cb.selectMenu)
		return 0;

	n = (*cb.selectMenu)((int) window, how, &sitems);

	if (n > 0 && sitems) {
		/* The bridge malloc'd sitems; nhswift_menu_item is binary-compatible
		 * with menu_item (verified by the compile-time asserts above), so
		 * we can hand the allocation directly to the core.  The core owns
		 * the array and will free() it when done. */
		*menu_list = (menu_item *) sitems;
		return n;
	}
	free((genericptr_t) sitems); /* free(NULL) is a harmless no-op */
	return (n < 0) ? -1 : 0;
}

staticfn char
swift_message_menu(char let, int how, const char *mesg)
{
	if (cb.messageMenu)
		return (char) (*cb.messageMenu)((int) let, how, mesg);
	return genl_message_menu(let, how, mesg);
}

/* --- input --- */

staticfn int
swift_nhgetch(void)
{
	if (!cb.getChar)
		return '\033';
	return (*cb.getChar)();
}

staticfn int
swift_nh_poskey(coordxy *x, coordxy *y, int *mod)
{
	int ix = 0, iy = 0, imod = 0, ret;

	if (!cb.posKey)
		return swift_nhgetch();

	ret = (*cb.posKey)(&ix, &iy, &imod);
	if (x)
		*x = (coordxy) ix;
	if (y)
		*y = (coordxy) iy;
	if (mod)
		*mod = imod;
	return ret;
}

staticfn char
swift_yn_function(const char *query, const char *resp, char deflt)
{
	int ret;

	if (!cb.ynFunction)
		return deflt ? deflt : '\033';

	ret = (*cb.ynFunction)(query, resp, (int) deflt);
	return (char) ret;
}

staticfn void
swift_getlin(const char *query, char *bufp)
{
	if (!bufp)
		return;
	bufp[0] = '\0';
	if (cb.getLine)
		(*cb.getLine)(query, bufp, BUFSZ);
	else
		Strcpy(bufp, "\033");
}

staticfn int
swift_get_ext_cmd(void)
{
	nhswift_extcmd cmds[512];
	const struct ext_func_tab *efp;
	int i, n, result;

	if (!cb.getExtCmd)
		return -1;

	/* Build the filtered list, recording each entry's original index. */
	n = 0;
	for (efp = extcmdlist, i = 0; efp->ef_txt; efp++, i++) {
		if (efp->flags & (CMD_NOT_AVAILABLE | INTERNALCMD))
			continue;
		if (n >= (int)(sizeof cmds / sizeof cmds[0]))
			break;
		cmds[n].index = i;
		cmds[n].key   = (int)(unsigned char)efp->key;
		cmds[n].name  = efp->ef_txt;
		cmds[n].desc  = efp->ef_desc;
		n++;
	}

	/* The callback returns an index into cmds[]; translate back to extcmdlist[]. */
	result = (*cb.getExtCmd)(cmds, n);
	if (result < 0 || result >= n)
		return -1;
	return cmds[result].index;
}

staticfn int
swift_doprev_message(void)
{
	if (!cb.prevMessage)
		return 0;
	return (*cb.prevMessage)();
}

staticfn void
swift_number_pad(int state)
{
	if (cb.numberPad)
		(*cb.numberPad)(state);
}

/* --- synchronization / misc --- */

staticfn void
swift_get_nh_event(void)
{
	if (cb.getEvent)
		(*cb.getEvent)();
}

staticfn void
swift_mark_synch(void)
{
	if (cb.markSynch)
		(*cb.markSynch)();
}

/*
wait_synch()    -- Wait until all pending output is complete (*flush*() for
				   streams goes here).
				-- May also deal with exposure events etc. so that the
				   display is OK when return from wait_synch().
*/
staticfn void
swift_wait_synch(void)
{
	if (cb.waitSynch)
		(*cb.waitSynch)();
}

staticfn void
swift_delay_output(void)
{
	if (cb.delayOutput)
		(*cb.delayOutput)();
}

staticfn void
swift_nhbell(void)
{
	if (cb.bell)
		(*cb.bell)();
}

/* raw_print must work before init_nhwindows() and after a teardown, so it
   falls back to stderr rather than silently dropping the message. */
staticfn void
swift_raw_print(const char *str)
{
	if (cb.rawPrint)
		(*cb.rawPrint)(str ? str : "");
	else
		(void) fprintf(stderr, "%s\n", str ? str : "");
	if (str && *str)
		iflags.raw_printed++;
}

staticfn void
swift_raw_print_bold(const char *str)
{
	if (cb.rawPrintBold)
		(*cb.rawPrintBold)(str ? str : "");
	else
		swift_raw_print(str);
}

staticfn void
swift_preference_update(const char *pref)
{
	if (cb.preferenceUpdate)
		(*cb.preferenceUpdate)(pref);
}

staticfn void
swift_fill_inven_slot(nhswift_inven_slot *out, NHEquipSlot id, struct obj *obj)
{
	(void) memset((genericptr_t) out, 0, sizeof *out);
	out->slot = id;
	if (obj) {
		glyph_info gli;
		int glyph = obj_to_glyph(obj, rn2_on_display_rng);
		map_glyphinfo(0, 0, glyph, 0, &gli);
		(void) memcpy(&out->glyph, &gli, sizeof(nhswift_glyph));
		out->cursed  = (unsigned) obj->cursed;
		out->blessed = (unsigned) obj->blessed;
		out->bknown  = (unsigned) obj->bknown;
		(void) strlcpy(out->name, doname(obj), sizeof out->name);
	} else {
		out->glyph.glyph = -1;
		/* name[] is already "" from memset */
	}
}

staticfn void
swift_update_inventory(int arg)
{
	nhswift_inven_slot slots[NHSWIFT_SLOT_COUNT];

	if (cb.updateInventory) {
		swift_fill_inven_slot(&slots[NHEquipSlotWeapon],    NHEquipSlotWeapon,    uwep);
		swift_fill_inven_slot(&slots[NHEquipSlotAltHand],   NHEquipSlotAltHand,   u.twoweap ? uswapwep : uarms);
		swift_fill_inven_slot(&slots[NHEquipSlotShirt],     NHEquipSlotShirt,     uarmu);
		swift_fill_inven_slot(&slots[NHEquipSlotArmor],     NHEquipSlotArmor,     uarm);
		swift_fill_inven_slot(&slots[NHEquipSlotCloak],     NHEquipSlotCloak,     uarmc);
		swift_fill_inven_slot(&slots[NHEquipSlotHelmet],    NHEquipSlotHelmet,    uarmh);
		swift_fill_inven_slot(&slots[NHEquipSlotGloves],    NHEquipSlotGloves,    uarmg);
		swift_fill_inven_slot(&slots[NHEquipSlotBoots],     NHEquipSlotBoots,     uarmf);
		swift_fill_inven_slot(&slots[NHEquipSlotAmulet],    NHEquipSlotAmulet,    uamul);
		swift_fill_inven_slot(&slots[NHEquipSlotRingRight], NHEquipSlotRingRight, uright);
		swift_fill_inven_slot(&slots[NHEquipSlotRingLeft],  NHEquipSlotRingLeft,  uleft);
		swift_fill_inven_slot(&slots[NHEquipSlotBlindfold], NHEquipSlotBlindfold, ublindf);
		(*cb.updateInventory)(slots, NHSWIFT_SLOT_COUNT);
	}
}

/* --- message history --- */

staticfn char *
swift_getmsghistory(boolean init)
{
	static char histbuf[BUFSZ];

	if (!cb.getMsgHistory)
		return (char *) 0;

	histbuf[0] = '\0';
	if (!(*cb.getMsgHistory)(init ? 1 : 0, histbuf, (int) sizeof histbuf))
		return (char *) 0;
	if (!histbuf[0])
		return (char *) 0;
	histbuf[sizeof histbuf - 1] = '\0';
	return histbuf;
}

staticfn void
swift_putmsghistory(const char *msg, boolean restoring)
{
	if (cb.putMsgHistory)
		(*cb.putMsgHistory)(msg, restoring ? 1 : 0);
	else
		genl_putmsghistory(msg, restoring);
}

/* --- status --- */

staticfn void
swift_status_init(void)
{
	if (cb.statusInit)
		(*cb.statusInit)();
}

staticfn void
swift_status_enablefield(int fieldidx, const char *nm, const char *fmt,
						 boolean enable);

staticfn void
swift_status_enablefield(int fieldidx, const char *nm, const char *fmt,
						 boolean enable)
{
	if (cb.statusEnableField)
		(*cb.statusEnableField)(fieldidx, nm, fmt, enable ? 1 : 0);
	/* keep the core's own bookkeeping in sync as well */
	genl_status_enablefield(fieldidx, nm, fmt, enable);
}

/* The second argument is a char* for most fields but a long* for
   BL_CONDITION.  Discriminate here so Swift never has to. */
staticfn void
swift_status_update(int fldidx, genericptr_t ptr, int chg, int percent,
					int color, unsigned long *colormasks)
{
	const char *text = (const char *) 0;
	long condbits = 0L;

	if (!cb.statusUpdate)
		return;

	if (fldidx == BL_CONDITION)
		condbits = ptr ? *(long *) ptr : 0L;
	else if (fldidx >= 0)
		text = (const char *) ptr;

	/* For BL_LEVELDESC, replace tty-style "Dlvl:N" with a human-readable
	   string like "Dungeons of Doom: level 5", or "Gehennom 3: level 15"
	   when the branch level (dunlev) differs from the absolute depth.
	   Special locations (Knox, endgame, quest) are shown as
	   "<dname>: <description>". */
	char dlvltext[BUFSZ];
	if (fldidx == BL_LEVELDESC && text) {
		const char *dname = svd.dungeons[u.uz.dnum].dname;
		/* describe_level appends a trailing space; strip it */
		char trimmed[BUFSZ];
		Strcpy(trimmed, text);
		int tlen = (int) strlen(trimmed);
		while (tlen > 0 && trimmed[tlen - 1] == ' ')
			trimmed[--tlen] = '\0';
		if (strncmp(trimmed, "Dlvl:", 5) == 0
		    || strncmp(trimmed, "Tutorial:", 9) == 0) {
			const char *colon = strchr(trimmed, ':');
			int displayed = colon ? atoi(colon + 1) : 0;
			int lvl = dunlev(&u.uz);
			if (lvl == displayed)
				Snprintf(dlvltext, sizeof dlvltext,
						 "%s: level %d", dname, lvl);
			else
				Snprintf(dlvltext, sizeof dlvltext,
						 "%s %d: level %d", dname, lvl, displayed);
		} else {
			Snprintf(dlvltext, sizeof dlvltext, "%s: %s", dname, trimmed);
		}
		text = dlvltext;
	}

	/* For BL_ALIGN, augment the alignment text with race adjective and
	   role (or polymorph form) so Swift can display a combined role string
	   like "Neutral human Wizard" or "Neutral (human) kobold". */
	char roletext[BUFSZ];
	if (fldidx == BL_ALIGN && text) {
		boolean polyd = (u.umonnum != u.umonster);
		if (polyd)
			Snprintf(roletext, sizeof roletext, "%s (%s) %s",
					 text, gu.urace.adj, pmname(&mons[u.umonnum], Ugender));
		else
			Snprintf(roletext, sizeof roletext, "%s %s %s",
					 text, gu.urace.adj, pmname(&mons[u.umonnum], Ugender));
		text = roletext;
	}

	(*cb.statusUpdate)(fldidx, text, condbits, chg, percent, color,
						colormasks);
}

/* --- colors --- */

#ifdef CHANGE_COLOR
staticfn void
swift_change_color(int color, long rgb, int reverse)
{
	if (cb.changeColor)
		(*cb.changeColor)(color, rgb, reverse);
}

staticfn char *
swift_get_color_string(void)
{
	if (cb.getColorString)
		return (char *) (*cb.getColorString)();
	return (char *) 0;
}
#endif /* CHANGE_COLOR */

/* --- window control --- */

staticfn win_request_info *
swift_ctrl_nhwindow(winid window UNUSED, int request UNUSED,
					win_request_info *wri UNUSED)
{
	return (win_request_info *) 0;
}

/* ------------------------------------------------------------------ */
/* Interface definition used in windows.c                              */
/*                                                                     */
/* The order below must match struct window_procs in include/winprocs.h */
/* exactly.  It mirrors win/shim/winshim.c, which is a working 5.0 port.*/
/* ------------------------------------------------------------------ */

struct window_procs swift_procs = {
	WPID(swift),
	(0
	 | WC_ASCII_MAP
	 | WC_MOUSE_SUPPORT
	 | WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_EIGHT_BIT_IN),
	(0
#if defined(SELECTSAVED)
	 | WC2_SELECTSAVED
#endif
#if defined(STATUS_HILITES)
	 | WC2_HILITE_STATUS | WC2_HITPOINTBAR | WC2_RESET_STATUS
#endif
	 | WC2_FLUSH_STATUS
	 | WC2_DARKGRAY | WC2_SUPPRESS_HIST | WC2_STATUSLINES),
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, /* has_color[] */
	swift_init_nhwindows, swift_player_selection, swift_askname,
	swift_get_nh_event,
	swift_exit_nhwindows, swift_suspend_nhwindows, swift_resume_nhwindows,
	swift_create_nhwindow, swift_clear_nhwindow, swift_display_nhwindow,
	swift_destroy_nhwindow, swift_curs, swift_putstr, genl_putmixed,
	swift_display_file, swift_start_menu, swift_add_menu, swift_end_menu,
	swift_select_menu, swift_message_menu, swift_mark_synch,
	swift_wait_synch,
#ifdef CLIPPING
	swift_cliparound,
#endif
#ifdef POSITIONBAR
	swift_update_positionbar,
#endif
	swift_print_glyph, swift_raw_print, swift_raw_print_bold, swift_nhgetch,
	swift_nh_poskey, swift_nhbell, swift_doprev_message, swift_yn_function,
	swift_getlin, swift_get_ext_cmd, swift_number_pad, swift_delay_output,
#ifdef CHANGE_COLOR
	swift_change_color,
#ifdef MAC68K
	/* VERIFY: winshim.c tests `MAC` here while windows.c tests `MAC68K`.
	   Whichever winprocs.h actually uses is the one that belongs here --
	   getting it wrong shifts every later member by two slots. */
	(void (*)(int)) 0, (short (*)(winid, char *)) 0,
#endif
	swift_get_color_string,
#endif /* CHANGE_COLOR */
	genl_outrip,
	swift_preference_update,
	swift_getmsghistory, swift_putmsghistory,
	swift_status_init,
	genl_status_finish, swift_status_enablefield,
	swift_status_update,
	genl_can_suspend_yes,
	swift_update_inventory,
	swift_ctrl_nhwindow,
};

#endif /* SWIFT_GRAPHICS */

/*winswift.c*/
