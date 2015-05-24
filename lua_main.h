#ifndef LUA_MAIN_H
#define LUA_MAIN_H

#define ENV_TABLE_NAME "envtbl"
#define INTERP_TABLE_NAME "interptbl"

/* Names of some functions declared on the lua side */
#define GETSCRIPT_FUNCTION "GetScript"
#define SAVETABLE_FUNCTION "SaveTable"
#define LOADTABLE_FUNCTION "LoadTable"
#define TPRINTSTR_FUNCTION "glob_tprintstr"

#define RUNDELAY_VNUM -1
#define LOADSCRIPT_VNUM 0
#define MAX_LUA_SECURITY 9
#define SEC_NOSCRIPT 99
double genrand(void);

int L_delay( lua_State *LS);
int L_cancel( lua_State *LS);
const char *check_fstring( lua_State *LS, int index, size_t size);
const char *check_string( lua_State *LS, int index, size_t size);
int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn);
void stackDump (lua_State *LS);
void dump_prog( CHAR_DATA *ch, const char *prog, bool numberlines);


extern lua_State *g_mud_LS;
extern bool       g_LuaScriptInProgress;
int               g_ScriptSecurity;
int               g_LoopCheckCounter;

void new_ref( LUAREF *ref);
void free_ref( LUAREF *ref );

void save_ref( lua_State *LS, int index, LUAREF *ref );
void release_ref( lua_State *LS, LUAREF *ref );
void push_ref( lua_State *LS, LUAREF ref );
bool is_set_ref( LUAREF ref );

void quest_buy_ptitle( CHAR_DATA *ch, const char *argument);
void fix_ptitles( CHAR_DATA *ch );
#endif
