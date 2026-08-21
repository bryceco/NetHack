/* NetHack 5.0  winswift.h */
/* Interface between NetHack's "swift" windowport and a Swift/Objective-C
 * front end.
 *
 * This header deliberately includes NO NetHack headers.  Everything here is
 * plain C using standard types, so it can be imported by Swift package.
 *
 * All NetHack-specific types are translated on the C side in winswift.c:
 *   winid    -> int
 *   boolean  -> int (0 / 1)
 *   coordxy  -> int
 *   anything -> an integer item index (see add_menu / select_menu)
 *   glyph_info -> nhswift_glyph (below)
 */

#ifndef WINSWIFT_H
#define WINSWIFT_H

#include <stddef.h>
#include <stdint.h>  /* uintptr_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Constants mirrored from NetHack headers.                            */
/* winswift.c contains compile-time checks that these still match.     */
/* ------------------------------------------------------------------ */

enum nhswift_wintype {
	NHSWIFT_WIN_MESSAGE = 1,
	NHSWIFT_WIN_STATUS  = 2,
	NHSWIFT_WIN_MAP     = 3,
	NHSWIFT_WIN_MENU    = 4,
	NHSWIFT_WIN_TEXT    = 5
};

#ifdef __OBJC__
typedef NS_ENUM(NSInteger, NHPickMode) {
    NHPickModeNone = 0,  /* PICK_NONE — display only      */
    NHPickModeOne  = 1,  /* PICK_ONE  — single selection  */
    NHPickModeAny  = 2,  /* PICK_ANY  — multi-selection   */
};
#else
typedef enum {
	NHSWIFT_PICK_NONE = 0,
	NHSWIFT_PICK_ONE  = 1,
	NHSWIFT_PICK_ANY  = 2
} NHPickMode;
#endif

/* Colors — mirrors CLR_* and NO_COLOR from NetHack's color.h.
 * In Objective-C / Swift contexts this becomes an NS_ENUM.
 * winswift.c contains _Static_assert checks that these values still match. */
#define NHSWIFT_CLR_MAX 16

#ifdef __OBJC__
typedef NS_ENUM(NSInteger, NHColor) {
#else
typedef enum {
#endif
    NHColorBlack         =  0,   /* CLR_BLACK         */
    NHColorRed           =  1,   /* CLR_RED           */
    NHColorGreen         =  2,   /* CLR_GREEN         */
    NHColorBrown         =  3,   /* CLR_BROWN (low-intensity yellow on IBM) */
    NHColorBlue          =  4,   /* CLR_BLUE          */
    NHColorMagenta       =  5,   /* CLR_MAGENTA       */
    NHColorCyan          =  6,   /* CLR_CYAN          */
    NHColorGray          =  7,   /* CLR_GRAY (low-intensity white) */
    NHColorNone          =  8,   /* NO_COLOR          */
    NHColorOrange        =  9,   /* CLR_ORANGE        */
    NHColorBrightGreen   = 10,   /* CLR_BRIGHT_GREEN  */
    NHColorYellow        = 11,   /* CLR_YELLOW        */
    NHColorBrightBlue    = 12,   /* CLR_BRIGHT_BLUE   */
    NHColorBrightMagenta = 13,   /* CLR_BRIGHT_MAGENTA */
    NHColorBrightCyan    = 14,   /* CLR_BRIGHT_CYAN   */
    NHColorWhite         = 15,   /* CLR_WHITE         */
#ifdef __OBJC__
};                               /* NS_ENUM already declared the typedef */
#else
} NHColor;
#endif

/* Returned by select_menu when the user cancelled (ESC). */
#define NHSWIFT_MENU_CANCELLED (-1)

/* Size of the buffer handed to getlin() and getmsghistory(). */
#define NHSWIFT_BUFSZ 256

/* ------------------------------------------------------------------ */
/* Player character selection.                                          */
/*                                                                     */
/* winswift.c reads the NetHack role/race/gender/alignment tables and  */
/* passes them to the playerSelection callback so the front end can    */
/* present a native UI without knowing about NetHack's internal structs.*/
/* ------------------------------------------------------------------ */

/* Maximum array sizes — generous enough for any foreseeable NetHack. */
#define NHSWIFT_MAX_ROLES   16
#define NHSWIFT_MAX_RACES    8
#define NHSWIFT_MAX_GENDERS  3
#define NHSWIFT_MAX_ALIGNS   4

/* Matches NetHack's ROLE_RANDOM (-2) and ROLE_NONE (-1). */
#define NHSWIFT_ROLE_RANDOM (-2)
#define NHSWIFT_ROLE_NONE   (-1)

/* Player name buffer — matches NetHack's PL_NSIZ (32). */
#define NHSWIFT_PLNAME_SZ 32

/* Passed from winswift.c to the playerSelection callback.
 * Each pointer points directly into NetHack's static role/race/gender/align
 * tables, so the strings are valid for the lifetime of the callback. */
typedef struct {
	const char *roles[NHSWIFT_MAX_ROLES];     int roleCount;
	int roleGlyphs[NHSWIFT_MAX_ROLES];        /* male glyph number for each role */
	const char *races[NHSWIFT_MAX_RACES];     int raceCount;
	int raceGlyphs[NHSWIFT_MAX_RACES];        /* male glyph number for each race */
	const char *genders[NHSWIFT_MAX_GENDERS]; int genderCount;
	const char *aligns[NHSWIFT_MAX_ALIGNS];   int alignCount;
} nhswift_playerOptions;

/* Filled by the playerSelection callback; winswift.c uses it to set flags. */
typedef struct {
	int  roleIndex;                      /* index into roles[],   or NHSWIFT_ROLE_RANDOM */
	int  raceIndex;                      /* index into races[],   or NHSWIFT_ROLE_RANDOM */
	int  genderIndex;                    /* index into genders[], or NHSWIFT_ROLE_RANDOM */
	int  alignIndex;                     /* index into aligns[],  or NHSWIFT_ROLE_RANDOM */
	char playerName[NHSWIFT_PLNAME_SZ]; /* empty string = keep existing plname          */
} nhswift_playerSelection;

/* ------------------------------------------------------------------ */
/* Glyph description.                                                  */
/*                                                                     */
/* nhswift_glyph mirrors glyph_info (struct glyphinfo) from NetHack's  */
/* wintype.h, field-for-field and byte-for-byte, so that a             */
/* const glyph_info * can be passed to the Swift layer directly as     */
/* const nhswift_glyph * with no field-by-field copy.                  */
/* winswift.c contains _Static_assert checks that verify the layout.   */
/*                                                                     */
/* NOTE: ENHANCED_SYMBOLS is assumed defined (as it is on macOS), so   */
/* nhswift_glyph_map includes the trailing unicode pointer.             */
/* ------------------------------------------------------------------ */

/* Mirrors struct classic_representation in NetHack's wintype.h. */
typedef struct {
	int color;
	int symidx;
} nhswift_classic_sym;

/* Mirrors glyph_map (struct glyph_map_entry) in NetHack's wintype.h. */
typedef struct {
	unsigned            glyphflags;
	nhswift_classic_sym sym;
	uint32_t            customcolor;
	uint16_t            color256idx;
	int16_t             tileidx;   /* tile index for tile-based rendering */
	void               *u;        /* struct unicode_representation * (ENHANCED_SYMBOLS) */
} nhswift_glyph_map;

/* Mirrors glyph_info (struct glyphinfo) in NetHack's wintype.h. */
typedef struct nhswift_glyph {
	int               glyph;      /* raw glyph number; NO_GLYPH (-1) = empty */
	int               ttychar;    /* ASCII char a tty port would draw */
	uint32_t          framecolor;
	nhswift_glyph_map gm;
} nhswift_glyph;

/* ------------------------------------------------------------------ */
/* Equipment slots (worn items / wielded weapons).                     */
/*                                                                     */
/* Defined once here.  In Objective-C / Swift contexts this becomes a  */
/* proper NS_ENUM so Swift sees .weapon, .altHand, etc.  In plain C   */
/* (winswift.c) it compiles as a regular typedef enum.                 */
/* ------------------------------------------------------------------ */

/* Total number of NHEquipSlot values — use this for array sizes. */
#define NHSWIFT_SLOT_COUNT 12

#ifdef __OBJC__
typedef NS_ENUM(NSInteger, NHEquipSlot) {
#else
typedef enum {
#endif
	NHEquipSlotWeapon     = 0,  /* primary weapon (uwep) */
	NHEquipSlotAltHand    = 1,  /* shield, or off-hand weapon when two-weaponing */
	NHEquipSlotShirt      = 2,  /* under-shirt (uarmu) */
	NHEquipSlotArmor      = 3,  /* body armor (uarm) */
	NHEquipSlotCloak      = 4,  /* cloak (uarmc) */
	NHEquipSlotHelmet     = 5,  /* helmet (uarmh) */
	NHEquipSlotGloves     = 6,  /* gloves (uarmg) */
	NHEquipSlotBoots      = 7,  /* boots (uarmf) */
	NHEquipSlotAmulet     = 8,  /* amulet (uamul) */
	NHEquipSlotRingRight  = 9,  /* right ring (uright) */
	NHEquipSlotRingLeft   = 10, /* left ring (uleft) */
	NHEquipSlotBlindfold  = 11, /* blindfold / lenses (ublindf) */
#ifdef __OBJC__
};                             /* NS_ENUM macro already declared the typedef */
#else
} NHEquipSlot;
#endif

typedef struct {
	NHEquipSlot   slot;
	nhswift_glyph glyph;               /* glyph.glyph == NO_GLYPH if slot is empty */
	unsigned      cursed  : 1;
	unsigned      blessed : 1;
	unsigned      bknown  : 1;         /* BUC status is known */
	char          name[NHSWIFT_BUFSZ]; /* doname() result, or "" if empty */
} nhswift_inven_slot;

/* ------------------------------------------------------------------ */
/* Extended command descriptor.                                        */
/*                                                                     */
/* swift_get_ext_cmd() builds a filtered array of these on the stack  */
/* and passes it to (*cb.getExtCmd)().  Strings are owned by NetHack  */
/* and are valid for the lifetime of the process.                      */
/* ------------------------------------------------------------------ */

typedef struct {
	int        index; /* original index in extcmdlist[] — return this value */
	int        key;   /* primary key binding (0 if none; >127 = meta key)   */
	const char *name; /* ef_txt  — command name,   e.g. "pray"              */
	const char *desc; /* ef_desc — description,    e.g. "pray to the gods"  */
} nhswift_extcmd;

/* ------------------------------------------------------------------ */
/* Menu selection result.                                              */
/*                                                                     */
/* Mirrors the layout of NetHack's menu_item (anything item + long     */
/* count) so winswift.c can convert between the two with a simple      */
/* field-by-field copy.  The `identifier` field carries the anything   */
/* union reinterpreted as a uintptr_t.                                 */
/* ------------------------------------------------------------------ */

typedef struct {
	uintptr_t identifier;  /* anything value, passed back unchanged */
	long      count;       /* selection multiplier; -1 means "all" */
	unsigned  itemflags;   /* item flags (mirrors menu_item.itemflags) */
} nhswift_menu_item;

/* ------------------------------------------------------------------ */
/* Status fields.                                                      */
/*                                                                     */
/* statusUpdate() is called once per changed field, then once with     */
/* NHStatusFieldFlush meaning "you now have a consistent snapshot".    */
/* Update your model incrementally and redisplay on Flush.             */
/*                                                                     */
/* For most fields `text` holds the preformatted value and `condbits`  */
/* is 0.  For NHStatusFieldCondition `condbits` is the active-         */
/* condition bitmask and `text` is NULL.                               */
/*                                                                     */
/* winswift.c contains _Static_assert checks that these values match   */
/* the BL_* constants in NetHack's botl.h.                             */
/* ------------------------------------------------------------------ */

#define NHSWIFT_MAXBLSTATS 27   /* mirrors MAXBLSTATS; use for array sizing */

#ifdef __OBJC__
typedef NS_ENUM(NSInteger, NHStatusField) {
#else
typedef enum {
#endif
    NHStatusFieldCharacteristics = -3,  /* BL_CHARACTERISTICS: alias for Str..Ch group  */
    NHStatusFieldReset           = -2,  /* BL_RESET:           force full redisplay      */
    NHStatusFieldFlush           = -1,  /* BL_FLUSH:           end of one update cycle   */
    NHStatusFieldTitle           =  0,  /* BL_TITLE:    character name / title           */
    NHStatusFieldStr             =  1,  /* BL_STR:      strength                         */
    NHStatusFieldDex             =  2,  /* BL_DX:       dexterity                        */
    NHStatusFieldCon             =  3,  /* BL_CO:       constitution                     */
    NHStatusFieldInt             =  4,  /* BL_IN:       intelligence                     */
    NHStatusFieldWis             =  5,  /* BL_WI:       wisdom                           */
    NHStatusFieldCha             =  6,  /* BL_CH:       charisma                         */
    NHStatusFieldAlign           =  7,  /* BL_ALIGN:    alignment                        */
    NHStatusFieldScore           =  8,  /* BL_SCORE:    score                            */
    NHStatusFieldCap             =  9,  /* BL_CAP:      carrying capacity                */
    NHStatusFieldGold            = 10,  /* BL_GOLD:     gold                             */
    NHStatusFieldEnergy          = 11,  /* BL_ENE:      power (current)                  */
    NHStatusFieldEnergyMax       = 12,  /* BL_ENEMAX:   power (maximum)                  */
    NHStatusFieldXp              = 13,  /* BL_XP:       experience level                 */
    NHStatusFieldAc              = 14,  /* BL_AC:       armor class                      */
    NHStatusFieldHd              = 15,  /* BL_HD:       hit dice (monsters only)         */
    NHStatusFieldTime            = 16,  /* BL_TIME:     turn count                       */
    NHStatusFieldHunger          = 17,  /* BL_HUNGER:   hunger state                     */
    NHStatusFieldHp              = 18,  /* BL_HP:       hit points (current)             */
    NHStatusFieldHpMax           = 19,  /* BL_HPMAX:    hit points (maximum)             */
    NHStatusFieldLevelDesc       = 20,  /* BL_LEVELDESC: dungeon level description       */
    NHStatusFieldExp             = 21,  /* BL_EXP:      experience points                */
    NHStatusFieldCondition       = 22,  /* BL_CONDITION: condition bitmask (see condbits)*/
    NHStatusFieldWeapon          = 23,  /* BL_WEAPON:   wielded weapon                   */
    NHStatusFieldArmor           = 24,  /* BL_ARMOR:    worn armor                       */
    NHStatusFieldTerrain         = 25,  /* BL_TERRAIN:  current terrain                  */
    NHStatusFieldVersion         = 26,  /* BL_VERS:     version string                   */
#ifdef __OBJC__
};
#else
} NHStatusField;
#endif

/* NHCondition — bitmask of active player conditions (BL_CONDITION).  */
/* Verified against BL_MASK_* in winswift.c.                          */
#ifdef __OBJC__
typedef NS_OPTIONS(NSUInteger, NHCondition) {
#else
typedef enum {
#endif
    NHConditionBareHanded    = 0x00000001UL,  /* BL_MASK_BAREH     */
    NHConditionBlind         = 0x00000002UL,  /* BL_MASK_BLIND     */
    NHConditionBusy          = 0x00000004UL,  /* BL_MASK_BUSY      */
    NHConditionConfused      = 0x00000008UL,  /* BL_MASK_CONF      */
    NHConditionDeaf          = 0x00000010UL,  /* BL_MASK_DEAF      */
    NHConditionElfIron       = 0x00000020UL,  /* BL_MASK_ELF_IRON  */
    NHConditionFlying        = 0x00000040UL,  /* BL_MASK_FLY       */
    NHConditionFoodPoisoned  = 0x00000080UL,  /* BL_MASK_FOODPOIS  */
    NHConditionGlowingHands  = 0x00000100UL,  /* BL_MASK_GLOWHANDS */
    NHConditionGrabbed       = 0x00000200UL,  /* BL_MASK_GRAB      */
    NHConditionHallucinating = 0x00000400UL,  /* BL_MASK_HALLU     */
    NHConditionHeld          = 0x00000800UL,  /* BL_MASK_HELD      */
    NHConditionIcy           = 0x00001000UL,  /* BL_MASK_ICY       */
    NHConditionInLava        = 0x00002000UL,  /* BL_MASK_INLAVA    */
    NHConditionLevitating    = 0x00004000UL,  /* BL_MASK_LEV       */
    NHConditionParalyzed     = 0x00008000UL,  /* BL_MASK_PARLYZ    */
    NHConditionRiding        = 0x00010000UL,  /* BL_MASK_RIDE      */
    NHConditionSleeping      = 0x00020000UL,  /* BL_MASK_SLEEPING  */
    NHConditionSlimed        = 0x00040000UL,  /* BL_MASK_SLIME     */
    NHConditionSlippery      = 0x00080000UL,  /* BL_MASK_SLIPPERY  */
    NHConditionStoning       = 0x00100000UL,  /* BL_MASK_STONE     */
    NHConditionStrangling    = 0x00200000UL,  /* BL_MASK_STRNGL    */
    NHConditionStunned       = 0x00400000UL,  /* BL_MASK_STUN      */
    NHConditionSubmerged     = 0x00800000UL,  /* BL_MASK_SUBMERGED */
    NHConditionTerminalIll   = 0x01000000UL,  /* BL_MASK_TERMILL   */
    NHConditionTethered      = 0x02000000UL,  /* BL_MASK_TETHERED  */
    NHConditionTrapped       = 0x04000000UL,  /* BL_MASK_TRAPPED   */
    NHConditionUnconscious   = 0x08000000UL,  /* BL_MASK_UNCONSC   */
    NHConditionWoundedLegs   = 0x10000000UL,  /* BL_MASK_WOUNDEDL  */
    NHConditionHolding       = 0x20000000UL,  /* BL_MASK_HOLDING   */
#ifdef __OBJC__
};
#else
} NHCondition;
#endif

/* ------------------------------------------------------------------ */
/* The callback table.                                                 */
/* ------------------------------------------------------------------ */

/* NAMING: these members are deliberately camelCase.  include/winprocs.h
 * turns nearly every windowport name into an object-like macro --
 *     #define putstr (*windowprocs.win_putstr)
 * -- so a member named `putstr` breaks both this declaration and every
 * `cb.putstr` use site in winswift.c, no matter what order things are
 * included in.  NetHack's house style is strictly snake_case, so
 * camelCase members cannot collide.  Do not "tidy" these back to
 * snake_case.
 *
 * Every member may be NULL; winswift.c substitutes a safe default
 * (usually a no-op, or an ESC / "no" answer for input routines).
 *
 * IMPORTANT: all of these are called on NetHack's game thread, never on
 * the main thread.  Marshal to the main actor yourself.  The input
 * routines (nhgetch, yn_function, getlin, select_menu, nh_poskey,
 * get_ext_cmd) are expected to BLOCK until the user has answered.
 */
typedef struct nhswift_callbacks {
	/* --- lifecycle --- */
	void (*initWindows)(int *argcp, char **argv);
	void (*exitWindows)(const char *lastgasp);
	void (*suspendWindows)(const char *str);
	void (*resumeWindows)(void);

	/* --- character creation --- */
	/* opts contains the available role/race/gender/alignment strings read
	 * from NetHack's tables.  Fill *result with the user's choices and
	 * return 1; or return 0 to fall back to NetHack's built-in dialog. */
	int (*playerSelection)(const nhswift_playerOptions *opts,
	                       nhswift_playerSelection *result);
	/* Fill NetHack's player name.  Write into buf (NUL-terminated). */
	void (*askName)(char *buf, int bufsize);

	/* --- windows --- */
	int  (*createWindow)(int type);   /* return your own window id, or 0 */
	void (*clearWindow)(int window);
	void (*displayWindow)(int window, int blocking);
	void (*destroyWindow)(int window);
	void (*moveCursor)(int window, int x, int y);
	void (*putString)(int window, int attr, const char *str);
	void (*displayFile)(const char *name, int complain);

	/* --- map --- */
	void (*printGlyph)(int window, int x, int y,
						const nhswift_glyph *gi,
						const nhswift_glyph *bkgi);
	void (*clipAround)(int x, int y);

	/* --- menus --- */
	void (*startMenu)(int window, unsigned long mbehavior);
	/* `identifier` is the anything value for this item, passed back verbatim
	 * from select_menu().  Zero means the item is a non-selectable heading. */
	void (*addMenu)(int window, const nhswift_glyph *gi,
					 int ch, int gch, int attr, int clr,
					 const char *str, unsigned int itemflags,
					 uintptr_t identifier);
	void (*endMenu)(int window, const char *prompt);
	/* Allocate and return (via *out_items) an array of chosen items;
	 * return how many were selected, 0 for none, or NHSWIFT_MENU_CANCELLED
	 * if the user backed out.  The caller owns the array and frees it with
	 * free().  *out_items may be left NULL when the return value is <= 0. */
	int  (*selectMenu)(int window, int how,
						nhswift_menu_item **out_items);
	/* Feedback from the --More-- prompt; return 0 if you don't handle it. */
	int  (*messageMenu)(int let, int how, const char *mesg);

	/* --- input --- */
	int  (*getChar)(void);
	int  (*posKey)(int *x, int *y, int *mod);
	int  (*ynFunction)(const char *query, const char *resp, int def);
	void (*getLine)(const char *query, char *buf, int bufsize);
	/* cmds is a filtered snapshot of extcmdlist[]; count is its length.
	 * Return the index into cmds of the chosen command, or -1 to cancel.
	 * swift_get_ext_cmd() translates that back to the extcmdlist[] index. */
	int  (*getExtCmd)(const nhswift_extcmd *cmds, int count);
	int  (*prevMessage)(void);
	void (*numberPad)(int state);

	/* --- synchronization / misc --- */
	void (*getEvent)(void);
	void (*markSynch)(void);
	void (*waitSynch)(void);
	void (*delayOutput)(void);
	void (*bell)(void);
	void (*rawPrint)(const char *str);
	void (*rawPrintBold)(const char *str);
	void (*preferenceUpdate)(const char *pref);
	/* slots points to an array of NHSWIFT_SLOT_COUNT entries, one per worn
	 * slot in nhswift_inven_slot_id order.  count == NHSWIFT_SLOT_COUNT. */
	void (*updateInventory)(const nhswift_inven_slot *slots, int count);
	void (*updatePositionBar)(const char *posbar);

	/* --- message history (save/restore) --- */
	/* Return 1 and fill buf with the next message to save, oldest first;
	 * return 0 when there are no more. */
	int  (*getMsgHistory)(int init, char *buf, int bufsize);
	void (*putMsgHistory)(const char *msg, int restoring);

	/* --- status --- */
	void (*statusInit)(void);
	void (*statusEnableField)(NHStatusField fieldidx, const char *nm,
							   const char *fmt, int enable);
	/* fldidx is plain int (not NHStatusField) so that the C caller and the
	 * ObjC callee agree on parameter width.  NHStatusField is int-sized in
	 * C but NSInteger-sized (long) in ObjC; passing it as NHStatusField
	 * through the function pointer causes ARM64 to zero-extend the value
	 * and deliver 4294967294 instead of -2 for BL_RESET. */
	void (*statusUpdate)(int fldidx, const char *text, long condbits,
						  int chg, int percent, NHColor color,
						  const unsigned long *colormasks);

	/* --- colors (only used when NetHack is built with CHANGE_COLOR) --- */
	void (*changeColor)(int color, long rgb, int reverse);
	const char *(*getColorString)(void);
} nhswift_callbacks;

/* ------------------------------------------------------------------ */
/* Entry points.                                                       */
/* ------------------------------------------------------------------ */

/* Set the paths for resources and playground */
void nhswift_set_paths(const char *hackdir, const char *playground);

/* Install the callback table.  MUST be called before choose_windows()
 * runs, i.e. before you start the game.  The table is copied, so the
 * caller does not need to keep it alive.  Passing NULL clears it. */
void nhswift_set_callbacks(const nhswift_callbacks *cb);

/* Query whether a role/race/gender/alignment combination is valid.
 * Each index is 0-based into the arrays passed to the playerSelection
 * callback, or NHSWIFT_ROLE_RANDOM (-2) to treat that slot as "any".
 * Returns 1 if valid, 0 if not. */
int nhswift_validrole(int roleIndex);
int nhswift_validrace(int roleIndex, int raceIndex);
int nhswift_validgend(int roleIndex, int raceIndex, int genderIndex);
int nhswift_validalign(int roleIndex, int raceIndex, int alignIndex);

/* Map a NetHack glyph number to its tile-sheet index (the value stored in
 * glyphmap[glyph].tileidx).  Returns -1 for out-of-range glyph numbers. */
int nhswift_glyph_to_tile(int glyph);

#ifdef __cplusplus
}
#endif

#endif /* WINSWIFT_H */
