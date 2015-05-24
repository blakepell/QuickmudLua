#ifndef LUA_SCRIPTING_H
#define LUA_SCRIPTING_H


void lua_mob_program( const char *text, int pvnum, const char *source,
        CHAR_DATA *mob, CHAR_DATA *ch,
        const void *arg1, sh_int arg1type,
        const void *arg2, sh_int arg2type,
        int trig_type,
        int security );

bool lua_obj_program( const char *trigger, int pvnum, const char *source,
        OBJ_DATA *obj, OBJ_DATA *obj2,CHAR_DATA *ch1, CHAR_DATA *ch2,
        int trig_type,
        int security );

bool lua_area_program( const char *trigger, int pvnum, const char *source,
        AREA_DATA *area, CHAR_DATA *ch1,
        int trig_type,
        int security );

bool lua_room_program( const char *trigger, int pvnum, const char *source,
        ROOM_INDEX_DATA *room, 
        CHAR_DATA *ch1, CHAR_DATA *ch2,
        OBJ_DATA *obj1, OBJ_DATA *obj2,
        const char *text1,
        int trig_type,
        int security);
   
bool lua_load_mprog( lua_State *LS, int vnum, const char *code);
bool lua_load_oprog( lua_State *LS, int vnum, const char *code);
bool lua_load_aprog( lua_State *LS, int vnum, const char *code);
bool lua_load_rprog( lua_State *LS, int vnum, const char *code);

DECLARE_DO_FUN(do_lboard);
DECLARE_DO_FUN(do_lhistory);
void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment );
void save_lboards();
void load_lboards();
void check_lboard_reset();
void check_mprog( lua_State *LS, int vnum, const char *code );
void check_oprog( lua_State *LS, int vnum, const char *code );
void check_aprog( lua_State *LS, int vnum, const char *code );
void check_rprog( lua_State *LS, int vnum, const char *code );
DECLARE_DO_FUN(do_lua);
bool run_lua_interpret( DESCRIPTOR_DATA *d );
void lua_unregister_desc( DESCRIPTOR_DATA *d );
void run_delayed_function( TIMER_NODE *tmr );
void open_lua();
bool run_lua_interpret( DESCRIPTOR_DATA *d);
DECLARE_DO_FUN(do_luai);

int GetLuaMemoryUsage();
int GetLuaGameObjectCount();
int GetLuaEnvironmentCount();

extern lua_State *g_mud_LS;
extern bool g_LuaScriptInProgress;
#endif
