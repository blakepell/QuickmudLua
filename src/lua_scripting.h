#ifndef LUA_SCRIPTING_H
#define LUA_SCRIPTING_H


void lua_mob_program( const char *text, int pvnum, const char *source,
        CHAR_DATA *mob, CHAR_DATA *ch,
        const void *arg1, sh_int arg1type,
        const void *arg2, sh_int arg2type,
        int trig_type,
        int security );
   
bool lua_load_mprog( lua_State *LS, int vnum, const char *code);
void check_mprog( lua_State *LS, int vnum, const char *code );

extern lua_State *g_mud_LS;
extern bool g_LuaScriptInProgress;
#endif
