#include <lualib.h>
#include <lauxlib.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "timer.h"
#include "lua_main.h"
#include "lua_arclib.h"
#include "interp.h"
#include "mudconfig.h"

void sorted_ctable_init( lua_State *LS );

lua_State *g_mud_LS = NULL;  /* Lua state for entire MUD */
bool       g_LuaScriptInProgress=FALSE;
int        g_ScriptSecurity=SEC_NOSCRIPT;
int        g_LoopCheckCounter;


/* keep these as LUAREFS for ease of use on the C side */
static LUAREF TABLE_INSERT;
static LUAREF TABLE_CONCAT;
static LUAREF STRING_FORMAT;

void register_LUAREFS( lua_State *LS)
{
    /* initialize the variables */
    new_ref( &TABLE_INSERT );
    new_ref( &TABLE_CONCAT );
    new_ref( &STRING_FORMAT );

    /* put stuff in the refs */
    lua_getglobal( LS, "table" );

    lua_getfield( LS, -1, "insert" );
    save_ref( LS, -1, &TABLE_INSERT );
    lua_pop( LS, 1 ); /* insert */

    lua_getfield( LS, -1, "concat" );
    save_ref( LS, -1, &TABLE_CONCAT );
    lua_pop( LS, 2 ); /* concat and table */

    lua_getglobal( LS, "string" );
    lua_getfield( LS, -1, "format" );
    save_ref( LS, -1, &STRING_FORMAT );
    lua_pop( LS, 2 ); /* string and format */
}

#define LUA_LOOP_CHECK_MAX_CNT 10000 /* give 1000000 instructions */
#define LUA_LOOP_CHECK_INCREMENT 100
#define ERR_INF_LOOP      -1

int GetLuaMemoryUsage()
{
    return lua_gc( g_mud_LS, LUA_GCCOUNT, 0);
}

int GetLuaGameObjectCount()
{
    lua_getglobal( g_mud_LS, "UdCnt");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 1) )
    {
        bugf ( "Error with UdCnt:\n %s",
                lua_tostring(g_mud_LS, -1));
        return -1;
    }

    int rtn=luaL_checkinteger( g_mud_LS, -1 );
    lua_pop( g_mud_LS, 1 );
    return rtn;
}

int GetLuaEnvironmentCount()
{
    lua_getglobal( g_mud_LS, "EnvCnt");  
    if (CallLuaWithTraceBack( g_mud_LS, 0, 1) )
    {
        bugf ( "Error with EnvCnt:\n %s",
                lua_tostring(g_mud_LS, -1));
        return -1;
    }

    int rtn=luaL_checkinteger( g_mud_LS, -1 );
    lua_pop( g_mud_LS, 1 );
    return rtn;
}

static void infinite_loop_check_hook( lua_State *LS, lua_Debug *ar)
{
    if (!g_LuaScriptInProgress)
        return;

    if ( g_LoopCheckCounter < LUA_LOOP_CHECK_MAX_CNT)
    {
        g_LoopCheckCounter++;
        return;
    }
    else
    {
        /* exit */
        luaL_error( LS, "Interrupted infinite loop." );
        return;
    }
}



void stackDump (lua_State *LS) {
    int i;
    int top = lua_gettop(LS);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(LS, i);
        switch (t) {

            case LUA_TSTRING:  /* strings */
                logpf("`%s'", lua_tostring(LS, i));
                break;

            case LUA_TBOOLEAN:  /* booleans */
                logpf(lua_toboolean(LS, i) ? "true" : "false");
                break;

            case LUA_TNUMBER:  /* numbers */
                logpf("%g", lua_tonumber(LS, i));
                break;

            default:  /* other values */
                logpf("%s", lua_typename(LS, t));
                break;

        }
    }
    logpf("\n");  /* end the listing */
}

const char *check_string( lua_State *LS, int index, size_t size)
{
    size_t rtn_size;
    const char *rtn=luaL_checklstring( LS, index, &rtn_size );
    /* Check >= because we assume 'size' argument refers to
       size of a char buffer rather than desired strlen.
       If called with MSL then the result (including terminating '\0'
       will fit in MSL sized buffer. */
    if (rtn_size >= size )
        luaL_error( LS, "String size %d exceeds maximum %d.",
                (int)rtn_size, (int)size-1 );

    return rtn;
}

/* Run string.format using args beginning at index 
   Assumes top is the last argument*/
const char *check_fstring( lua_State *LS, int index, size_t size)
{
    int narg=lua_gettop(LS)-(index-1);

    if ( (narg>1))
    {
        push_ref( LS, STRING_FORMAT );
        lua_insert( LS, index );
        lua_call( LS, narg, 1);
    }

    return check_string( LS, index, size);
}

static void GetTracebackFunction (lua_State *LS)
{
    lua_pushliteral (LS, LUA_DBLIBNAME);     /* "debug"   */
    lua_rawget      (LS, LUA_GLOBALSINDEX);    /* get debug library   */

    if (!lua_istable (LS, -1))
    {
        lua_pop (LS, 2);   /* pop result and debug table  */
        lua_pushnil (LS);
        return;
    }

    /* get debug.traceback  */
    lua_pushstring(LS, "traceback");
    lua_rawget    (LS, -2);               /* get traceback function  */

    if (!lua_isfunction (LS, -1))
    {
        lua_pop (LS, 2);   /* pop result and debug table  */
        lua_pushnil (LS);
        return;
    }

    lua_remove (LS, -2);   /* remove debug table, leave traceback function  */
}  /* end of GetTracebackFunction */

int CallLuaWithTraceBack (lua_State *LS, const int iArguments, const int iReturn)
{
    int error;

    int base = lua_gettop (LS) - iArguments;  /* function index */
    GetTracebackFunction (LS);
    if (lua_isnil (LS, -1))
    {
        lua_pop (LS, 1);   /* pop non-existent function  */
        bugf("pop non-existent function");
        error = lua_pcall (LS, iArguments, iReturn, 0);
    }
    else
    {
        lua_insert (LS, base);  /* put it under chunk and args */
        error = lua_pcall (LS, iArguments, iReturn, base);
        lua_remove (LS, base);  /* remove traceback function */
    }

    return error;
}  /* end of CallLuaWithTraceBack  */

DEF_DO_FUN(do_lboard)
{
    lua_getglobal(g_mud_LS, "do_lboard");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf ( "Error with do_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_lhistory)
{
    lua_getglobal(g_mud_LS, "do_lhistory");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf ( "Error with do_lhistory:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void update_lboard( int lboard_type, CHAR_DATA *ch, int current, int increment )
{
    if (IS_NPC(ch) || IS_IMMORTAL(ch) )
        return;

    lua_getglobal(g_mud_LS, "update_lboard");
    lua_pushnumber( g_mud_LS, lboard_type);
    lua_pushstring( g_mud_LS, ch->name);
    lua_pushnumber( g_mud_LS, current);
    lua_pushnumber( g_mud_LS, increment);

    if (CallLuaWithTraceBack( g_mud_LS, 4, 0) )
    {
        bugf ( "Error with update_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void save_lboards()
{
    lua_getglobal( g_mud_LS, "save_lboards");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with save_lboard:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }  
}

void load_lboards()
{
    lua_getglobal( g_mud_LS, "load_lboards");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_lboards:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void check_lboard_reset()
{
    lua_getglobal( g_mud_LS, "check_lboard_reset");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with check_lboard_resets:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

/* currently unused - commented to avoid warning
static int L_save_mudconfig(lua_State *LS)
{
    int i;
    CFG_DATA_ENTRY *en;

    lua_getglobal(LS, "SaveTbl");
    lua_newtable(LS);
    for ( i=0 ; mudconfig_table[i].name ; i++ )
    {
        en=&mudconfig_table[i];
        switch( en->type )
        {
            case CFG_INT:
            {
                lua_pushinteger( LS, *((int *)(en->value)));
                break;
            }
            case CFG_FLOAT:
            {
                lua_pushnumber( LS, *((float *)(en->value)));
                break;
            }
            case CFG_STRING:
            {
                lua_pushstring( LS, *((char **)(en->value)));
                break;
            }
            case CFG_BOOL:
            {
                lua_pushboolean( LS, *((bool *)(en->value)));
                break;
            }
            default:
            {
                luaL_error( LS, "Bad type.");
            }
        }

        lua_setfield(LS, -2, en->name);
    }

    return 1;
}
*/

void save_mudconfig()
{
    lua_getglobal( g_mud_LS, "save_mudconfig");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with save_mudconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void load_mudconfig()
{
    lua_getglobal( g_mud_LS, "load_mudconfig");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_mudconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }    
}

bool run_lua_interpret( DESCRIPTOR_DATA *d)
{
    if (!d->lua.interpret) /* not in interpreter */
        return FALSE; 

    if (!strcmp( d->incomm, "@") )
    {
        /* kick out of interpret */
        d->lua.interpret=FALSE;
        d->lua.incmpl=FALSE;

        lua_unregister_desc(d);

        ptc(d->character, "Exited lua interpreter.\n\r");
        return TRUE;
    }

    lua_getglobal( g_mud_LS, "run_lua_interpret"); //what we'll call if no errors

    /* Check this all in C so we can exit interpreter for missing env */
    lua_pushlightuserdata( g_mud_LS, d); /* we'll check this against the table */
    lua_getglobal( g_mud_LS, INTERP_TABLE_NAME);
    if (lua_isnil( g_mud_LS, -1) )
    {
        bugf("Couldn't find " INTERP_TABLE_NAME);
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }
    bool interpalive=FALSE;
    lua_pushnil( g_mud_LS);
    while (lua_next(g_mud_LS, -2) != 0)
    {
        lua_getfield( g_mud_LS, -1, "desc");
        if (lua_equal( g_mud_LS, -1,-5))
        {
           interpalive=TRUE;
        }
        lua_pop(g_mud_LS, 2);

        if (interpalive)
            break;
    }
    if (!interpalive)
    {
        ptc( d->character, "Interpreter session was closed, was object destroyed?\n\r"
                "Exiting interpreter.\n\r");
        d->lua.interpret=FALSE;

        ptc(d->character, "Exited lua interpreter.\n\r");
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }
    /* object pointer should be sitting at -1, interptbl at -2, desc lightud at -3 */
    lua_remove( g_mud_LS, -3);
    lua_remove( g_mud_LS, -2);


    lua_getglobal( g_mud_LS, ENV_TABLE_NAME);
    if (lua_isnil( g_mud_LS, -1) )
    {
        bugf("Couldn't find " ENV_TABLE_NAME);
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }
    lua_pushvalue( g_mud_LS, -2);
    lua_remove( g_mud_LS, -3);
    lua_gettable( g_mud_LS, -2);
    lua_remove( g_mud_LS, -2); /* don't need envtbl anymore*/
    if ( lua_isnil( g_mud_LS, -1) )
    {
        bugf("Game object not found for interpreter session for %s.", d->character->name);
        ptc( d->character, "Couldn't find game object, was it destroyed?\n\r"
                "Exiting interpreter.\n\r");
        d->lua.interpret=FALSE;

        ptc(d->character, "Exited lua interpreter.\n\r");
        lua_settop(g_mud_LS, 0);
        return TRUE;
    }

    /* if we're here then we just need to push the string and call the func */
    lua_pushstring( g_mud_LS, d->incomm);

    g_ScriptSecurity= d->character->pcdata->security ;
    g_LoopCheckCounter=0;
    g_LuaScriptInProgress = TRUE;

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        ptc(d->character,  "LUA error for lua_interpret:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        d->lua.incmpl=FALSE; //force it whether it was or wasn't
    } 
    else
    {
        bool incmpl=(bool)luaL_checknumber( g_mud_LS, -1 );
        if (incmpl)
        {
            d->lua.incmpl=TRUE;
        } 
        else
        {
            d->lua.incmpl=FALSE;
        
        }
    }

    g_ScriptSecurity = SEC_NOSCRIPT;
    g_LuaScriptInProgress=FALSE;

    lua_settop( g_mud_LS, 0);
    return TRUE;

}

void lua_unregister_desc (DESCRIPTOR_DATA *d)
{
    lua_getglobal( g_mud_LS, "UnregisterDesc");
    lua_pushlightuserdata( g_mud_LS, d);
    int error=CallLuaWithTraceBack (g_mud_LS, 1, 0) ;
    if (error > 0 )
    {
        ptc(d->character,  "LUA error for UnregisterDesc:\n %s",
                lua_tostring(g_mud_LS, -1));
    }
}

DEF_DO_FUN(do_luai)
{
    if IS_NPC(ch)
        return;
    
    if ( ch->desc->lua.interpret )
    {
        ptc(ch, "Lua interpreter already active!\n\r");
        return;
    }

    char arg1[MSL];
    const char *name;

    argument=one_argument(argument, arg1);

    void *victim=NULL;
    LUA_OBJ_TYPE *type;

    if ( arg1[0]== '\0' )
    {
        victim=(void *)ch;
        type=&CH_type;
        name=ch->name;
    }
    else if (!strcmp( arg1, "mob") )
    {
        CHAR_DATA *mob;
        mob=get_char_room( ch, argument );
        if (!mob)
        {
            ptc(ch, "Could not find %s in the room.\n\r", argument);
            return;
        }
        else if (!IS_NPC(mob))
        {
            ptc(ch, "Not on PCs.\n\r");
            return;
        }

        victim = (void *)mob;
        type= &CH_type;
        name=mob->name;
    }
    else if (!strcmp( arg1, "obj") )
    {
        OBJ_DATA *obj=NULL;
        obj=get_obj_here( ch, argument);

        if (!obj)
        {
            ptc(ch, "Could not find %s in room or inventory.\n\r", argument);
            return;
        }

        victim= (void *)obj;
        type=&OBJ_type;
        name=obj->name;
    }
    else if (!strcmp( arg1, "area") )
    {
        if (!ch->in_room)
        {
            bugf("do_luai: %s in_room is NULL.", ch->name);
            return;
        }

        victim= (void *)(ch->in_room->area);
        type=&AREA_type;
        name=ch->in_room->area->name;
    }
    else if (!strcmp( arg1, "room") )
    {
        if (!ch->in_room)
        {
            bugf("do_luai: %s in_room is NULL.", ch->name);
            return;
        }
        
        victim= (void *)(ch->in_room);
        type=&ROOM_type;
        name=ch->in_room->name;
    }
    else
    {
        ptc(ch, "luai [no argument] -- open interpreter in your own env\n\r"
                "luai mob <target>  -- open interpreter in env of target mob (in same room)\n\r"
                "luai obj <target>  -- open interpreter in env of target obj (inventory or same room)\n\r"
                "luai area          -- open interpreter in env of current area\n\r"
                "luai room          -- open interpreter in env of current room\n\r"); 
        return;
    }

    if (!ch->desc)
    {
        bugf("do_luai: %s has null desc", ch->name);
        return;
    }

    /* do the stuff */
    lua_getglobal( g_mud_LS, "interp_setup");
    if ( !type->push(g_mud_LS, victim) )
    {
        bugf("do_luai: couldn't make udtable argument %s",
                argument);
        lua_settop(g_mud_LS, 0);
        return;
    }

    if ( type == &CH_type )
    {
        lua_pushliteral( g_mud_LS, "mob"); 
    }
    else if ( type == &OBJ_type )
    {
        lua_pushliteral( g_mud_LS, "obj"); 
    }
    else if ( type == &AREA_type )
    {
        lua_pushliteral( g_mud_LS, "area"); 
    }
    else if ( type == &ROOM_type )
    {
        lua_pushliteral( g_mud_LS, "room"); 
    }
    else
    {
            bugf("do_luai: invalid type: %s", type->type_name);
            lua_settop(g_mud_LS, 0);
            return;
    }

    lua_pushlightuserdata(g_mud_LS, ch->desc);
    lua_pushstring( g_mud_LS, ch->name );

    int error=CallLuaWithTraceBack (g_mud_LS, 4, 2) ;
    if (error > 0 )
    {
        bugf ( "LUA error for interp_setup:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_settop(g_mud_LS, 0);
        return;
    }

    /* 2 values, true or false (false if somebody already interpreting on that object)
       and name of person interping if false */
    bool success=(bool)luaL_checknumber( g_mud_LS, -2);
    if (!success)
    {
        ptc(ch, "Can't open lua interpreter, %s already has it open for that object.\n\r",
                check_string( g_mud_LS, -1, MIL));
        lua_settop(g_mud_LS, 0);
        return;
    }

    /* finally, if everything worked out, we can set this stuff */
    ch->desc->lua.interpret=TRUE;
    ch->desc->lua.incmpl=FALSE;

    ptc(ch, "Entered lua interpreter mode for %s %s\n\r", 
            type->type_name,
            name);
    ptc(ch, "Use @ on a blank line to exit.\n\r");
    ptc(ch, "Use do and end to create multiline chunks.\n\r");
    ptc(ch, "Use '%s' to access target's self.\n\r",
            type == &CH_type ? "mob" :
            type == &OBJ_type ? "obj" :
            type == &ROOM_type ? "room" :
            type == &AREA_type ? "area" :
            "ERROR" );

    lua_settop(g_mud_LS, 0);
    return;
}

static int RegisterLuaRoutines (lua_State *LS)
{
    time_t timer;
    time (&timer);

    init_genrand (timer);
    type_init( LS );

    register_globals ( LS );
    sorted_ctable_init( LS );
    register_LUAREFS( LS );

    return 0;

}  /* end of RegisterLuaRoutines */

void open_lua ()
{
    lua_State *LS = luaL_newstate ();   /* opens Lua */
    g_mud_LS = LS;

    if (g_mud_LS == NULL)
    {
        bugf("Cannot open Lua state");
        return;  /* catastrophic failure */
    }

    luaL_openlibs (LS);    /* open all standard libraries */

    /* call as Lua function because we need the environment  */
    lua_pushcfunction(LS, RegisterLuaRoutines);
    lua_call(LS, 0, 0);

    lua_sethook(LS, infinite_loop_check_hook, LUA_MASKCOUNT, LUA_LOOP_CHECK_INCREMENT);
    /* run initialiation script */
    if (luaL_loadfile (LS, LUA_STARTUP) ||
            CallLuaWithTraceBack (LS, 0, 0))
    {
        bugf ( "Error loading Lua startup file %s:\n %s", 
                LUA_STARTUP,
                lua_tostring(LS, -1));
    }

    lua_settop (LS, 0);    /* get rid of stuff lying around */

}  /* end of open_lua */

DEF_DO_FUN(do_scriptdump)
{
    lua_getglobal(g_mud_LS, "do_scriptdump");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_scriptdump:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

}
static int L_wizhelp( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    
    int index=1; /* the current table index */
    lua_newtable( LS );/* the table of commands */
    
    int cmd;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( cmd_table[cmd].level >= LEVEL_HERO
            &&  is_granted( ud_ch, cmd_table[cmd].do_fun )
            &&  cmd_table[cmd].show )
        {
            lua_newtable( LS ); /* this command's table */
            
            lua_pushinteger( LS, cmd_table[cmd].level );
            lua_setfield( LS, -2, "level" );

            lua_pushstring( LS, cmd_table[cmd].name );
            lua_setfield( LS, -2, "name" );
            
            lua_rawseti( LS, -2, index++ ); /* save the command to the table */
        }
    }

    lua_getglobal( LS, "wizhelp" );
    lua_insert( LS, 1 ); /* shove it to the top */

    lua_call( LS, lua_gettop(LS)-1, 0 );

    return 0;
}
     
DEF_DO_FUN(do_luaquery)
{
    lua_getglobal( g_mud_LS, "do_luaquery");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luaquery:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}    

DEF_DO_FUN(do_wizhelp)
{
    lua_pushcfunction(g_mud_LS, L_wizhelp);
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_wizhelp:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_qset)
{
    lua_getglobal( g_mud_LS, "do_qset");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_qset:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

static CHAR_DATA *clt_list;
static int L_charloadtest( lua_State *LS )
{
    /* 1 is script function */
    /* 2 is ch */
    DESCRIPTOR_DATA d;
    clt_list=NULL;

    lua_getglobal( LS, "list_files");
    lua_pushliteral( LS, "../area");
    lua_call( LS, 1, 1 );
  
    lua_newtable( LS ); /* the table of chars */ 
    /* iterate the table */
    int index;
    int newindex=1;
    for ( index=1 ; ; index++ )
    {
        lua_rawgeti( LS, -2, index );
        if (lua_isnil( LS, -1) )
        {
            lua_pop( LS, 1 );
            break;
        }
        if ( !load_char_obj(&d, check_string(LS, -1, MIL), TRUE) )
        {
            lua_pop( LS, 1 );
            continue;
            //luaL_error( LS, "Couldn't load '%s'", check_string( LS, -1, MIL) );
        }
        lua_pop( LS, 1 );

        d.character->next=clt_list;
        clt_list=d.character;

        d.character->desc=NULL;
        reset_char(d.character);
        if (!push_CH(LS, d.character) )
            luaL_error( LS, "Couldn't make UD for %s", d.character->name);
        lua_rawseti( LS, -2, newindex++);
    }
    
    lua_remove( LS, -2 ); /* kill name table */
    stackDump(LS);

    lua_call( LS, 2, 0 );
    return 0;

}
        
DEF_DO_FUN(do_charloadtest)
{
    char arg1[MIL];
    char arg2[MIL];
    const char *code=NULL;

    argument=one_argument(argument, arg1);
    argument=one_argument(argument, arg2);

    if (is_number(arg1))
    {
        PROG_CODE *prg=get_mprog_index( atoi(arg1));
        if (!prg)
        {
            ptc( ch, "No such mprog: %s\n\r", arg1 );
            return;
        }
        code=prg->code;
    }
    else
    {
        ptc(ch, "'%s' is not a valid mprog vnum.\n\r", arg1 );
        return;
    }

    char buf[MAX_SCRIPT_LENGTH + MSL ];

    sprintf(buf, "return function (ch, players)\n"
            "%s\n"
            "end",
            code);

    if (luaL_loadstring ( g_mud_LS, buf) ||
            CallLuaWithTraceBack ( g_mud_LS, 0, 1) )
    {
        ptc( ch, "Error loading vnum %s: %s",
                arg1,
                lua_tostring( g_mud_LS, -1));
        lua_settop( g_mud_LS, 0);
        return;
    }

    push_CH( g_mud_LS, ch);
    lua_pushcfunction(g_mud_LS, L_charloadtest);
    lua_insert( g_mud_LS, 1);

    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_charloadtest:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

    /* clean up the stuff */
    CHAR_DATA *tch, *next;
    for ( tch=clt_list ; tch ; tch = next )
    {
        next=tch->next;
        free_char( tch );
    }

    clt_list=NULL;

    return;
}

void show_image_to_char( CHAR_DATA *ch, const char *txt )
{
    if (IS_NPC(ch))
        return;

    if (!ch->pcdata->guiconfig.show_images)
        return;

    if (ch->pcdata->guiconfig.image_window)
    {
        open_imagewin_tag( ch );
    }

    lua_getglobal( g_mud_LS, "show_image_to_char" );
    push_CH( g_mud_LS, ch );
    lua_pushstring( g_mud_LS, txt );
    
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf ( "Error with show_image_to_char:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

    if (ch->pcdata->guiconfig.image_window)
    {
        close_imagewin_tag( ch );
    }
}

const char *save_ptitles( CHAR_DATA *ch )
{
    if (IS_NPC(ch))
        return NULL;

    lua_getglobal(g_mud_LS, "save_ptitles" );
    push_CH(g_mud_LS, ch);
    if (CallLuaWithTraceBack( g_mud_LS, 1, 1 ) )
    {
        ptc (ch, "Error with save_ptitles:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
        return NULL;
    }

    const char *rtn;
    if (lua_isnil(g_mud_LS, -1) || lua_isnone(g_mud_LS, -1) )
    {
        rtn=NULL;
    }
    else if (!lua_isstring(g_mud_LS, -1))
    {
        bugf("String wasn't returned in save_ptitles.");
        rtn=NULL;
    }
    else
    {
        rtn=str_dup(luaL_checkstring( g_mud_LS, -1 ));
    }
   
    lua_pop( g_mud_LS, 1 );

    return rtn;
}

void load_ptitles( CHAR_DATA *ch, const char *text )
{
    lua_getglobal(g_mud_LS, "load_ptitles" );
    push_CH(g_mud_LS, ch);
    lua_pushstring( g_mud_LS, text );

    if (CallLuaWithTraceBack( g_mud_LS, 2, 0 ) )
    {
        ptc (ch, "Error with load_ptitles:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}
const char *save_luaconfig( CHAR_DATA *ch )
{
    if (!IS_IMMORTAL(ch))
        return NULL;

    lua_getglobal(g_mud_LS, "save_luaconfig" );
    push_CH(g_mud_LS, ch);
    if (CallLuaWithTraceBack( g_mud_LS, 1, 1 ) )
    {
        ptc (ch, "Error with save_luaconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
        return NULL;
    }

    const char *rtn;
    if (lua_isnil(g_mud_LS, -1) || lua_isnone(g_mud_LS, -1) )
    {
        rtn=NULL;
    }
    else if (!lua_isstring(g_mud_LS, -1))
    {
        bugf("String wasn't returned in save_luaconfig.");
        rtn=NULL;
    }
    else
    {
        rtn=luaL_checkstring( g_mud_LS, -1 );
    }

    lua_pop( g_mud_LS, 1 );

    return rtn;
}

void load_luaconfig( CHAR_DATA *ch, const char *text )
{
    lua_getglobal(g_mud_LS, "load_luaconfig" );
    push_CH(g_mud_LS, ch);
    lua_pushstring( g_mud_LS, text );

    if (CallLuaWithTraceBack( g_mud_LS, 2, 0 ) )
    {
        ptc (ch, "Error with load_luaconfig:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luaconfig)
{
    lua_getglobal(g_mud_LS, "do_luaconfig");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luaconfig:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

static int L_dump_prog( lua_State *LS)
{
    // 1 is ch
    // 2 is prog 
    // 3 is numberlines
    bool numberlines=lua_toboolean(LS, 3);
    lua_pop(LS,1);


    lua_getglobal( LS, "colorize");
    lua_insert( LS, -2 );
    lua_pushvalue(LS,1); //push a copy of ch
    lua_call( LS, 2, 1 );

    // 1 is ch
    // 2 is colorized text
    if (numberlines)
    {
        lua_getglobal( LS, "linenumber");
        lua_insert( LS, -2);
        lua_call(LS, 1, 1);
    }
    lua_pushstring(LS, "\n\r");
    lua_concat( LS, 2);
    page_to_char_new( 
            luaL_checkstring(LS, 2),
            check_CH(LS, 1),
            TRUE);

    return 0;
}

void dump_prog( CHAR_DATA *ch, const char *prog, bool numberlines)
{
    lua_pushcfunction( g_mud_LS, L_dump_prog);
    push_CH(g_mud_LS, ch);
    lua_pushstring( g_mud_LS, prog);
    lua_pushboolean( g_mud_LS, numberlines);

    if (CallLuaWithTraceBack( g_mud_LS, 3, 0) )
    {
        ptc (ch, "Error with dump_prog:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luareset)
{
    lua_getglobal(g_mud_LS, "do_luareset");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luareset:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_alist)
{
    lua_getglobal(g_mud_LS, "do_alist");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_alist:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_mudconfig)
{
    lua_getglobal(g_mud_LS, "do_mudconfig");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_mudconfig:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_perfmon)
{
    lua_getglobal(g_mud_LS, "do_perfmon");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_perfmon:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void lua_log_perf( double value )
{
    lua_getglobal( g_mud_LS, "log_perf" );
    lua_pushnumber( g_mud_LS, value );
    lua_call( g_mud_LS, 1, 0 );
}

DEF_DO_FUN(do_findreset)
{
    lua_getglobal(g_mud_LS, "do_findreset");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_findreset:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_diagnostic)
{
    lua_getglobal(g_mud_LS, "do_diagnostic");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_diagnostic:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void do_achievements_boss( CHAR_DATA *ch, CHAR_DATA *vic )
{
    lua_getglobal(g_mud_LS, "do_achievements_boss");
    push_CH(g_mud_LS, ch);
    push_CH(g_mud_LS, vic);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf( "Error with do_achievements_boss:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}
void do_achievements_boss_reward( CHAR_DATA *ch )
{
    lua_getglobal(g_mud_LS, "do_achievements_boss_reward");
    push_CH(g_mud_LS, ch);
    if (CallLuaWithTraceBack( g_mud_LS, 1, 0) )
    {
        bugf( "Error with do_achievements_boss_reward:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void update_bossachv_table()
{
    lua_getglobal(g_mud_LS, "update_bossachv_table" );
    
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf( "Error with update_bossachv_table:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void check_lua_stack()
{
    int top=lua_gettop( g_mud_LS );
    if ( top > 0 )
    {
        bugf("%d items left on Lua stack. Clearing.", top );
        lua_settop( g_mud_LS, 0);
    }
}

DEF_DO_FUN(do_path)
{
    lua_getglobal(g_mud_LS, "do_path");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_path:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_luahelp)
{
    lua_getglobal(g_mud_LS, "do_luahelp");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        ptc (ch, "Error with do_luahelp:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_ptitle)
{
    lua_getglobal(g_mud_LS, "do_ptitle");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf( "Error with do_ptitle:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void quest_buy_ptitle( CHAR_DATA *ch, const char *argument)
{
    lua_getglobal(g_mud_LS, "quest_buy_ptitle");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf( "Error with quest_buy_ptitle:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

void fix_ptitles( CHAR_DATA *ch)
{
    lua_getglobal(g_mud_LS, "fix_ptitles");
    push_CH(g_mud_LS, ch);
    
    if (CallLuaWithTraceBack( g_mud_LS, 1, 0) )
    {
        bugf( "Error with fix_ptitles:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

DEF_DO_FUN(do_changelog)
{
    lua_getglobal(g_mud_LS, "do_changelog");
    push_CH(g_mud_LS, ch);
    lua_pushstring(g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf("Error with do_changelog:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}
void save_changelog()
{
    lua_getglobal( g_mud_LS, "save_changelog");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with save_changelog:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }  
}

void load_changelog()
{
    lua_getglobal( g_mud_LS, "load_changelog");
    if (CallLuaWithTraceBack( g_mud_LS, 0, 0) )
    {
        bugf ( "Error with load_changelog:\n %s",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
}

/* sorted ctable section */
void make_skill_table( lua_State *LS )
{
    int sn;
    int tindex=1;
    lua_newtable( LS );
    for ( sn = 0 ; skill_table[sn].name ; sn++ )
    {
        lua_newtable( LS );
        lua_pushstring( LS, skill_table[sn].name );
        lua_setfield( LS, -2, "name" );

        lua_pushinteger( LS, sn );
        lua_setfield( LS, -2, "index" );

        lua_rawseti( LS, -2, tindex++ );
    }
}

void make_group_table( lua_State *LS )
{
    int sn;
    int tindex=1;
    lua_newtable( LS );
    for ( sn = 0 ; sn < MAX_GROUP ; sn++ )
    {
        lua_newtable( LS );
        lua_pushstring( LS, group_table[sn].name );
        lua_setfield( LS, -2, "name" );

        lua_pushinteger( LS, sn );
        lua_setfield( LS, -2, "index" );

        lua_rawseti( LS, -2, tindex++ );
    }    

}

struct sorted_ctable 
{
    struct sorted_ctable **ptr;
    const void *regkey;
    void (*tablefun)( lua_State *LS);
    const char *sortfun;
};

struct sorted_ctable *sorted_skill_table;
struct sorted_ctable *sorted_group_table;

struct sorted_ctable sorted_ctable_table [] =
{
    { 
        &sorted_skill_table,
        NULL,
        make_skill_table,
        "return function(a,b) return a.name<b.name end" 
    },
    {
        &sorted_group_table,
        NULL,
        make_group_table,
        "return function(a,b) return a.name<b.name end"
    },
    { NULL, NULL, NULL, NULL }
};

void sorted_ctable_init( lua_State *LS )
{
    int i;
    for ( i=0 ; sorted_ctable_table[i].tablefun ; i++ )
    {
        struct sorted_ctable *tbl = &sorted_ctable_table[i];
        *(tbl->ptr) = tbl;
    
        /* make and sort the table */
        tbl->tablefun( LS );

        /* new table should be at -1 */
        lua_getglobal( LS, "table" );
        lua_getfield( LS, -1, "sort" );
        lua_remove( LS, -2 ); /* remove "table" */
        lua_pushvalue(LS, -2 ); /* push the actual table */
        if ( luaL_dostring(LS, tbl->sortfun) )
            luaL_error(LS, "sorted_ctable_init: sortfun error");
        lua_call( LS, 2, 0 );

        /* sorted table should be at -1 */
        /* save the thing for later */
        lua_pushlightuserdata( LS, &(tbl->regkey) );
        lua_pushvalue( LS, -2 ); /* push the table */
        lua_settable( LS, LUA_REGISTRYINDEX );

        /* sorted table should be at -1 again*/
        lua_pop( LS, 1 );
    }
}
/* create and keep a sorted list for skill table in order of skill name
   argument represents the sorted index, return value will be the
   corresponding index in actual skill_table.
   In other words, argument 0 will give first alphabetized skill and
   argument MAX_KILL will give last alphabetized skill */
/* return -1 if not a valid sequence index */

static int sorted_table_seq_lookup( struct sorted_ctable *tbl, int sequence )
{
    sequence++; /* offset by +1 because lua indices start at 1 */
    
    lua_pushlightuserdata( g_mud_LS, &(tbl->regkey) );
    lua_gettable( g_mud_LS, LUA_REGISTRYINDEX );

    if ( lua_isnil( g_mud_LS, -1 ) )
    {
        bugf("Didn't find sorted table in registry.");
        return -1;
    }

    int result;

    lua_rawgeti( g_mud_LS, -1, sequence );
    if ( lua_isnil( g_mud_LS, -1 ) )
    {
        result=-1;
        lua_pop( g_mud_LS, 2 );
    }
    else
    {
        lua_getfield( g_mud_LS, -1, "index" );
        result=luaL_checkinteger( g_mud_LS, -1 );
        lua_pop( g_mud_LS, 3 );
    }

    return result;
}

int name_sorted_skill_table( int sequence )
{
    return sorted_table_seq_lookup( sorted_skill_table, sequence );
}

int name_sorted_group_table( int sequence )
{
    return sorted_table_seq_lookup( sorted_group_table, sequence );
}

/* end sorted ctable section */

/* LUAREF section */

/* called as constructor */
void new_ref( LUAREF *ref )
{
    *ref=LUA_NOREF;
}

/* called as destructor
   only call this when the LUAREF's lifetime is ending,
   otherwise use release_ref to release values */
void free_ref( LUAREF *ref )
{
    if (*ref!=LUA_NOREF)
    {
        luaL_unref( g_mud_LS, LUA_GLOBALSINDEX, *ref );
        *ref=LUA_NOREF;
    }

}

void save_ref( lua_State *LS, int index, LUAREF *ref )
{
    if ( *ref!=LUA_NOREF )
    {
        bugf( "Tried to save over existing ref.");
        return;
    }
    lua_pushvalue(LS, index);
    *ref = luaL_ref( LS, LUA_GLOBALSINDEX );
}

void release_ref( lua_State *LS,  LUAREF *ref )
{
    if ( *ref==LUA_NOREF )
    {
        bugf( "Tried to release bad ref.");
        return;
    }
    luaL_unref( LS, LUA_GLOBALSINDEX, *ref ); 
    *ref=LUA_NOREF;
}

void push_ref( lua_State *LS, LUAREF ref )
{
    lua_rawgeti( LS, LUA_GLOBALSINDEX, ref );
}

bool is_set_ref( LUAREF ref )
{
    return !(ref==LUA_NOREF);
}

/* end LUAREF section */

/* string buffer section */

BUFFER *new_buf()
{
    BUFFER *buffer=alloc_mem(sizeof(BUFFER));
    new_ref( &buffer->table ); 
    new_ref( &buffer->string );

    lua_newtable( g_mud_LS );
    save_ref( g_mud_LS, -1, &buffer->table );    
    lua_pop( g_mud_LS, 1 );
    return buffer;
}

void free_buf(BUFFER *buffer)
{
    free_ref( &buffer->table );
    free_ref( &buffer->string );
    free_mem( buffer, sizeof(BUFFER) );
}

bool add_buf(BUFFER *buffer, const char *string)
{
    if (!is_set_ref( buffer->table ))
    {
        bugf("add_buf called with no ref");
        return FALSE;
    }

    push_ref( g_mud_LS, TABLE_INSERT );
    push_ref( g_mud_LS, buffer->table );
    lua_pushstring( g_mud_LS, string ); 
    lua_call( g_mud_LS, 2, 0 );

    return TRUE;
}

void clear_buf(BUFFER *buffer)
{
    release_ref( g_mud_LS, &buffer->table );
    lua_newtable( g_mud_LS );
    save_ref( g_mud_LS, -1, &buffer->table );
    lua_pop( g_mud_LS, 1 );
}

const char *buf_string(BUFFER *buffer)
{
    push_ref( g_mud_LS, TABLE_CONCAT );
    push_ref( g_mud_LS, buffer->table );
    lua_call( g_mud_LS, 1, 1 );

    /* save the ref to guarantee the string is valid as
       long as the BUFFER is alive */
    save_ref( g_mud_LS, -1, &buffer->string ); 
    const char *rtn=luaL_checkstring( g_mud_LS, -1 );
    lua_pop( g_mud_LS, 1 );
    return rtn;
}
/* end string buffer section*/

void lua_con_handler( DESCRIPTOR_DATA *d, const char *argument )
{
    lua_getglobal( g_mud_LS, "lua_con_handler" );
    push_DESCRIPTOR( g_mud_LS, d);
    lua_pushstring( g_mud_LS, argument);
    if (CallLuaWithTraceBack( g_mud_LS, 2, 0) )
    {
        bugf("Error with lua_con_handler:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }

}

static int L_DO_FUN_caller( lua_State *LS )
{
    DO_FUN *fun=lua_touserdata( LS, 1 );
    CHAR_DATA *ch=check_CH( LS, 2 );
    const char *arg=check_string( LS, 3, MIL );
    fun(ch, arg);
    return 0;
}

/*  confirm_yes_no()

    Have the player confirm an action.
    
    If provided, yes/no callbacks are called upon selection of Y or n by
    the player.  These callbacks must have DO_FUN signature.

    If yes_argument/no_argument are provided, they are used as the 'argument'
    parameter to the respective callback, otherwise an empty string is used.
    
    Example using original do_fun as callback:

    DEF_DO_FUN( test1 )
    {
        if (strcmp(argument, "confirm"))
        {
            send_to_char( "Are you sure you want to do it?\n\r", ch);
            confirm_yes_no( ch->desc, do_test1, "confirm", NULL, NULL);
            return;
        }

        send_to_char( ch, "You did it!\n\r");
    }


    Example using separate callback function:

    DEF_DO_FUN( test1_confirm )
    {
        ptc( ch, "You did it!\n\rHere's your argument: %s\n\r", argument);
        return;
    }

    DEF_DO_FUN( test1 )
    {
        send_to_char( "Are you sure you want to do it?\n\r", ch);
        // forward original argument onto the callback
        confirm_yes_no( ch->desc, do_test1_confirm, argument, NULL, NULL);
        return;
    }

*/
void confirm_yes_no( DESCRIPTOR_DATA *d,
        DO_FUN yes_callback, 
        const char *yes_argument,
        DO_FUN no_callback,
        const char *no_argument)
{
    lua_getglobal( g_mud_LS, "confirm_yes_no");

    lua_pushcfunction( g_mud_LS, L_DO_FUN_caller);

    push_DESCRIPTOR( g_mud_LS, d );

    if (yes_callback)
    {
        lua_pushlightuserdata( g_mud_LS, yes_callback);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (yes_argument)
    {
        lua_pushstring( g_mud_LS, yes_argument);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (no_callback)
    {
        lua_pushlightuserdata( g_mud_LS, no_callback);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (no_argument)
    {
        lua_pushstring( g_mud_LS, no_argument);
    }
    else
    {
        lua_pushnil( g_mud_LS);
    }

    if (CallLuaWithTraceBack( g_mud_LS, 6, 0) )
    {
        bugf("Error with confirm_yes_no:\n %s\n\r",
                lua_tostring(g_mud_LS, -1));
        lua_pop( g_mud_LS, 1);
    }
    return;
}
