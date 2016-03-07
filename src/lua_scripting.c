#include <stdio.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "merc.h"
#include "lua_scripting.h"
#include "lua_romlib.h"
#include "lua_main.h"

#define MOB_ARG "mob"
#define NUM_MPROG_ARGS 7 
#define CH_ARG "ch"
#define OBJ1_ARG "obj1"
#define OBJ2_ARG "obj2"
#define TRIG_ARG "trigger"
#define TEXT1_ARG "text1"
#define TEXT2_ARG "text2"
#define VICTIM_ARG "victim"



static void script_error(char *fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    log_string(buf);
    wiznet(buf, NULL, NULL, WIZ_LUAERROR, 0, 0);
}

bool lua_load_mprog( lua_State *LS, int vnum, const char *code)
{
    char buf[MAX_SCRIPT_LENGTH + MSL]; /* Allow big strings from loadscript */

    if ( strlen(code) >= MAX_SCRIPT_LENGTH )
    {
        script_error("MPROG script %d exceeds %d characters.",
                vnum, MAX_SCRIPT_LENGTH);
        return FALSE;
    }

    sprintf(buf, "return function (%s,%s,%s,%s,%s,%s,%s)"
            "%s\n"
            "end",
            CH_ARG, OBJ1_ARG, OBJ2_ARG, TRIG_ARG,
            TEXT1_ARG, TEXT2_ARG, VICTIM_ARG,
            code);


    if (luaL_loadstring ( LS, buf) ||
            CallLuaWithTraceBack ( LS, 0, 1))
    {
        script_error( "LUA mprog error loading vnum %d:\n %s",
                vnum,
                lua_tostring( LS, -1));

        return FALSE;
    }
    else return TRUE;
}

void check_mprog( lua_State *LS, int vnum, const char *code )
{
    if (lua_load_mprog( LS, vnum, code ))
        lua_pop(LS, 1);
}

/* lua_mob_program
   lua equivalent of program_flow
 */
void lua_mob_program( const char *text, int pvnum, const char *source, 
        CHAR_DATA *mob, CHAR_DATA *ch, 
        const void *arg1, sh_int arg1type, 
        const void *arg2, sh_int arg2type,
        int trig_type,
        int security ) 

{
    lua_getglobal( g_mud_LS, "mob_program_setup");

    if ( !push_CH( g_mud_LS, mob ) )
    {
        /* Most likely failed because the gobj was destroyed */
        return;
    }
    if (lua_isnil(g_mud_LS, -1) )
    {
        script_error("push_CH pushed nil to lua_mob_program");
        return;
    }

    if ( !lua_load_mprog( g_mud_LS, pvnum, source) )
    {
        return;
    }

    int error=CallLuaWithTraceBack (g_mud_LS, 2, 1) ;
    if (error > 0 )
    {
        script_error( "LUA error for mob_program_setup:\n %s",
                lua_tostring(g_mud_LS, -1));
    } 

    /* CH_ARG */
    if ( !(ch && push_CH( g_mud_LS, ch)))
        lua_pushnil(g_mud_LS);

    /* TRIG_ARG */
    if (text)
        lua_pushstring ( g_mud_LS, text);
    else lua_pushnil(g_mud_LS);

    /* OBJ1_ARG */
    if ( !((arg1type== ACT_ARG_OBJ && arg1) 
                && push_OBJ(g_mud_LS, (OBJ_DATA*)arg1)))
        lua_pushnil(g_mud_LS);

    /* OBJ2_ARG */
    if ( !((arg2type== ACT_ARG_OBJ && arg2)
                && push_OBJ( g_mud_LS, (OBJ_DATA*)arg2)))
        lua_pushnil(g_mud_LS);

    /* TEXT1_ARG */
    if (arg1type== ACT_ARG_TEXT && arg1)
        lua_pushstring ( g_mud_LS, (char *)arg1);
    else lua_pushnil(g_mud_LS);

    /* TEXT2_ARG */
    if (arg2type== ACT_ARG_TEXT && arg2)
        lua_pushstring ( g_mud_LS, (char *)arg2);
    else lua_pushnil(g_mud_LS);

    /* VICTIM_ARG */
    if ( !((arg2type== ACT_ARG_CHARACTER && arg2)
                && push_CH( g_mud_LS, (CHAR_DATA*)arg2)) )
        lua_pushnil(g_mud_LS);


    /* some snazzy stuff to prevent crashes and other bad things*/
    bool nest=g_LuaScriptInProgress;
    if ( !nest )
    {
        g_LoopCheckCounter=0;
        g_LuaScriptInProgress=TRUE;
        g_ScriptSecurity=security;
    }

    error=CallLuaWithTraceBack (g_mud_LS, NUM_MPROG_ARGS, 0) ;
    if (error > 0 )
    {
        script_error( "LUA mprog error for %s(%d), mprog %d:\n %s",
                mob->name,
                mob->pIndexData ? mob->pIndexData->vnum : 0,
                pvnum,
                lua_tostring(g_mud_LS, -1));
    }

    if ( !nest )
    {
        g_LuaScriptInProgress=FALSE;
        lua_settop (g_mud_LS, 0);    /* get rid of stuff lying around */
        g_ScriptSecurity=SEC_NOSCRIPT; /*just in case*/
    }
}



