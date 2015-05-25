#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <lualib.h>
#include <lauxlib.h>
#include "merc.h"
#include "lua_arclib.h"
#include "lua_main.h"
#include "olc.h"
#include "tables.h"
#include "mob_cmds.h"
#include "interp.h"

/* for iterating */
LUA_OBJ_TYPE *type_list [] =
{
    &CH_type,
    &OBJ_type,
    &AREA_type,
    &ROOM_type,
    &EXIT_type,
    &RESET_type,
    &OBJPROTO_type,
    &MOBPROTO_type,
    &SHOP_type,
    &AFFECT_type,
    &MPROG_type,
    &MTRIG_type,
    &HELP_type,
    &DESCRIPTOR_type,
    NULL
};

/* Define game object types and global functions */

#define GETP(type, field, sec ) { \
    #field , \
    type ## _get_ ## field, \
    sec,  \
    STS_ACTIVE}

#define SETP(type, field, sec) { \
    #field, \
    type ## _set_ ## field, \
    sec, \
    STS_ACTIVE}

#define METH(type, field, sec) { \
    #field, \
    type ## _ ## field, \
    sec, \
    STS_ACTIVE}

#define CHGET( field, sec ) GETP( CH, field, sec)
#define CHSET( field, sec ) SETP( CH, field, sec)
#define CHMETH( field, sec ) METH( CH, field, sec)

#define OBJGET( field, sec ) GETP( OBJ, field, sec)
#define OBJSET( field, sec ) SETP( OBJ, field, sec)
#define OBJMETH( field, sec ) METH( OBJ, field, sec)

#define AREAGET( field, sec ) GETP( AREA, field, sec)
#define AREASET( field, sec ) SETP( AREA, field, sec)
#define AREAMETH( field, sec ) METH( AREA, field, sec)

#define ROOMGET( field, sec ) GETP( ROOM, field, sec)
#define ROOMSET( field, sec ) SETP( ROOM, field sec)
#define ROOMMETH( field, sec ) METH( ROOM, field, sec)

#define OPGET( field, sec ) GETP( OBJPROTO, field, sec)
#define OPSET( field, sec ) SETP( OBJPROTO, field sec)
#define OPMETH( field, sec ) METH( OBJPROTO, field, sec)

#define MPGET( field, sec ) GETP( MOBPROTO, field, sec)
#define MPSET( field, sec ) SETP( MOBPROTO, field sec)
#define MPMETH( field, sec ) METH( MOBPROTO, field, sec)

#define EXGET( field, sec ) GETP( EXIT, field, sec)
#define EXSET( field, sec ) SETP( EXIT, field sec)
#define EXMETH( field, sec ) METH( EXIT, field, sec)

#define RSTGET( field, sec ) GETP( RESET, field, sec)
#define RSTSET( field, sec ) SETP( RESET, field sec)
#define RSTMETH( field, sec ) METH( RESET, field, sec)

#define MPROGGET( field, sec) GETP( MPROG, field, sec)

#define MTRIGGET( field, sec) GETP( MTRIG, field, sec)

#define SHOPGET( field, sec) GETP( SHOP, field, sec)
#define SHOPMETH( field, sec) METH( SHOP, field, sec)

#define AFFGET( field, sec) GETP( AFFECT, field, sec)


typedef struct lua_prop_type
{
    const char *field;
    int  (*func)();
    int security;
    int status; 
} LUA_PROP_TYPE;

#define STS_ACTIVE     0
#define STS_DEPRECATED 1

#define ENDPTABLE {NULL, NULL, 0, 0}

/* global section */
typedef struct glob_type
{
    const char *lib;
    const char *name;
    int (*func)();
    int security; /* if SEC_NOSCRIPT then not available in prog scripts */
    int status;
} GLOB_TYPE;

struct glob_type glob_table[];

#if 0
static int utillib_func (lua_State *LS, const char *funcname)
{
    int narg=lua_gettop(LS);
    lua_getglobal( LS, "glob_util");
    lua_getfield( LS, -1, funcname);
    lua_remove( LS, -2 );
    lua_insert( LS, 1 );
    lua_call( LS, narg, LUA_MULTRET );

    return lua_gettop(LS);
}

static int utillib_trim (lua_State *LS )
{
    return utillib_func( LS, "trim");
}

static int utillib_convert_time (lua_State *LS )
{
    return utillib_func( LS, "convert_time");
}

static int utillib_capitalize( lua_State *LS )
{
    return utillib_func( LS, "capitalize");
}

static int utillib_pluralize( lua_State *LS )
{
    return utillib_func( LS, "pluralize");
}

static int utillib_format_list( lua_State *LS )
{
    return utillib_func( LS, "format_list");
}

static int utillib_strlen_color( lua_State *LS )
{
   lua_pushinteger( LS,
           strlen_color( luaL_checkstring( LS, 1) ) );
   return 1;
}

static int utillib_truncate_color_string( lua_State *LS )
{
    lua_pushstring( LS,
            truncate_color_string( 
                check_string( LS, 1, MSL),
                luaL_checkinteger( LS, 2 )
            ) 
    );
    return 1;
}

static int utillib_format_color_string( lua_State *LS )
{
    lua_pushstring( LS,
            format_color_string(
                check_string( LS, 1, MSL),
                luaL_checkinteger( LS, 2 )
            )
    );
    return 1;
}

#endif

static int glob_sendtochar (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *msg=check_fstring( LS, 2, MSL);

    send_to_char(msg, ch);
    return 0;
}

static int glob_echoat (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *msg=check_fstring( LS, 2, MSL);

    send_to_char(msg, ch);
    send_to_char("\n\r",ch);
    return 0;
}

static int glob_echoaround (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *msg=check_fstring( LS, 2, MSL);

    CHAR_DATA *tochar, *next_char;

    for ( tochar=ch->in_room->people; tochar ; tochar=next_char )
    {
        next_char=tochar->next_in_room;
        if ( tochar == ch )
            continue;

        send_to_char( msg, tochar );
        send_to_char( "\n\r", tochar);
    }

    return 0;
}

static int glob_getglobals (lua_State *LS)
{
    int i;
    int index=1;
    lua_newtable( LS );

    for ( i=0 ; glob_table[i].name ; i++ )
    {
        if ( glob_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS );
            
            if (glob_table[i].lib)
            {
                lua_pushstring( LS, glob_table[i].lib );
                lua_setfield( LS, -2, "lib" );
            }

            lua_pushstring( LS, glob_table[i].name );
            lua_setfield( LS, -2, "name" );

            lua_pushinteger( LS, glob_table[i].security );
            lua_setfield( LS, -2, "security" );

            lua_rawseti( LS, -2, index++ );
        }
    }
    return 1;
}

static int glob_forceget (lua_State *LS)
{
    lua_getmetatable( LS, 1);
    lua_getfield( LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );
    lua_pop( LS, 2 );
    const char *arg=check_string( LS, 2, MIL);
    lua_remove( LS, 2 );

    
    const LUA_PROP_TYPE *get=type->get_table;
    int i;

    for (i=0 ; get[i].field ; i++ )
    {
        if ( !strcmp(get[i].field, arg) )
        {
            return (get[i].func)(LS);
        }
    }

    luaL_error( LS, "Can't get field '%s' for type %s.", arg, type->type_name);
    return 0;
}

static int glob_forceset (lua_State *LS)
{
    lua_getmetatable( LS, 1);
    lua_getfield( LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );
    lua_pop( LS, 2 );
    const char *arg=check_string( LS, 2, MIL);
    lua_remove( LS, 2 );

    
    const LUA_PROP_TYPE *set=type->set_table;
    int i;

    for (i=0 ; set[i].field ; i++ )
    {
        if ( !strcmp(set[i].field, arg) )
        {
            lua_pushcfunction( LS, set[i].func );
            lua_insert( LS, 1 );
            lua_call(LS, 2, 0);
            return 0;
        }
    }

    luaL_error( LS, "Can't set field '%s' for type %s.", arg, type->type_name);
    return 0;
}

static int glob_getluatype (lua_State *LS)
{
    if ( lua_isnone( LS, 1 ) )
    {
        /* Send a list of types */
        lua_newtable( LS );
        int i;
        int index=1;
        for ( i=0 ; type_list[i] ; i++ )
        {
            lua_pushstring( LS, type_list[i]->type_name );
            lua_rawseti( LS, -2, index++ );
        }
        return 1;
    }

    const char *arg1=luaL_checkstring( LS, 1 );
    int i;

    LUA_OBJ_TYPE *tp=NULL;
    for ( i=0 ; type_list[i] ; i++ )
    {
        if (!str_cmp( arg1, type_list[i]->type_name ) )
        {
            tp=type_list[i];
        }
    } 

    if (!tp)
        return 0;

    lua_newtable( LS ); /* main table */

    int index=1;
    lua_newtable( LS ); /* get table */
    for ( i=0 ; tp->get_table[i].field ; i++ )
    {
        if ( tp->get_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS ); /* get entry */
            lua_pushstring( LS, tp->get_table[i].field );
            lua_setfield( LS, -2, "field" );
            lua_pushinteger( LS, tp->get_table[i].security );
            lua_setfield( LS, -2, "security" );
            
            lua_rawseti( LS, -2, index++ );
        }
    }
    lua_setfield( LS, -2, "get" );


    index=1;
    lua_newtable( LS ); /* set table */
    for ( i=0 ; tp->set_table[i].field ; i++ )
    {
        if ( tp->set_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS ); /* get entry */
            lua_pushstring( LS, tp->set_table[i].field );
            lua_setfield( LS, -2, "field" );
            lua_pushinteger( LS, tp->set_table[i].security );
            lua_setfield( LS, -2, "security" );

            lua_rawseti( LS, -2, index++ );
        }
    }
    lua_setfield( LS, -2, "set" );

    index=1;
    lua_newtable( LS ); /* method table */
    for ( i=0 ; tp->method_table[i].field ; i++ )
    {
        if ( tp->method_table[i].status == STS_ACTIVE )
        {
            lua_newtable( LS ); /* get entry */
            lua_pushstring( LS, tp->method_table[i].field );
            lua_setfield( LS, -2, "field" );
            lua_pushinteger( LS, tp->method_table[i].security );
            lua_setfield( LS, -2, "security" );
            
            lua_rawseti( LS, -2, index++ );
        }
    }
    lua_setfield( LS, -2, "method" );

    return 1;
}

static int glob_clearloopcount (lua_State *LS)
{
    g_LoopCheckCounter=0;
    return 0;
}

static int glob_log (lua_State *LS)
{
    char buf[MSL];
    sprintf(buf, "LUA::%s", check_fstring( LS, 1, MIL));

    log_string(buf);
    return 0;
}

static int glob_hour (lua_State *LS)
{
    lua_pushnumber( LS, time_info.hour );
    return 1;
}

static int glob_gettime (lua_State *LS)
{
    char buf[MSL];
    struct timeval t;
    gettimeofday( &t, NULL);

    sprintf(buf, "%ld.%ld", (long)t.tv_sec, (long)t.tv_usec);
    lua_pushstring( LS, buf);
   
    lua_pushnumber( LS, lua_tonumber( LS, -1 ) );
    return 1;
}

static int glob_getroom (lua_State *LS)
{
    // do some if is number thing here eventually
    int num = (int)luaL_checknumber (LS, 1);

    ROOM_INDEX_DATA *room=get_room_index(num);

    if (!room)
        return 0;

    if ( !push_ROOM( LS, room) )
        return 0;
    else
        return 1;

}

static int glob_getobjproto (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    OBJ_INDEX_DATA *obj=get_obj_index(num);

    if (!obj)
        return 0;

    if ( !push_OBJPROTO( LS, obj) )
        return 0;
    else
        return 1;
}

static int glob_getobjworld (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    OBJ_DATA *obj;

    int index=1;
    lua_newtable(LS);
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( obj->pIndexData->vnum == num )
        {
            if (push_OBJ( LS, obj))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int glob_getmobproto (lua_State *LS)
{
    int num = luaL_checknumber (LS, 1);

    MOB_INDEX_DATA *mob=get_mob_index(num);

    if (!mob)
        return 0;

    if ( !push_MOBPROTO( LS, mob) )
        return 0;
    else
        return 1;
}

static int glob_getmobworld (lua_State *LS)
{
    int num = (int)luaL_checknumber (LS, 1);

    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
        if ( ch->pIndexData )
        {
            if ( ch->pIndexData->vnum == num )
            {
                if (push_CH( LS, ch))
                    lua_rawseti(LS, -2, index++);
            }
        }
    }
    return 1;
}

static int glob_getpc (lua_State *LS)
{
    const char *name=check_string (LS, 1, MIL );
    
    CHAR_DATA *ch;
    for (ch=char_list; ch; ch=ch->next)
    {
        if (IS_NPC(ch))
            continue;

        if (!str_cmp(name, ch->name))
        {
            push_CH(LS, ch);
            return 1;
        }
    }

    return 0;
}

static int glob_pagetochar (lua_State *LS)
{
    if (!lua_isnone(LS, 3) )
    {
        page_to_char_new( 
                luaL_checkstring(LS, 2),
                check_CH(LS,1),
                lua_toboolean( LS, 3 ) );
        return 0;
    }
    else
    {
        page_to_char( 
                luaL_checkstring( LS, 2),
                check_CH(LS,1) );
    }

    return 0;
}

static int glob_getobjlist (lua_State *LS)
{
    OBJ_DATA *obj;

    int index=1;
    lua_newtable(LS);

    for ( obj=object_list ; obj ; obj=obj->next )
    {
        if (push_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getcharlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if (push_CH(LS, ch))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getmoblist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( IS_NPC(ch) )
        {
            if (push_CH(LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}

static int glob_getdescriptorlist (lua_State *LS)
{
    DESCRIPTOR_DATA *d;

    int index=1;
    lua_newtable(LS);

    for ( d=descriptor_list ; d ; d=d->next )
    {
        if (push_DESCRIPTOR(LS, d))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getplayerlist (lua_State *LS)
{
    CHAR_DATA *ch;

    int index=1;
    lua_newtable(LS);

    for ( ch=char_list ; ch ; ch=ch->next )
    {
        if ( !IS_NPC(ch) )
        {
            if (push_CH(LS, ch))
                lua_rawseti(LS, -2, index++);
        }
    }

    return 1;
}

static int glob_getarealist (lua_State *LS)
{
    AREA_DATA *area;

    int index=1;
    lua_newtable(LS);

    for ( area=area_first ; area ; area=area->next )
    {
        if (push_AREA(LS, area))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_getshoplist ( lua_State *LS)
{
    SHOP_DATA *shop;
    
    int index=1;
    lua_newtable(LS);

    for ( shop=shop_first ; shop ; shop=shop->next )
    {
        if (push_SHOP(LS, shop))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int glob_gethelplist ( lua_State *LS )
{
    HELP_DATA *help;

    int index=1;
    lua_newtable(LS);

    for ( help=help_first ; help ; help=help->next )
    {
        if (push_HELP(LS, help))
            lua_rawseti(LS, -2, index++);
    }

    return 1;
}

static int mudlib_luadir( lua_State *LS)
{
    lua_pushliteral( LS, LUA_DIR);
    return 1;
}

/* return tprintstr of the given global (string arg)*/
static int dbglib_show ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);
    lua_getglobal( LS, luaL_checkstring( LS, 1 ) );
    lua_call( LS, 1, 1 );

    return 1;
}

static int glob_getrandomroom ( lua_State *LS)
{
    ROOM_INDEX_DATA *room;

    int i;
    for ( i=0; i<10000; i++ ) // limit to 10k loops just in case
    {
        room=get_room_index(number_range(0,65535));
        if ( ( room )
                &&   !room_is_private(room)
                &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
                &&   !IS_SET(room->room_flags, ROOM_SOLITARY)
                &&   !IS_SET(room->room_flags, ROOM_SAFE) )
            break;
    }

    if (!room)
        luaL_error(LS, "Couldn't get a random room.");

    if (push_ROOM(LS,room))
        return 1;
    else
        return 0;

}

        
#define ENDGTABLE { NULL, NULL, NULL, 0, 0 }
#define GFUN( fun, sec ) { NULL, #fun , glob_ ## fun , sec, STS_ACTIVE }
#define LFUN( lib, fun, sec) { #lib, #fun, lib ## lib_ ## fun , sec, STS_ACTIVE}
#define DBGF( fun ) LFUN( dbg, fun, 9 )

GLOB_TYPE glob_table[] =
{
    GFUN(hour,          0),
    GFUN(gettime,       0),
    GFUN(getroom,       0),
    GFUN(getobjproto,   0),
    GFUN(getobjworld,   0),
    GFUN(getmobproto,   0),
    GFUN(getmobworld,   0),
    GFUN(getpc,         0),
    GFUN(getrandomroom, 0),
    GFUN(sendtochar,    0),
    GFUN(echoat,        0),
    GFUN(echoaround,    0),
    GFUN(pagetochar,    0),
    GFUN(log,           0),
    GFUN(getcharlist,   9),
    GFUN(getobjlist,    9),
    GFUN(getmoblist,    9),
    GFUN(getplayerlist, 9),
    GFUN(getarealist,   9),
    GFUN(getshoplist,   9),
    GFUN(gethelplist,   9),
    GFUN(getdescriptorlist, 9),
    GFUN(clearloopcount,9),
    GFUN(forceget,      SEC_NOSCRIPT),
    GFUN(forceset,      SEC_NOSCRIPT),
    GFUN(getluatype,    SEC_NOSCRIPT),
    GFUN(getglobals,    SEC_NOSCRIPT),

    DBGF(show),

    /* SEC_NOSCRIPT means aren't available for prog scripts */

    LFUN( mud, luadir,      SEC_NOSCRIPT ),
    
    ENDGTABLE
};

static int global_sec_check (lua_State *LS)
{
    int security=luaL_checkinteger( LS, lua_upvalueindex(1) );
    
    if ( g_ScriptSecurity < security )
        luaL_error( LS, "Current security %d. Function requires %d.",
                g_ScriptSecurity,
                security);

    int (*fun)()=lua_tocfunction( LS, lua_upvalueindex(2) );

    return fun(LS);
}

#define SCRIPT_GLOBS_TABLE "script_globs"
/* Register funcs globally then if needed,
   security closure into script_globs
   to be inserted into main_lib in startup.lua */
/* would be nice to do this in lua but we run it before startup */
void register_globals( lua_State *LS )
{
    //if (1) return;
    int top=lua_gettop(LS); 
    int i;

    /* create script_globs */
    lua_newtable(LS);
    lua_setglobal(LS, SCRIPT_GLOBS_TABLE );

    for ( i=0 ; glob_table[i].name ; i++ )
    {
        /* is it a lib thing? */
        if ( glob_table[i].lib )
        {
            lua_getglobal( LS, glob_table[i].lib );
            if ( lua_isnil( LS, -1 ) )
            {
                lua_pop(LS, 1); /* kill the nil */
                lua_newtable( LS );
                lua_pushvalue( LS, -1 ); /* make a copy cause we poppin it */
                lua_setglobal( LS, glob_table[i].lib );
                
            }
            lua_pushcfunction( LS, glob_table[i].func );
            lua_setfield( LS, -2, glob_table[i].name );
            lua_pop(LS, 1); // kill lib table
        }
        else
        {
            lua_getglobal( LS, glob_table[i].name );
            if (!lua_isnil( LS, -1 ) )
            {
                luaL_error( LS, "Global already exists: %s",
                        glob_table[i].name);
            }
            else
                lua_pop(LS, 1); /* kill the nil */
            
            lua_pushcfunction( LS, glob_table[i].func );
            lua_setglobal( LS, glob_table[i].name );

        }


        if (glob_table[i].security == SEC_NOSCRIPT)
            continue; /* don't add to script_globs */ 

        
        /* get script_globs */
        lua_getglobal( LS, SCRIPT_GLOBS_TABLE );
        if ( glob_table[i].lib )
        {
            lua_getfield( LS, -1, glob_table[i].lib );
            if ( lua_isnil( LS, -1 ) )
            {
                lua_pop(LS, 1); // kill the nil
                lua_newtable(LS);
                lua_pushvalue( LS, -1); //make a copy because
                lua_setfield( LS, -3, glob_table[i].lib );
            }
        }


        /* create the security closure */
        lua_pushinteger( LS, glob_table[i].security );
        lua_pushcfunction( LS, glob_table[i].func );
        lua_pushcclosure( LS, global_sec_check, 2 );
        
        /* set as field to script_globs script_globs.lib */
        lua_setfield( LS, -2, glob_table[i].name );

        lua_settop(LS, top); // clearn junk after each loop
    }

}



/* end global section */

/* common section */
static int set_flag( lua_State *LS,
        const char *funcname, 
        const struct flag_type *flagtbl, 
        long *flagvar )
{
    const char *argument = check_string( LS, 2, MIL);
    bool set = TRUE;
    if (!lua_isnone( LS, 3 ) )
    {
        set = lua_toboolean( LS, 3 );
    }
    
    int flag=flag_lookup( argument, flagtbl );
    if ( flag == NO_FLAG )
        luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);
        
    if ( set )
        SET_BIT( *flagvar, flag );
    else
        REMOVE_BIT( *flagvar, flag);
        
    return 0;
}

static int check_flag( lua_State *LS, 
        const char *funcname, 
        const struct flag_type *flagtbl, 
        long flagvar)
{
    if (lua_isnone( LS, 2)) /* called with no string arg */
    {
        /* return array of currently set flags */
        int index=1;
        lua_newtable( LS );
        int i;
        for ( i=0 ; flagtbl[i].name ; i++)
        {

            if ( IS_SET(flagvar, flagtbl[i].bit) )
            {
                lua_pushstring( LS, flagtbl[i].name);
                lua_rawseti(LS, -2, index++);
            }
        }
        return 1;
    }
    
    const char *argument = check_fstring( LS, 2, MIL);
    int flag=NO_FLAG;
       
    if ((flag=flag_lookup(argument, flagtbl)) == NO_FLAG)
        luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);
    
    lua_pushboolean( LS, IS_SET( flagvar, flag ) );

    return 1;
}

/* macro the heck out of this stuff so we don't have to rewrite for OBJ_DATA and OBJ_INDEX_DATA */
#define OBJVGT( funcname, funcbody ) \
static int OBJ_get_ ## funcname (lua_State *LS)\
{\
    OBJ_DATA *ud_obj=check_OBJ(LS,1);\
    \
    funcbody \
}\
\
static int OBJPROTO_get_ ## funcname (lua_State *LS)\
{\
    OBJ_INDEX_DATA *ud_obj=check_OBJPROTO(LS,1);\
    \
    funcbody \
}

#define OBJVGETINT( funcname, otype, vind ) \
OBJVGT( funcname, \
    if (ud_obj->item_type != otype )\
        luaL_error(LS, #funcname " for %s only.", \
                item_name( otype ) );\
    \
    lua_pushinteger( LS, ud_obj->value[ vind ] );\
    return 1;\
)

#define OBJVGETSTR( funcname, otype, vval )\
OBJVGT( funcname, \
    if (ud_obj->item_type != otype )\
        luaL_error(LS, #funcname " for %s only.", \
                item_name( otype ) );\
    \
    lua_pushstring( LS, vval );\
    return 1;\
)


OBJVGETINT( light, ITEM_LIGHT, 2 )

OBJVGT( spelllevel,  
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            lua_pushinteger( LS,
                    ud_obj->value[0]);
            return 1;
        default:
            luaL_error(LS, "Spelllevel for wands, staves, scrolls, potions, and pills only.");
    }
    return 0;
)

OBJVGT( chargestotal,
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
            lua_pushinteger( LS,
                    ud_obj->value[1]);
            return 1;
        default:
            luaL_error(LS, "Chargestotal for wands and staves only.");
    }

    return 1;
)

OBJVGT( chargesleft,
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
            lua_pushinteger(LS,
                    ud_obj->value[2]);
            return 1;
        case ITEM_PORTAL:
            lua_pushinteger(LS,
                    ud_obj->value[0]);
            return 1;
        default:
            luaL_error(LS, "Chargesleft for wands, staves, and portals only.");
    }

    return 0;
)

OBJVGT( spellname, 
    switch(ud_obj->item_type)
    {
        case ITEM_WAND:
        case ITEM_STAFF:
            lua_pushstring( LS,
                    ud_obj->value[3] != -1 ? skill_table[ud_obj->value[3]].name
                        : "reserved" );
            return 1; 
        default:
            luaL_error(LS, "Spellname for wands and staves only.");
    }

    return 1;
)

OBJVGETINT( toroom, ITEM_PORTAL, 3 )

OBJVGETINT( maxpeople, ITEM_FURNITURE, 0 )

OBJVGT( maxweight, 
    switch(ud_obj->item_type)
    {
        case ITEM_FURNITURE:
            lua_pushinteger( LS,
                    ud_obj->value[1] );
            return 1;
        case ITEM_CONTAINER:
            lua_pushinteger( LS,
                    ud_obj->value[0] );
            return 1;
        default:
            luaL_error(LS, "Maxweight for furniture and containers only.");
    }

    return 0;
)

OBJVGETINT( healbonus, ITEM_FURNITURE, 3 )

OBJVGETINT( manabonus, ITEM_FURNITURE, 4 )

OBJVGT( spells, 
    switch(ud_obj->item_type)
    {
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            lua_newtable(LS);
            int index=1;
            int i;

            for ( i=1 ; i<5 ; i++ )
            {
                if ( ud_obj->value[i] < 1 )
                    continue;

                lua_pushstring( LS,
                        skill_table[ud_obj->value[i]].name );
                lua_rawseti( LS, -2, index++ );
            } 
            return 1;
        default:
            luaL_error( LS, "Spells for pill, potion, and scroll only.");
    }
    
    return 0;
)

OBJVGETINT( ac, ITEM_ARMOR, 0 )

OBJVGETSTR( weapontype, ITEM_WEAPON,
        flag_bit_name(weapon_class, ud_obj->value[0]) )

OBJVGETINT( numdice, ITEM_WEAPON, 1 )

OBJVGETINT( dicetype, ITEM_WEAPON, 2 )

OBJVGETSTR( attacktype, ITEM_WEAPON, attack_table[ud_obj->value[3]].name )

OBJVGETSTR( damnoun, ITEM_WEAPON, attack_table[ud_obj->value[3]].noun )

OBJVGETINT( key, ITEM_CONTAINER, 2 )

OBJVGETINT( capacity, ITEM_CONTAINER, 3 )

OBJVGETINT( weightmult, ITEM_CONTAINER, 4 )

OBJVGT( liquidtotal, 
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushinteger( LS, ud_obj->value[0] );
            return 1;
        default:
            luaL_error(LS, "liquidtotal for drink and fountain only");
    }

    return 0;
)

OBJVGT( liquidleft,
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushinteger( LS, ud_obj->value[1] );
            return 1;
        default:
            luaL_error(LS, "liquidleft for drink and fountain only");
    }

    return 0;
)

OBJVGT( liquid,
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushstring( LS,
                    liq_table[ud_obj->value[2]].liq_name);
            return 1;
        default:
            luaL_error(LS, "liquid for drink and fountain only");
    }

    return 0;
)

OBJVGT( liquidcolor,
    switch(ud_obj->item_type)
    {
        case ITEM_FOUNTAIN:
        case ITEM_DRINK_CON:
            lua_pushstring( LS,
                    liq_table[ud_obj->value[2]].liq_color);
            return 1;
        default:
            luaL_error(LS, "liquidcolor for drink and fountain only");
    }

    return 0;
)

OBJVGT( poisoned, 
    switch(ud_obj->item_type)
    {
        case ITEM_DRINK_CON:
        case ITEM_FOOD:
            lua_pushboolean( LS, ud_obj->value[3] );
            return 1;
        default:
            luaL_error(LS, "poisoned for drink and food only");
    }

    return 0;
)

OBJVGETINT( foodhours, ITEM_FOOD, 0 )

OBJVGETINT( fullhours, ITEM_FOOD, 1 )

OBJVGETINT( silver, ITEM_MONEY, 0 )

OBJVGETINT( gold, ITEM_MONEY, 1 )

#define OBJVM( funcname, body ) \
static int OBJ_ ## funcname ( lua_State *LS )\
{\
    OBJ_DATA *ud_obj=check_OBJ(LS,1);\
    body \
}\
static int OBJPROTO_ ## funcname ( lua_State *LS )\
{\
    OBJ_INDEX_DATA *ud_obj=check_OBJPROTO(LS,1);\
    body \
}

#define OBJVIF( funcname, otype, vind, flagtbl ) \
OBJVM( funcname, \
    if (ud_obj->item_type != otype)\
        luaL_error( LS, #funcname " for %s only", item_name( otype ) );\
    \
    return check_flag( LS, #funcname, flagtbl, ud_obj->value[ vind ] );\
)

OBJVM( apply,
    const char *type=check_string(LS,2,MIL);
    AFFECT_DATA *pAf;
    for (pAf=ud_obj->affected ; pAf ; pAf=pAf->next)
    {
        if ( !strcmp(
                flag_bit_name(apply_flags, pAf->location),
                type ) )
        {
            lua_pushinteger( LS, pAf->modifier );
            return 1;
        }
    }
    return 0;
)

OBJVIF ( exitflag, ITEM_PORTAL, 1, exit_flags )

OBJVIF ( portalflag, ITEM_PORTAL, 2, portal_flags )

OBJVIF ( furnitureflag, ITEM_FURNITURE, 2, furniture_flags )

OBJVIF ( weaponflag, ITEM_WEAPON, 4, weapon_type2 )

OBJVIF ( containerflag, ITEM_CONTAINER, 1, container_flags )
/* end common section */

/* CH section */
static int CH_randchar (lua_State *LS)
{
    CHAR_DATA *ch=get_random_char(check_CH(LS,1) );
    if ( ! ch )
        return 0;

    if ( !push_CH(LS,ch))
        return 0;
    else
        return 1;

}

/* analog of run_olc_editor in olc.c */
/*
static bool run_olc_editor_lua( CHAR_DATA *ch, const char *argument )
{
    if (IS_NPC(ch))
        return FALSE;

    switch ( ch->desc->editor )
    {
        case ED_AREA:
            aedit( ch, argument );
            break;
        case ED_ROOM:
            redit( ch, argument );
            break;
        case ED_OBJECT:
            oedit( ch, argument );
            break;
        case ED_MOBILE:
            medit( ch, argument );
            break;
        case ED_MPCODE:
            mpedit( ch, argument );
            break;
        case ED_OPCODE:
            opedit( ch, argument );
            break;
        case ED_APCODE:
            apedit( ch, argument );
            break;
        case ED_RPCODE:
            rpedit( ch, argument );
            break;
        case ED_HELP:
            hedit( ch, argument );
            break;
        default:
            return FALSE;
    }
    return TRUE; 
}

static int CH_olc (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    if (IS_NPC(ud_ch) )
    {
        luaL_error( LS, "NPCs cannot use OLC!");
    }

    if (!run_olc_editor_lua( ud_ch, check_fstring( LS, 2, MIL)) )
        luaL_error(LS, "Not currently in olc edit mode.");

    return 0;
}
*/
static int CH_tprint ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    do_say( ud_ch, check_string(LS, -1, MIL));

    return 0;
}
static int CH_savetbl (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
    {
        luaL_error( LS, "PCs cannot call savetbl.");
        return 0;
    }

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    /* Push original args into SaveTable */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_ch->pIndexData->area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

#if 0
static int CH_loadtbl (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
    {
        luaL_error( LS, "PCs cannot call loadtbl.");
        return 0;
    }

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_ch->pIndexData->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int CH_loadscript (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular mprog with vnum 0*/
    lua_mob_program( NULL, LOADSCRIPT_VNUM, check_string(LS, -1, MAX_SCRIPT_LENGTH), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );

    return 0;
}

static int CH_loadfunction ( lua_State *LS )
{
    lua_mob_program( NULL, RUNDELAY_VNUM, NULL,
                check_CH(LS, -2), NULL,
                NULL, 0, NULL, 0,
                TRIG_CALL, 0 );
    return 0;
}

static int CH_loadstring (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_mob_program( NULL, LOADSCRIPT_VNUM, check_string(LS, 2, MAX_SCRIPT_LENGTH), ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 );
    return 0;
} 

static int CH_loadprog (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pMcode;

    if ( (pMcode = get_mprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: mprog vnum %d doesn't exist", num);
        return 0;
    }

    if ( !pMcode->is_lua)
    {
        luaL_error(LS, "loadprog: mprog vnum %d is not lua code", num);
        return 0;
    }

    lua_mob_program( NULL, num, pMcode->code, ud_ch, NULL, NULL, 0, NULL, 0, TRIG_CALL, 0 ); 

    return 0;
}

#endif
static int CH_emote (lua_State *LS)
{
    do_emote( check_CH(LS, 1), check_fstring( LS, 2, MIL) );
    return 0;
}

static int CH_asound (lua_State *LS)
{
    do_mpasound( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0; 
}

static int CH_zecho (lua_State *LS)
{
    do_mpzecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    return 0;
}

static int CH_kill (lua_State *LS)
{
    do_mpkill( check_CH(LS, 1), check_string( LS, 2, MIL));
    return 0;
}

static int CH_assist (lua_State *LS)
{
    do_mpassist( check_CH(LS, 1), check_string( LS, 2, MIL));
    return 0;
}

static int CH_junk (lua_State *LS)
{
    do_mpjunk( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_echo (lua_State *LS)
{
    do_mpecho( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_purge (lua_State *LS)
{
    // Send empty string for no argument
    if ( lua_isnone( LS, 2) )
    {
        do_mppurge( check_CH(LS, 1), "");
    }
    else
    {
        do_mppurge( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    }

    return 0;
}

static int CH_goto (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    const char *location = check_string(LS,2,MIL);
    bool hidden=FALSE;
    if ( !lua_isnone(LS,3) )
    {
        hidden=lua_toboolean(LS,3);
    }

    do_mpgoto( ud_ch, location);

    if (!hidden)
    {
        do_look( ud_ch, "auto");
    }

    return 0;
}

static int CH_at (lua_State *LS)
{

    do_mpat( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_otransfer (lua_State *LS)
{
    do_mpotransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_force (lua_State *LS)
{

    do_mpforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_gforce (lua_State *LS)
{

    do_mpgforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_vforce (lua_State *LS)
{

    do_mpvforce( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_cast (lua_State *LS)
{

    do_mpcast( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_damage (lua_State *LS)
{
    if ( lua_type(LS, 2 ) == LUA_TSTRING )
    {
        do_mpdamage( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }
    
    CHAR_DATA *ud_ch=check_CH(LS, 1);
    CHAR_DATA *victim=check_CH(LS, 2);
    if (ud_ch->in_room != victim->in_room)
        luaL_error(LS, 
                "Actor and victim must be in same room." );
    int dam=luaL_checkinteger(LS, 3);
    
    int damtype;
    damtype=DAM_NONE;

    lua_pushboolean( LS,
            damage(ud_ch, victim, dam, TYPE_UNDEFINED, damtype, FALSE) );
    return 1;
}

static int CH_remove (lua_State *LS)
{

    do_mpremove( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_mdo (lua_State *LS)
{
    interpret( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_mobhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, atoi(argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_number( argument ) )
        lua_pushboolean( LS,(bool) get_obj_vnum_room( ud_ch, atoi(argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS,(bool) (get_char_world( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, (bool) (get_obj_world( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_get_ispc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && !IS_NPC( ud_ch ) );
    return 1;
}

static int CH_canattack (lua_State *LS)
{
    lua_pushboolean( LS, !is_safe(check_CH (LS, 1), check_CH (LS, 2)) );
    return 1;
}

static int CH_get_isnpc (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_NPC( ud_ch ) );
    return 1;
}

static int CH_get_isgood (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_GOOD( ud_ch ) ) ;
    return 1;
}

static int CH_get_isevil (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_EVIL( ud_ch ) ) ;
    return 1;
}

static int CH_get_isneutral (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean(  LS, ud_ch != NULL && IS_NEUTRAL( ud_ch ) ) ;
    return 1;
}

static int CH_get_isimmort (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_IMMORTAL( ud_ch ) ) ;
    return 1;
}

static int CH_get_ischarm (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && IS_AFFECTED( ud_ch, AFF_CHARM ) ) ;
    return 1;
}

static int CH_get_isfollow (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->master != NULL ) ;
    return 1;
}

static int CH_get_isactive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    lua_pushboolean( LS, ud_ch != NULL && ud_ch->position > POS_SLEEPING ) ;
    return 1;
}

static int CH_cansee (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH(LS, 1);
    CHAR_DATA * ud_vic = check_CH (LS, 2);

    lua_pushboolean( LS, ud_ch != NULL && ud_vic != NULL && can_see( ud_ch, ud_vic ) ) ;

    return 1;
}

static int CH_affected (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string(LS, 2, MIL);

    lua_pushboolean( LS,
            IS_SET( ud_ch->affected_by,
                flag_lookup(argument, act_flags)));
    return 1;
}

static int CH_act (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (IS_NPC(ud_ch))
    {
        return check_flag( LS, "act[NPC]", act_flags, ud_ch->act );
    }
    else
    {
        return check_flag( LS, "act[PC]", plr_flags, ud_ch->act );
    }
}

static int CH_setact (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "act[NPC]", act_flags, &ud_ch->act );
    }
    else
        return luaL_error( LS, "'setact' for NPC only." );

}

static int CH_offensive (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "offensive",off_flags, ud_ch->off_flags );
}

static int CH_immune (lua_State *LS)
{ 
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "immune", imm_flags, ud_ch->imm_flags );
}

static int CH_setimmune (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "immune", imm_flags, &ud_ch->imm_flags );
    }
    else
        return luaL_error( LS, "'setimmune' for NPC only.");

}

static int CH_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string( LS, 2, MIL);

    if ( is_number( argument ) )
    {
        int vnum=atoi( argument );

        lua_pushboolean(LS,
                has_item(ud_ch, vnum, -1, FALSE));
        return 1;
    }
    else
    {
        lua_pushboolean(LS,
                get_obj_carry(ud_ch, argument, ud_ch) != NULL);
        return 1;
    }
}

static int CH_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, atoi(argument), -1, TRUE ) );
    else
        lua_pushboolean( LS, ud_ch != NULL && (get_obj_wear( ud_ch, argument ) != NULL) );

    return 1;
}

static int CH_has (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), FALSE ) );

    return 1;
}

static int CH_uses (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, -1, item_lookup(argument), TRUE ) );

    return 1;
}
static int CH_say (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    do_say( ud_ch, check_fstring( LS, 2, MIL) );
    return 0;
}

#if 0
static int CH_addaffect (lua_State *LS)
{
    int arg_index=1;
    CHAR_DATA * ud_ch = check_CH (LS, arg_index++);
    AFFECT_DATA af;
    const char *temp = NULL;
    const struct flag_type *flag_table;

    /* where */
    temp=check_string(LS,arg_index++,MIL);
    af.where=flag_lookup( temp, apply_types);

    if (af.where==NO_FLAG)
        luaL_error(LS, "No such 'apply_type' flag: %s", temp); 
    else if (af.where != TO_AFFECTS &&
             af.where != TO_IMMUNE &&
             af.where != TO_RESIST &&
             af.where != TO_VULN /* &&
             af.where != TO_SPECIAL*/ /* not supported yet */
            )
        luaL_error(LS, "%s not supported for CH affects.", temp);

    /* type */
    temp=check_string(LS,arg_index++,MIL);
    af.type=skill_lookup( temp );

    if (af.type == -1)
        luaL_error(LS, "Invalid skill: %s", temp);

    /* level */
    af.level=luaL_checkinteger(LS,arg_index++);
    if (af.level<1)
        luaL_error(LS, "Level must be > 0.");

    /* duration */
    af.duration=luaL_checkinteger(LS,arg_index++);
    if (af.duration<-1)
        luaL_error(LS, "Duration must be -1 (indefinite) or greater.");

    /* location */
    switch (af.where)
    {
        case TO_IMMUNE:
        case TO_RESIST:
        case TO_VULN:
            af.location=APPLY_NONE;
            break;
        case TO_AFFECTS:
        {
            temp=check_string(LS,arg_index++,MIL);
            af.location=flag_lookup( temp, apply_flags );
            if (af.location == NO_FLAG)
                luaL_error(LS, "Invalid location: %s", temp);
            break;
        }
        default:
            luaL_error(LS, "Invalid where.");
      
    }

    /* modifier */
    switch (af.where)
    {
        case TO_IMMUNE:
        case TO_RESIST:
        case TO_VULN:
            af.modifier=0;
            break;
        case TO_AFFECTS:
            af.modifier=luaL_checkinteger(LS,arg_index++);
            break;
        default:
            luaL_error(LS, "Invalid where.");
    }

    /* bitvector */
    temp=check_string(LS,arg_index++,MIL);
    if (!strcmp(temp, "none"))
    {
        af.bitvector=0;
    }
    else
    {
        switch (af.where)
        {
            case TO_AFFECTS:
                flag_table=affect_flags;
                break;
            case TO_IMMUNE:
                flag_table=imm_flags;
                break;
            case TO_RESIST:
                flag_table=res_flags;
                break;
            case TO_VULN:
                flag_table=vuln_flags;
                break;
            default:
                return luaL_error(LS, "'where' not supported");
        }
        af.bitvector=flag_lookup( temp, flag_table);
        if (af.bitvector==NO_FLAG)
            luaL_error(LS, "Invalid bitvector: %s", temp);
        else if ( !flag_table[index_lookup(af.bitvector, flag_table)].settable )
            luaL_error(LS, "Flag '%s' is not settable.", temp);
    }

    /* tag (custom_affect only) */
    if (af.type==gsn_custom_affect)
    {
        af.tag=str_dup(check_string(LS,arg_index++,MIL));
    }
    else
        af.tag=NULL;
    
    affect_to_char_tagsafe( ud_ch, &af );

    return 0;
}

static int CH_removeaffect (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    
    if (is_AFFECT(LS,2))
    {
        /* remove a specific affect */
        affect_remove( ud_ch, check_AFFECT(LS,2));
        return 0;
    }

    /* remove by sn */
    const char *skill = check_string(LS, 2, MIL);
    int sn=skill_lookup( skill );
    if (sn==-1)
       luaL_error(LS, "Invalid skill: %s", skill);
    else if (sn==gsn_custom_affect)
    {
        custom_affect_strip( ud_ch, check_string(LS,3,MIL) );
    }
    else
    {
        affect_strip( ud_ch, sn );
    }

    return 0;
} 
#endif
static int CH_oload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex, 0);

    obj_to_char(obj,ud_ch);

    if ( !push_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}

static int CH_destroy (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);

    if (!ud_ch)
    {
        luaL_error(LS, "Null pointer in destroy");
        return 0;
    }

    if (!IS_NPC(ud_ch))
    {
        luaL_error(LS, "Trying to destroy player");
        return 0;
    }

    extract_char(ud_ch,TRUE);
    return 0;
}

static int CH_vuln (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "vuln", vuln_flags, ud_ch->vuln_flags );
}

static int CH_setvuln (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "vuln", vuln_flags, &ud_ch->vuln_flags );
    }
    else
        return luaL_error( LS, "'setvuln' for NPC only." );

}

static int CH_resist (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    return check_flag( LS, "resist", res_flags, ud_ch->res_flags );
}

static int CH_setresist (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        return set_flag( LS, "resist", res_flags, &ud_ch->res_flags );
    }
    else
        return luaL_error( LS, "'setresist' for NPC only.");
}

static int CH_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string( LS, 2, MIL);

    int sn=skill_lookup(argument);
    if (sn==-1)
        luaL_error(LS, "No such skill '%s'", argument);

    int skill=get_skill(ud_ch, sn);
    
    if (skill<1)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }

    lua_pushinteger(LS, skill);
    return 1;
}

static int CH_set_waitcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    int val=luaL_checkinteger( LS, 2);

    if ( val < 0 || val > 120 )
    {
        return luaL_error( LS, "Valid stopcount range is 0 to 120");
    }
    
    ud_ch->wait=val;

    return 0;
}

static int CH_get_hitroll (lua_State *LS)
{
    lua_pushinteger( LS,
            GET_HITROLL( check_CH( LS, 1 ) ) );
    return 1;
}

static int CH_get_hitrollbase (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH( LS, 1 ))->hitroll );
    return 1;
}

static int CH_set_hitrollbase (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set hitrollbase on PCs.");

    int val=luaL_checkinteger( LS, 2 );

    if ( val < 0 || val > 1000 )
    {
        return luaL_error( LS, "Value must be between 0 and 1000." );
    } 
    ud_ch->hitroll=val;

    return 0;
}

static int CH_set_hrpcnt (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set hrpcnt on PCs.");

    /* analogous to mob_base_hitroll */
    ud_ch->hitroll= ud_ch->level * luaL_checkinteger( LS, 2 ) / 100 ; 
    return 0;
}

static int CH_get_damroll (lua_State *LS)
{
    lua_pushinteger( LS,
            GET_DAMROLL( check_CH( LS, 1 ) ) );
    return 1;
}

static int CH_get_damrollbase (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH( LS, 1 ))->damroll );
    return 1;
}

static int CH_set_damrollbase (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set damrollbase on PCs.");

    int val=luaL_checkinteger( LS, 2 );

    if ( val < 0 || val > 1000 )
    {
        return luaL_error( LS, "Value must be between 0 and 1000." );
    }
    ud_ch->damroll=val;

    return 0;
}


static int CH_set_drpcnt (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set drpcnt on PCs.");

    /* analogous to mob_base_damroll */
    ud_ch->damroll= ud_ch->level * luaL_checkinteger( LS, 2 ) / 100 ;
    return 0;
}

static int CH_get_attacktype( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS, attack_table[ud_ch->dam_type].name );
    return 1;
}

static int CH_set_attacktype (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set attacktype on PCs.");

    const char *arg=check_string(LS,2, MIL);

    int i;
    for ( i=0 ; attack_table[i].name ; i++ )
    {
        if (!strcmp( attack_table[i].name, arg ) )
        {
            ud_ch->dam_type=i;
            return 0;
        }
    }

    luaL_error(LS, "No such attacktype: %s", arg );
    return 1;
}

static int CH_get_damnoun (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS, attack_table[ud_ch->dam_type].noun );
    return 1;
}

static int CH_get_hp (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH (LS, 1))->hit );
    return 1;
}

static int CH_set_hp (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->hit=num;
    return 0;
}

static int CH_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_CH(LS,1))->name );
    return 1;
}

static int CH_set_name (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set name on PCs.");
    const char *new=check_string(LS, 2, MIL);
    free_string( ud_ch->name );
    ud_ch->name=str_dup(new);
    return 0;
}

static int CH_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->level );
    return 1;
}

static int CH_get_maxhp (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_hit );
    return 1;
}

static int CH_set_maxhp (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxhp on PCs.");
        
    ud_ch->max_hit = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_mana (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->mana );
    return 1;
}

static int CH_set_mana (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->mana=num;
    return 0;
}

static int CH_get_maxmana (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_mana );
    return 1;
}

static int CH_set_maxmana (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxmana on PCs.");
        
    ud_ch->max_mana = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_move (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->move );
    return 1;
}

static int CH_set_move (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH (LS, 1);
    int num = luaL_checkinteger (LS, 2);

    ud_ch->move=num;
    return 0;
}

static int CH_get_maxmove (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->max_move );
    return 1;
}

static int CH_set_maxmove (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set maxmove on PCs.");
        
    ud_ch->max_move = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_gold (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->gold );
    return 1;
}

static int CH_set_gold (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set gold on PCs.");
        
    ud_ch->gold = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_silver (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->silver );
    return 1;
}

static int CH_set_silver (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set silver on PCs.");
        
    ud_ch->silver = luaL_checkinteger( LS, 2);
    return 0;
}

static int CH_get_money (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger( LS,
            ud_ch->silver + ud_ch->gold*100 );
    return 1;
}

static int CH_get_sex (lua_State *LS)
{
    lua_pushstring( LS,
            sex_table[(check_CH(LS,1))->sex].name );
    return 1;
}

static int CH_set_sex (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set sex on PCs.");
    const char *arg=check_string( LS, 2, MIL);
    
    int i;
    for ( i=0 ; sex_table[i].name ; i++ )
    {
        if (!strcmp(sex_table[i].name, arg) )
        {
            ud_ch->sex=i;
            return 0;
        }
    }
    
    luaL_error(LS, "No such sex: %s", arg );
    return 0;
}

static int CH_get_size (lua_State *LS)
{
    lua_pushstring( LS,
            size_table[(check_CH(LS,1))->size].name );
    return 1;
}

static int CH_set_size (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set gold on PCs.");
        
    const char *arg=check_string( LS, 2, MIL);
    int i;
    for ( i=0 ; size_table[i].name ; i++ )
    {
        if (!strcmp( size_table[i].name, arg ) )
        {
            ud_ch->size=i;
            return 0;
        }
    }
    
    luaL_error( LS, "No such size: %s", arg );
    return 0;
}

static int CH_get_position (lua_State *LS)
{
    lua_pushstring( LS,
            position_table[(check_CH(LS,1))->position].short_name );
    return 1;
}

static int CH_get_align (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH(LS,1))->alignment );
    return 1;
}

static int CH_set_align (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int num=luaL_checkinteger( LS, 2);
    if (num < -1000 || num > 1000)
        luaL_error(LS, "Invalid align: %d, range is -1000 to 1000.", num);
    ud_ch->alignment = num;
    return 0;
}

#define CHGETSTAT( statname, statnum ) \
static int CH_get_ ## statname ( lua_State *LS ) \
{\
    lua_pushinteger( LS, \
            get_curr_stat((check_CH(LS,1)), statnum ));\
    return 1;\
}

CHGETSTAT( str, STAT_STR );
CHGETSTAT( con, STAT_CON );
CHGETSTAT( dex, STAT_DEX );
CHGETSTAT( int, STAT_INT );
CHGETSTAT( wis, STAT_WIS );

#define CHSETSTAT( statname, statnum ) \
static int CH_set_ ## statname ( lua_State *LS ) \
{\
    CHAR_DATA *ud_ch=check_CH(LS,1);\
    if (!IS_NPC(ud_ch))\
        luaL_error(LS, "Can't set stats on PCs.");\
    \
    int num = luaL_checkinteger( LS, 2);\
    if (num < 1 || num > 200 )\
        luaL_error(LS, "Invalid stat value: %d, range is 1 to 200.", num );\
    \
    ud_ch->perm_stat[ statnum ] = num;\
    return 0;\
}

CHSETSTAT( str, STAT_STR );     
CHSETSTAT( con, STAT_CON );
CHSETSTAT( dex, STAT_DEX );
CHSETSTAT( int, STAT_INT );
CHSETSTAT( wis, STAT_WIS );

static int CH_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_CH(LS,1))->clan].name);
    return 1;
}

static int CH_get_class (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch))
    {
        luaL_error(LS, "Can't check class on NPC.");
    }

    lua_pushstring( LS,
            class_table[ud_ch->class].name);
    return 1;
}

static int CH_get_race (lua_State *LS)
{
    lua_pushstring( LS,
            race_table[(check_CH(LS,1))->race].name);
    return 1;
}

static int CH_get_fighting (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!ud_ch->fighting)
        return 0;
    else if (!push_CH(LS, ud_ch->fighting) )
        return 0;
    else
        return 1;
}

static int CH_get_waitcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger(LS, ud_ch->wait);
    return 1;
}

static int CH_get_heshe (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if ( ud_ch->sex==SEX_MALE )
    {
        lua_pushstring( LS, "he");
        return 1;
    }
    else if ( ud_ch->sex==SEX_FEMALE )
    {
        lua_pushstring( LS, "she");
        return 1;
    }
    else
    {
        lua_pushstring( LS, "it");
        return 1;
    }
}

static int CH_get_himher (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if ( ud_ch->sex==SEX_MALE )
    {
        lua_pushstring( LS, "him");
        return 1;
    }
    else if ( ud_ch->sex==SEX_FEMALE )
    {
        lua_pushstring( LS, "her");
        return 1;
    }
    else
    {
        lua_pushstring( LS, "it");
        return 1;
    }
}

static int CH_get_hisher (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if ( ud_ch->sex==SEX_MALE )
    {
        lua_pushstring( LS, "his");
        return 1;
    }
    else if ( ud_ch->sex==SEX_FEMALE )
    {
        lua_pushstring( LS, "her");
        return 1;
    }
    else
    {
        lua_pushstring( LS, "its");
        return 1;
    }
}

static int CH_get_inventory (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_ch->carrying ; obj ; obj=obj->next_content)
    {
        if (push_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int CH_get_room (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    
    if (!ud_ch->in_room)
        return 0;
    else if ( push_ROOM(LS, check_CH(LS,1)->in_room) )
        return 1;
    else
        return 0;
}

static int CH_get_groupsize (lua_State *LS)
{
    lua_pushinteger( LS,
            count_people_room( check_CH(LS, 1), 4 ) );
    return 1;
}

static int CH_get_vnum( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get vnum on PCs.");

    lua_pushinteger( LS,
            ud_ch->pIndexData->vnum);
    return 1;
}

static int CH_get_proto( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get proto on PCs.");

    if (!push_MOBPROTO( LS, ud_ch->pIndexData ) )
        return 0;
    else
        return 1;
}

static int CH_get_shortdescr( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get shortdescr on PCs.");

    lua_pushstring( LS,
            ud_ch->short_descr);
    return 1;
}

static int CH_set_shortdescr (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set shortdescr on PCs.");
    const char *new=check_string(LS, 2, MIL);
    free_string( ud_ch->short_descr );
    ud_ch->short_descr=str_dup(new);
    return 0;
}

static int CH_get_longdescr( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get longdescr on PCs.");

    lua_pushstring( LS,
            ud_ch->long_descr);
    return 1;
}

static int CH_set_longdescr (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set longdescr on PCs.");
    const char *new=check_string(LS, 2, MIL);
    free_string( ud_ch->long_descr );
    ud_ch->long_descr=str_dup(new);
    return 0;
}

static int CH_get_description( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS,
            ud_ch->description);
    return 1;
}

static int CH_set_description (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set description on PCs.");
    const char *new=check_string(LS, 2, MSL);

    // Need to make sure \n\r at the end but don't add if already there.
    int len=strlen(new);
    if ( len>1 &&
            !( new[len-2]=='\n' && new[len-1]=='\r') )
    {
        if ( len > (MSL-3) )
            luaL_error( LS, "Description must be %d characters or less.", MSL-3);

        char buf[MSL];
        sprintf(buf, "%s\n\r",new);
        new=buf;
    }
    free_string( ud_ch->description );
    ud_ch->description=str_dup(new);
    return 0;
}

static int CH_get_pet (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if ( ud_ch->pet && push_CH(LS, ud_ch->pet) )
        return 1;
    else
        return 0;
}

static int CH_get_master (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if ( IS_AFFECTED(ud_ch, AFF_CHARM) 
            && ud_ch->master 
            && push_CH(LS, ud_ch->master) )
        return 1;
    else
        return 0;
}

static int CH_get_leader (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if ( ud_ch->leader && push_CH(LS, ud_ch->leader) )
        return 1;
    else
        return 0;
}

static int CH_get_id ( lua_State *LS )
{
    lua_pushinteger( LS,
            check_CH(LS,1)->id );
    return 1;
}

static int CH_get_scroll ( lua_State *LS )
{
    lua_pushinteger( LS,
            check_CH(LS,1)->lines );
    return 1;
}

static int CH_get_affects ( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    AFFECT_DATA *af;

    int index=1;
    lua_newtable( LS );

    for ( af=ud_ch->affected ; af ; af=af->next )
    {
        if (push_AFFECT(LS,af))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int CH_get_descriptor( lua_State *LS )
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (ud_ch->desc)
    {
        if ( push_DESCRIPTOR(LS, ud_ch->desc) )
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

static const LUA_PROP_TYPE CH_get_table [] =
{
    CHGET(name, 0),
    CHGET(level, 0),
    CHGET(hp, 0),
    CHGET(maxhp, 0),
    CHGET(mana, 0),
    CHGET(maxmana, 0),
    CHGET(move, 0),
    CHGET(maxmove, 0),
    CHGET(gold, 0),
    CHGET(silver, 0),
    CHGET(money, 0),
    CHGET(sex, 0),
    CHGET(size, 0),
    CHGET(position, 0),
    CHGET(align, 0),
    CHGET(str, 0),
    CHGET(con, 0),
    CHGET(dex, 0),
    CHGET(int, 0),
    CHGET(wis, 0),
    CHGET(hitroll, 0),
    CHGET(hitrollbase, 0),
    CHGET(damroll, 0),
    CHGET(damrollbase, 0),
    CHGET(attacktype, 0),
    CHGET(damnoun, 0),
    CHGET(clan, 0),
    CHGET(class, 0),
    CHGET(race, 0),
    CHGET(ispc, 0),
    CHGET(isnpc, 0),
    CHGET(isgood, 0),
    CHGET(isevil, 0),
    CHGET(isneutral, 0),
    CHGET(isimmort, 0),
    CHGET(ischarm, 0),
    CHGET(isfollow, 0),
    CHGET(isactive, 0),
    CHGET(fighting, 0),
    CHGET(waitcount, 0),
    CHGET(heshe, 0),
    CHGET(himher, 0),
    CHGET(hisher, 0),
    CHGET(inventory, 0),
    CHGET(room, 0),
    CHGET(groupsize, 0),
    CHGET(description, 0),
    CHGET(pet, 0),
    CHGET(master, 0),
    CHGET(leader, 0),
    CHGET(affects, 0),
    CHGET(scroll, 0),
    CHGET(id, 0 ),
    /* PC only */
    CHGET(descriptor, 0),
    /* NPC only */
    CHGET(vnum, 0),
    CHGET(proto,0),
    CHGET(shortdescr, 0),
    CHGET(longdescr, 0),    
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_set_table [] =
{
    CHSET(name, 5),
    CHSET(hp, 5),
    CHSET(maxhp, 5),
    CHSET(mana, 5),
    CHSET(maxmana, 5),
    CHSET(move, 5),
    CHSET(maxmove, 5),
    CHSET(gold, 5),
    CHSET(silver, 5),
    CHSET(sex, 5),
    CHSET(size, 5),
    CHSET(align, 5),
    CHSET(str, 5),
    CHSET(con, 5),
    CHSET(dex, 5),
    CHSET(int, 5),
    CHSET(wis, 5),
    CHSET(hrpcnt, 5),
    CHSET(hitrollbase, 5),
    CHSET(drpcnt, 5),
    CHSET(damrollbase, 5),
    CHSET(attacktype, 5),
    CHSET(waitcount, 5),
    CHSET(shortdescr, 5),
    CHSET(longdescr, 5),
    CHSET(description, 5),
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_method_table [] =
{
    CHMETH(mobhere, 0),
    CHMETH(objhere, 0),
    CHMETH(mobexists, 0),
    CHMETH(objexists, 0),
    CHMETH(affected, 0),
    CHMETH(act, 0),
    CHMETH(offensive, 0),
    CHMETH(immune, 0),
    CHMETH(carries, 0),
    CHMETH(wears, 0),
    CHMETH(has, 0),
    CHMETH(uses, 0),
    CHMETH(resist, 0),
    CHMETH(vuln, 0),
    CHMETH(skilled, 0),
    CHMETH(cansee, 0),
    CHMETH(canattack, 0),
    CHMETH(destroy, 1),
    CHMETH(oload, 1),
    CHMETH(say, 1),
    CHMETH(emote, 1),
    CHMETH(mdo, 1),
    CHMETH(asound, 1),
    CHMETH(zecho, 1),
    CHMETH(kill, 1),
    CHMETH(assist, 1),
    CHMETH(junk, 1),
    CHMETH(echo, 1),
    CHMETH(purge, 1),
    CHMETH(goto, 1),
    CHMETH(at, 1),
    CHMETH(otransfer, 1),
    CHMETH(force, 1),
    CHMETH(gforce, 1),
    CHMETH(vforce, 1),
    CHMETH(cast, 1),
    CHMETH(damage, 1),
    CHMETH(remove, 1),
    CHMETH(setact, 1),
    CHMETH(setvuln, 1),
    CHMETH(setimmune, 1),
    CHMETH(setresist, 1),
    CHMETH(randchar, 0),
    //CHMETH(loadprog, 1),
    //CHMETH(loadscript, 1),
    //CHMETH(loadstring, 1),
    //CHMETH(loadfunction, 1),
    //CHMETH(savetbl, 1),
    //CHMETH(loadtbl, 1),
    CHMETH(tprint, 1),
    //CHMETH(olc, 1),
    //CHMETH(addaffect, 9),
    //CHMETH(removeaffect,9),
    ENDPTABLE
}; 

/* end CH section */

/* OBJ section */
static int OBJ_set_weapontype( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    if (ud_obj->item_type != ITEM_WEAPON )
        return luaL_error(LS, "weapontype for weapon only.");
 
    const char *arg1=check_string(LS,2,MIL);

    int new_type=flag_lookup(arg1, weapon_class);
    if ( new_type == NO_FLAG )
    {
        return luaL_error(LS, "No such weapontype '%s'", arg1);
    }

    ud_obj->value[0]=new_type;

    return 0;
}

static int OBJ_setexitflag( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    if (ud_obj->item_type != ITEM_PORTAL)
        return luaL_error(LS, "setexitflag for portal only");

    return set_flag( LS, "exit_flags", exit_flags, &ud_obj->value[1] );
}

#if 0
static int OBJ_loadfunction (lua_State *LS)
{
    lua_obj_program( NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -2), NULL,
                NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int OBJ_savetbl (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    /* Push original args into SaveTable */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_obj->pIndexData->area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int OBJ_loadtbl (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_obj->pIndexData->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int OBJ_loadscript (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular oprog with vnum 0*/

    lua_pushboolean( LS,
            lua_obj_program( NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH), ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );

    return 1;

}

static int OBJ_loadstring (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_pushboolean( LS,
            lua_obj_program( NULL, LOADSCRIPT_VNUM, check_string( LS, 2, MAX_SCRIPT_LENGTH), ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );
    return 1;
}

static int OBJ_loadprog (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pOcode;

    if ( (pOcode = get_oprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: oprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_obj_program( NULL, num, pOcode->code, ud_obj, NULL, NULL, NULL, OTRIG_CALL, 0) );

    return 1;
}
#endif
static int OBJ_destroy( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);

    if (!ud_obj)
    {
        luaL_error(LS, "Null pointer in L_obj_destroy.");
        return 0;
    }
    extract_obj(ud_obj);
    return 0;
}

static int OBJ_clone( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);

    OBJ_DATA *clone = create_object(ud_obj->pIndexData,0);
    clone_object( ud_obj, clone );

    if (ud_obj->carried_by)
        obj_to_char( clone, ud_obj->carried_by );
    else if (ud_obj->in_room)
        obj_to_room( clone, ud_obj->in_room );
    else if (ud_obj->in_obj)
        obj_to_obj( clone, ud_obj->in_obj );
    else
        luaL_error( LS, "Cloned object has no location.");

    if (push_OBJ( LS, clone))
        return 1;
    else
        return 0;
}

static int OBJ_oload (lua_State *LS)
{
    OBJ_DATA * ud_obj = check_OBJ (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if ( ud_obj->item_type != ITEM_CONTAINER )
    {
        luaL_error(LS, "Tried to load object in non-container." );
    }

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex,0);
    obj_to_obj(obj,ud_obj);

    if ( !push_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}

static int OBJ_extra( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    return check_flag( LS, "extra", extra_flags, ud_obj->extra_flags );
}

static int OBJ_wear( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    return check_flag( LS, "wear", wear_flags, ud_obj->wear_flags );
}

static int OBJ_echo( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *argument = check_fstring(LS, 2, MIL);

    if (ud_obj->carried_by)
    {
        send_to_char(argument, ud_obj->carried_by);
        send_to_char( "\n\r", ud_obj->carried_by);
    }
    else if (ud_obj->in_room)
    {
        CHAR_DATA *ch;
        for ( ch=ud_obj->in_room->people ; ch ; ch=ch->next_in_room )
        {
            send_to_char( argument, ch );
            send_to_char( "\n\r", ch );
        }
    }
    else
    {
        // Nothing, must be in a container
    }

    return 0;
}

static int OBJ_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, OBJ_echo );
    /* now line up arguments for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;

}

static int OBJ_setweaponflag( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    
    if (ud_obj->item_type != ITEM_WEAPON)
        return luaL_error(LS, "setweaponflag for weapon only");

    return set_flag( LS, "weapon_type2", weapon_type2, &ud_obj->value[4]);
}

static int OBJ_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->name);
    return 1;
}

static int OBJ_set_name (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->name);
    ud_obj->name=str_dup(arg);
    return 0;
}

static int OBJ_get_shortdescr (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->short_descr);
    return 1;
}

static int OBJ_set_shortdescr (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->short_descr);
    ud_obj->short_descr=str_dup(arg);
    return 0;
}

static int OBJ_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->description);
    return 1;
}

static int OBJ_set_description (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->description);
    ud_obj->description=str_dup(arg);
    return 0;
}

static int OBJ_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->level);
    return 1;
}

static int OBJ_set_level (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int arg=luaL_checkinteger(LS,2);
    
    ud_obj->level=arg;
    return 0;
}

static int OBJ_get_owner (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->owner)
        return 0;
    lua_pushstring( LS,
            ud_obj->owner);
    return 1;
}

static int OBJ_set_owner (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->owner);
    ud_obj->owner=str_dup(arg);
    return 0;
}

static int OBJ_get_cost (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->cost);
    return 1;
}

static int OBJ_get_material (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJ(LS,1))->material);
    return 1;
}

static int OBJ_set_material (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    const char *arg=check_string(LS,2,MIL);
    free_string(ud_obj->material);
    ud_obj->material=str_dup(arg);
    return 0;
}

static int OBJ_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->pIndexData->vnum);
    return 1;
}

static int OBJ_get_otype (lua_State *LS)
{
    lua_pushstring( LS,
            item_name((check_OBJ(LS,1))->item_type));
    return 1;
}

static int OBJ_get_weight (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->weight);
    return 1;
}

static int OBJ_set_weight (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS, 1);
    int arg=luaL_checkinteger(LS,2);
    
    ud_obj->weight=arg;
    return 0;
}

static int OBJ_get_wearlocation (lua_State *LS)
{
    lua_pushstring( LS,
            flag_bit_name(wear_loc_flags,(check_OBJ(LS,1))->wear_loc) );
    return 1;
}

static int OBJ_get_proto (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!push_OBJPROTO( LS, ud_obj->pIndexData) )
        return 0;
    else
        return 1;
}

static int OBJ_get_contents (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_obj->contains ; obj ; obj=obj->next_content)
    {
        if ( push_OBJ(LS, obj) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int OBJ_get_room (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->in_room)
        return 0;
    if ( push_ROOM(LS, ud_obj->in_room) )
        return 1;
    else
        return 0;
}

static int OBJ_set_room (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    ROOM_INDEX_DATA *rid=check_ROOM(LS,2);

    if (ud_obj->in_room)
        obj_from_room(ud_obj);
    else if (ud_obj->carried_by)
        obj_from_char(ud_obj);
    else if (ud_obj->in_obj)
        obj_from_obj(ud_obj);
    else
        luaL_error(LS, "No location for %s (%d)", 
                ud_obj->name, 
                ud_obj->pIndexData->vnum);

    obj_to_room(ud_obj, rid);
    return 0;
}

static int OBJ_get_inobj (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->in_obj)
        return 0;

    if ( !push_OBJ(LS, ud_obj->in_obj) )
        return 0;
    else
        return 1;
}

static int OBJ_get_carriedby (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    if (!ud_obj->carried_by )
        return 0;
    else if (!push_CH( LS, ud_obj->carried_by) )
        return 0;
    else
        return 1;
}

static int OBJ_set_carriedby (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    CHAR_DATA *ch=check_CH(LS,2);

    if (ud_obj->carried_by)
        obj_from_char(ud_obj);
    else if (ud_obj->in_room)
        obj_from_room(ud_obj);
    else if (ud_obj->in_obj)
        obj_from_obj(ud_obj);
    else
        luaL_error(LS, "No location for %s (%d)",
                ud_obj->name,
                ud_obj->pIndexData->vnum);
    
    obj_to_char( ud_obj, ch );

    return 0;
}

static int OBJ_get_v0 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[0]);
    return 1;
}

static int OBJ_get_v1 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[1]);
    return 1;
}


static int OBJ_get_v2 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[2]);
    return 1;
}

static int OBJ_get_v3 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[3]);
    return 1;
}

static int OBJ_get_v4 (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->value[4]);
    return 1;
}

static int OBJ_get_timer (lua_State *LS)
{
    lua_pushinteger( LS, (check_OBJ(LS,1))->timer);
    return 1;
}

static int OBJ_get_affects ( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    AFFECT_DATA *af;

    int index=1;
    lua_newtable( LS );

    for ( af = ud_obj->affected ; af ; af = af->next )
    {
        if (push_AFFECT( LS, af) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static const LUA_PROP_TYPE OBJ_get_table [] =
{
    OBJGET(name, 0),
    OBJGET(shortdescr, 0),
    OBJGET(description, 0),
    OBJGET(level, 0),
    OBJGET(owner, 0),
    OBJGET(cost, 0),
    OBJGET(material, 0),
    OBJGET(vnum, 0),
    OBJGET(otype, 0),
    OBJGET(weight, 0),
    OBJGET(room, 0),
    OBJGET(inobj, 0),
    OBJGET(carriedby, 0),
    OBJGET(v0, 0),
    OBJGET(v1, 0),
    OBJGET(v2, 0),
    OBJGET(v3, 0),
    OBJGET(v4, 0),
    OBJGET(wearlocation, 0),
    OBJGET(contents, 0),
    OBJGET(proto, 0),
    OBJGET(timer, 0),
    OBJGET(affects, 0),
    
    /*light*/
    OBJGET(light, 0),

    /* wand, staff */
    OBJGET(spelllevel, 0),
    OBJGET(chargestotal, 0),
    OBJGET(chargesleft, 0),
    OBJGET(spellname, 0),
    
    /* portal */
    // chargesleft
    OBJGET(toroom, 0),

    /* furniture */
    OBJGET(maxpeople, 0),
    OBJGET(maxweight, 0),
    OBJGET(healbonus, 0),
    OBJGET(manabonus, 0),

    /* scroll, potion, pill */
    //spelllevel
    OBJGET(spells, 0),

    /* armor */
    OBJGET( ac, 0),

    /* weapon */
    OBJGET( weapontype, 0),
    OBJGET( numdice, 0),
    OBJGET( dicetype, 0),
    OBJGET( attacktype, 0),
    OBJGET( damnoun, 0),

    /* container */
    //maxweight
    OBJGET( key, 0),
    OBJGET( capacity, 0),
    OBJGET( weightmult, 0),

    /* drink container */
    OBJGET( liquidtotal, 0),
    OBJGET( liquidleft, 0),
    OBJGET( liquid, 0),
    OBJGET( liquidcolor, 0),
    OBJGET( poisoned, 0),

    /*fountain*/
    //liquid
    //liquidleft
    //liquidtotal

    /* food */
    OBJGET( foodhours, 0),
    OBJGET( fullhours, 0),
    // poisoned
    
    /* money */
    OBJGET( silver, 0),
    OBJGET( gold, 0),
    
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJ_set_table [] =
{
    OBJSET(name, 5 ),
    OBJSET(shortdescr, 5),
    OBJSET(description, 5),
    OBJSET(level, 5),
    OBJSET(owner, 5),
    OBJSET(material, 5),
    OBJSET(weight, 5),
    OBJSET(room, 5),
    OBJSET(carriedby, 5),
    OBJSET(weapontype, 9),
       
    ENDPTABLE
};


static const LUA_PROP_TYPE OBJ_method_table [] =
{
    OBJMETH(extra, 0),
    OBJMETH(wear, 0),
    OBJMETH(apply, 0),
    OBJMETH(destroy, 1),
    OBJMETH(clone, 1),
    OBJMETH(echo, 1),
    //OBJMETH(loadprog, 1),
    //OBJMETH(loadscript, 1),
    //OBJMETH(loadstring, 1),
    //OBJMETH(loadfunction, 1),
    OBJMETH(oload, 1),
    //OBJMETH(savetbl, 1),
    //OBJMETH(loadtbl, 1),
    OBJMETH(tprint, 1),
    
    /* portal only */
    OBJMETH(exitflag, 0),
    OBJMETH(setexitflag, 1),
    OBJMETH(portalflag, 0),

    /* furniture only */
    OBJMETH(furnitureflag, 0),
    
    /* weapon only */
    OBJMETH(weaponflag, 0),
    OBJMETH(setweaponflag, 5),
    
    /* container only */
    OBJMETH(containerflag, 0),
    
    ENDPTABLE
}; 

/* end OBJ section */

/* AREA section */
#if 0
static int AREA_loadfunction( lua_State *LS)
{
    lua_area_program( NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -2), NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int AREA_loadscript (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    /* now run the result as a regular aprog with vnum 0*/
    lua_pushboolean( LS,
            lua_area_program( NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH), ud_area, NULL, ATRIG_CALL, 0) );

    return 1;
}

static int AREA_loadstring (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);
    lua_pushboolean( LS,
            lua_area_program( NULL, LOADSCRIPT_VNUM, check_string( LS, 2, MAX_SCRIPT_LENGTH), ud_area, NULL, ATRIG_CALL, 0) );
    return 1;
}

static int AREA_loadprog (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pAcode;

    if ( (pAcode = get_aprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: aprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_area_program( NULL, num, pAcode->code, ud_area, NULL, ATRIG_CALL, 0) );

    return 1;
}

#endif

static int AREA_flag( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    return check_flag( LS, "area", area_flags, ud_area->area_flags );
}

static int AREA_reset( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS,1);
    reset_area(ud_area);
    return 0;
}

static int AREA_get_name( lua_State *LS)
{
    lua_pushstring( LS, (check_AREA(LS, 1))->name);
    return 1;
}

static int AREA_get_filename( lua_State *LS)
{
    lua_pushstring( LS, (check_AREA(LS, 1))->file_name);
    return 1;
}

static int AREA_get_nplayer( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->nplayer);
    return 1;
}

static int AREA_get_security( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->security);
    return 1;
}

static int AREA_get_rooms( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    ROOM_INDEX_DATA *room;
    int vnum;
    for (vnum=ud_area->min_vnum ; vnum<=ud_area->max_vnum ; vnum++)
    {
        if ((room=get_room_index(vnum))==NULL)
            continue;
        if (push_ROOM(LS, room))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_people( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=char_list ; people ; people=people->next)
    {
        if ( !people || !people->in_room
                || (people->in_room->area != ud_area) )
            continue;
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_players( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=char_list ; people ; people=people->next)
    {
        if ( IS_NPC(people)
                || !people || !people->in_room
                || (people->in_room->area != ud_area) )
            continue;
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_mobs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=char_list ; people ; people=people->next)
    {
        if ( !IS_NPC(people)
                || !people || !people->in_room
                || (people->in_room->area != ud_area) )
            continue;
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int AREA_get_mobprotos( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    MOB_INDEX_DATA *mid;
    for ( vnum=ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((mid=get_mob_index(vnum)) != NULL )
        {
            if (push_MOBPROTO(LS, mid))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_objprotos( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    OBJ_INDEX_DATA *oid;
    for ( vnum=ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((oid=get_obj_index(vnum)) != NULL )
        {
            if (push_OBJPROTO(LS, oid))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_mprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    MPROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_mprog_index(vnum)) != NULL )
        {
            if (push_MPROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_vnum ( lua_State *LS)
{
    lua_pushinteger( LS,
            (check_AREA(LS,1))->vnum);
    return 1;
}

static int AREA_get_minvnum ( lua_State *LS)
{
    lua_pushinteger( LS,
            (check_AREA(LS,1))->min_vnum);
    return 1;
}

static int AREA_get_maxvnum ( lua_State *LS)
{
    lua_pushinteger( LS,
            (check_AREA(LS,1))->max_vnum);
    return 1;
}

static int AREA_get_credits ( lua_State *LS)
{
    lua_pushstring( LS,
            (check_AREA(LS,1))->credits);
    return 1;
}

static int AREA_get_builders ( lua_State *LS)
{
    lua_pushstring( LS,
            (check_AREA(LS,1))->builders);
    return 1;
}

static const LUA_PROP_TYPE AREA_get_table [] =
{
    AREAGET(name, 0),
    AREAGET(filename, 0),
    AREAGET(nplayer, 0),
    AREAGET(security, 0),
    AREAGET(vnum, 0),
    AREAGET(minvnum, 0),
    AREAGET(maxvnum, 0),
    AREAGET(credits, 0),
    AREAGET(builders, 0),
    AREAGET(rooms, 0),
    AREAGET(people, 0),
    AREAGET(players, 0),
    AREAGET(mobs, 0),
    AREAGET(mobprotos, 0),
    AREAGET(objprotos, 0),
    AREAGET(mprogs, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_method_table [] =
{
    AREAMETH(flag, 0),
    AREAMETH(reset, 5),
    //AREAMETH(loadprog, 1),
    //AREAMETH(loadscript, 1),
    //AREAMETH(loadstring, 1),
    //AREAMETH(loadfunction, 1),
    ENDPTABLE
}; 

/* end AREA section */

/* ROOM section */
/*
static int ROOM_loadfunction ( lua_State *LS)
{
    lua_room_program( NULL, RUNDELAY_VNUM, NULL,
                check_ROOM(LS, -2), NULL,
                NULL, NULL, NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}*/

static int ROOM_mload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    MOB_INDEX_DATA *pObjIndex = get_mob_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No mob with vnum: %d", num);

    CHAR_DATA *mob=create_mobile( pObjIndex);
    char_to_room(mob,ud_room);

    if ( !push_CH(LS, mob))
        return 0;
    else
        return 1;

}

static int ROOM_oload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex,0);
    obj_to_room(obj,ud_room);

    if ( !push_OBJ(LS, obj) )
        return 0;
    else
        return 1;

}

static int ROOM_flag( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    return check_flag( LS, "room", room_flags, ud_room->room_flags );
}

static int ROOM_reset( lua_State *LS)
{
    reset_room( check_ROOM(LS,1) );
    return 0;
}

static int ROOM_echo( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room = check_ROOM(LS, 1);
    const char *argument = check_fstring( LS, 2, MSL);

    CHAR_DATA *vic;
    for ( vic=ud_room->people ; vic ; vic=vic->next_in_room )
    {
        if (!IS_NPC(vic) )
        {
            send_to_char(argument, vic);
            send_to_char("\n\r", vic);
        }
    }

    return 0;
}

static int ROOM_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, ROOM_echo );
    /* now line up argumenets for echo */
    lua_pushvalue( LS, 1); /* obj */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

    return 0;
}
#if 0
static int ROOM_savetbl (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_room->area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int ROOM_loadtbl (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_room->area->file_name );
    lua_call( LS, 2, 1);

    return 1;
}

static int ROOM_loadscript (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, GETSCRIPT_FUNCTION);

    /* Push original args into GetScript */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_call( LS, 2, 1);

    lua_pushboolean( LS,
            lua_room_program( NULL, LOADSCRIPT_VNUM, check_string( LS, -1, MAX_SCRIPT_LENGTH),
                ud_room, NULL, NULL, NULL, NULL, NULL, RTRIG_CALL, 0) );
    return 1;
}

static int ROOM_loadstring (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_pushboolean( LS,
            lua_room_program( NULL, LOADSCRIPT_VNUM, check_string(LS, 2, MAX_SCRIPT_LENGTH),
                ud_room, NULL, NULL, NULL, NULL, NULL, RTRIG_CALL, 0) );
    return 1;
}

static int ROOM_loadprog (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int num = (int)luaL_checknumber (LS, 2);
    PROG_CODE *pRcode;

    if ( (pRcode = get_rprog_index(num)) == NULL )
    {
        luaL_error(LS, "loadprog: rprog vnum %d doesn't exist", num);
        return 0;
    }

    lua_pushboolean( LS,
            lua_room_program( NULL, num, pRcode->code,
                ud_room, NULL, NULL, NULL, NULL, NULL,
                RTRIG_CALL, 0) );
    return 1;
}
#endif

static int ROOM_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->name);
    return 1;
}

static int ROOM_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->vnum);
    return 1;
}

static int ROOM_get_healrate (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->heal_rate);
    return 1;
}

static int ROOM_get_manarate (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->mana_rate);
    return 1;
}

static int ROOM_get_owner (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->owner);
    return 1;
}

static int ROOM_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_ROOM(LS,1))->description);
    return 1;
}

static int ROOM_get_sector (lua_State *LS)
{
    lua_pushstring( LS,
            flag_bit_name(sector_flags, (check_ROOM(LS,1))->sector_type) );
    return 1;
}

static int ROOM_get_contents (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    OBJ_DATA *obj;
    for (obj=ud_room->contents ; obj ; obj=obj->next_content)
    {
        if (push_OBJ(LS, obj))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_area (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    if ( !push_AREA(LS, ud_room->area))
        return 0;
    else
        return 1;
}

static int ROOM_get_people (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *people;
    for (people=ud_room->people ; people ; people=people->next_in_room)
    {
        if (push_CH(LS, people))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_players (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *plr;
    for ( plr=ud_room->people ; plr ; plr=plr->next_in_room)
    {
        if (!IS_NPC(plr) && push_CH(LS, plr))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_mobs (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    int index=1;
    lua_newtable(LS);
    CHAR_DATA *mob;
    for ( mob=ud_room->people ; mob ; mob=mob->next_in_room)
    {
        if ( IS_NPC(mob) && push_CH(LS, mob))
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int ROOM_get_exits (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_newtable(LS);
    int i;
    int index=1;
    for ( i=0; i<MAX_DIR ; i++)
    {
        if (ud_room->exit[i])
        {
            lua_pushstring(LS,dir_name[i]);
            lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

#define ROOM_dir(dirname, dirnumber) static int ROOM_get_ ## dirname (lua_State *LS)\
{\
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);\
    if (!ud_room->exit[dirnumber])\
        return 0;\
    if (!push_EXIT(LS, ud_room->exit[dirnumber]))\
        return 0;\
    else\
        return 1;\
}

ROOM_dir(north, DIR_NORTH)
ROOM_dir(south, DIR_SOUTH)
ROOM_dir(east, DIR_EAST)
ROOM_dir(west, DIR_WEST)
ROOM_dir(up, DIR_UP)
ROOM_dir(down, DIR_DOWN)

static int ROOM_get_resets (lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_newtable(LS);
    int index=1;
    RESET_DATA *reset;
    for ( reset=ud_room->reset_first; reset; reset=reset->next)
    {
        if (push_RESET(LS, reset) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static const LUA_PROP_TYPE ROOM_get_table [] =
{
    ROOMGET(name, 0),
    ROOMGET(vnum, 0),
    ROOMGET(healrate, 0),
    ROOMGET(manarate, 0),
    ROOMGET(owner, 0),
    ROOMGET(description, 0),
    ROOMGET(sector, 0),
    ROOMGET(contents, 0),
    ROOMGET(area, 0),
    ROOMGET(people, 0),
    ROOMGET(players, 0),
    ROOMGET(mobs, 0),
    ROOMGET(exits, 0),
    ROOMGET(north, 0),
    ROOMGET(south, 0),
    ROOMGET(east, 0),
    ROOMGET(west, 0),
    ROOMGET(up, 0),
    ROOMGET(down, 0),
    ROOMGET(resets, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE ROOM_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE ROOM_method_table [] =
{
    ROOMMETH(flag, 0),
    ROOMMETH(reset, 5),
    ROOMMETH(oload, 1),
    ROOMMETH(mload, 1),
    ROOMMETH(echo, 1),
    //ROOMMETH(loadprog, 1),
    //ROOMMETH(loadscript, 1),
    //ROOMMETH(loadstring, 1),
    //ROOMMETH(loadfunction, 1),
    ROOMMETH(tprint, 1),
    //ROOMMETH(savetbl, 1),
    //ROOMMETH(loadtbl, 1),
    ENDPTABLE
}; 

/* end ROOM section */

/* EXIT section */
static int EXIT_flag (lua_State *LS)
{
    EXIT_DATA *ed=check_EXIT( LS, 1 );
    return check_flag( LS, "exit", exit_flags, ed->exit_info );
}

static int EXIT_setflag( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);
    return set_flag( LS, "exit", exit_flags, ud_exit->exit_info); 
}

static int EXIT_lock( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot lock.");
    }

    /* force closed if necessary */
    SET_BIT(ud_exit->exit_info, EX_CLOSED);
    SET_BIT(ud_exit->exit_info, EX_LOCKED);
    return 0;
}

static int EXIT_unlock( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot unlock.");
    }

    REMOVE_BIT(ud_exit->exit_info, EX_LOCKED);
    return 0;
}

static int EXIT_close( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot close.");
    }

    SET_BIT(ud_exit->exit_info, EX_CLOSED);
    return 0;
}

static int EXIT_open( lua_State *LS)
{
    EXIT_DATA *ud_exit = check_EXIT(LS, 1);

    if (!IS_SET(ud_exit->exit_info, EX_ISDOOR))
    {
        luaL_error(LS, "Exit is not a door, cannot open.");
    }

    /* force unlock if necessary */
    REMOVE_BIT(ud_exit->exit_info, EX_LOCKED);
    REMOVE_BIT(ud_exit->exit_info, EX_CLOSED);

    return 0;
}

static int EXIT_get_toroom (lua_State *LS)
{
    EXIT_DATA *ud_exit=check_EXIT(LS,1);
    if ( !push_ROOM( LS, ud_exit->u1.to_room ))
        return 0;
    else
        return 1;
}

static int EXIT_get_keyword (lua_State *LS)
{
    lua_pushstring(LS,
            (check_EXIT(LS,1))->keyword);
    return 1;
}

static int EXIT_get_description (lua_State *LS)
{
    lua_pushstring(LS,
            (check_EXIT(LS,1))->description);
    return 1;
}

static int EXIT_get_key (lua_State *LS)
{
    lua_pushinteger(LS,
            (check_EXIT(LS,1))->key);
    return 1;
}

static const LUA_PROP_TYPE EXIT_get_table [] =
{
    EXGET(toroom, 0),
    EXGET(keyword,0),
    EXGET(description, 0),
    EXGET(key, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE EXIT_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE EXIT_method_table [] =
{
    EXMETH(flag, 0),
    EXMETH(setflag, 0),
    EXMETH(open, 0),
    EXMETH(close, 0),
    EXMETH(unlock, 0),
    EXMETH(lock, 0),
    ENDPTABLE
}; 

/* end EXIT section */

/* RESET section */
static int RESET_get_command(lua_State *LS, RESET_DATA *rd )
{
    static char buf[2];
    sprintf(buf, "%c", rd->command);
    lua_pushstring(LS, buf);
    return 1;
}

#define RESETGETARG( num ) static int RESET_get_arg ## num ( lua_State *LS)\
{\
    lua_pushinteger( LS,\
            (check_RESET(LS,1))->arg ## num);\
    return 1;\
}

RESETGETARG(1);
RESETGETARG(2);
RESETGETARG(3);
RESETGETARG(4);

static const LUA_PROP_TYPE RESET_get_table [] =
{
    RSTGET( command, 0),
    RSTGET( arg1, 0),
    RSTGET( arg2, 0),
    RSTGET( arg3, 0),
    RSTGET( arg4, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE RESET_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE RESET_method_table [] =
{
    ENDPTABLE
}; 

/* end RESET section */

/* OBJPROTO section */
static int OBJPROTO_wear( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    return check_flag( LS, "wear", wear_flags, ud_objp->wear_flags );
}

static int OBJPROTO_extra( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    return check_flag( LS, "extra", extra_flags, ud_objp->extra_flags );
}

static int OBJPROTO_get_name (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->name);
    return 1;
}

static int OBJPROTO_get_shortdescr (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->short_descr);
    return 1;
}

static int OBJPROTO_get_description (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->description);
    return 1;
}

static int OBJPROTO_get_level (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->level);
    return 1;
}

static int OBJPROTO_get_cost (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->cost);
    return 1;
}

static int OBJPROTO_get_material (lua_State *LS)
{
    lua_pushstring( LS,
            (check_OBJPROTO(LS,1))->material);
    return 1;
}

static int OBJPROTO_get_vnum (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->vnum);
    return 1;
}

static int OBJPROTO_get_otype (lua_State *LS)
{
    lua_pushstring( LS,
            item_name((check_OBJPROTO(LS,1))->item_type));
    return 1;
}

static int OBJPROTO_get_weight (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->weight);
    return 1;
}

#define OPGETV( num ) static int OBJPROTO_get_v ## num (lua_State *LS)\
{\
    lua_pushinteger( LS,\
            (check_OBJPROTO(LS,1))->value[num]);\
    return 1;\
}

OPGETV(0);
OPGETV(1);
OPGETV(2);
OPGETV(3);
OPGETV(4);

static int OBJPROTO_get_area ( lua_State *LS )
{
    if (push_AREA(LS, (check_OBJPROTO(LS,1))->area) )
        return 1;
    return 0;
}

static int OBJPROTO_get_affects ( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_oid=check_OBJPROTO( LS, 1);
    AFFECT_DATA *af;
    
    int index=1;
    lua_newtable( LS );

    for ( af = ud_oid->affected ; af ; af = af->next )
    {
        if (push_AFFECT( LS, af) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static const LUA_PROP_TYPE OBJPROTO_get_table [] =
{
    OPGET( name, 0),
    OPGET( shortdescr, 0),
    OPGET( description, 0),
    OPGET( level, 0),
    OPGET( cost, 0),
    OPGET( material, 0),
    OPGET( vnum, 0),
    OPGET( otype, 0),
    OPGET( weight, 0),
    OPGET( v0, 0),
    OPGET( v1, 0),
    OPGET( v2, 0),
    OPGET( v3, 0),
    OPGET( v4, 0),
    OPGET( area, 0),
    OPGET( affects, 0),

    /*light*/
    OPGET(light, 0),

    /* wand, staff */
    OPGET(spelllevel, 0),
    OPGET(chargestotal, 0),
    OPGET(chargesleft, 0),
    OPGET(spellname, 0),
    
    /* portal */
    // chargesleft
    OPGET(toroom, 0),

    /* furniture */
    OPGET(maxpeople, 0),
    OPGET(maxweight, 0),
    OPGET(healbonus, 0),
    OPGET(manabonus, 0),

    /* scroll, potion, pill */
    //spelllevel
    OPGET(spells, 0),

    /* armor */
    OPGET( ac, 0),

    /* weapon */
    OPGET( weapontype, 0),
    OPGET( numdice, 0),
    OPGET( dicetype, 0),
    OPGET( attacktype, 0),
    OPGET( damnoun, 0),

    /* container */
    //maxweight
    OPGET( key, 0),
    OPGET( capacity, 0),
    OPGET( weightmult, 0),

    /* drink container */
    OPGET( liquidtotal, 0),
    OPGET( liquidleft, 0),
    OPGET( liquid, 0),
    OPGET( liquidcolor, 0),
    OPGET( poisoned, 0),

    /*fountain*/
    //liquid
    //liquidleft
    //liquidtotal

    /* food */
    OPGET( foodhours, 0),
    OPGET( fullhours, 0),
    // poisoned
    
    /* money */
    OPGET( silver, 0),
    OPGET( gold, 0),

    ENDPTABLE
};

static const LUA_PROP_TYPE OBJPROTO_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE OBJPROTO_method_table [] =
{
    OPMETH( extra, 0),
    OPMETH( wear, 0),
    OPMETH( apply, 0),
   
    /* portal only */
    OPMETH( exitflag, 0),
    OPMETH( portalflag, 0),
    
    /* furniture only */
    OPMETH(furnitureflag, 0),
    
    /* weapon only */
    OPMETH(weaponflag, 0),
    
    /* container only */
    OPMETH(containerflag, 0),
    
    ENDPTABLE
}; 

/* end OBJPROTO section */

/* MOBPROTO section */
static int MOBPROTO_affected (lua_State *LS)
{
    MOB_INDEX_DATA *ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "affected", affect_flags, ud_mobp->affected_by );
}

static int MOBPROTO_act (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "act", act_flags, ud_mobp->act );
}

static int MOBPROTO_offensive (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "offensive", off_flags, ud_mobp->off_flags );
}

static int MOBPROTO_immune (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "immune", imm_flags, ud_mobp->imm_flags );
}

static int MOBPROTO_vuln (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "vuln", vuln_flags, ud_mobp->vuln_flags );
}

static int MOBPROTO_resist (lua_State *LS)
{
    MOB_INDEX_DATA * ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "resist", res_flags, ud_mobp->res_flags );
}

#define MPGETSTR( field, val, hsumm, hinfo ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushstring(LS, val );\
    return 1;\
}

#define MPGETINT( field, val, hsumm, hinfo ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushinteger(LS, val );\
    return 1;\
}

#define MPGETBOOL( field, val, hsumm, hinfo ) static int MOBPROTO_get_ ## field (lua_State *LS)\
{\
    MOB_INDEX_DATA *ud_mobp=check_MOBPROTO(LS,1);\
    lua_pushboolean(LS, val );\
    return 1;\
}

MPGETINT( vnum, ud_mobp->vnum ,"" ,"" );
MPGETSTR( name, ud_mobp->player_name , "" ,"");
MPGETSTR( shortdescr, ud_mobp->short_descr,"" ,"");
MPGETSTR( longdescr, ud_mobp->long_descr,"" ,"");
MPGETSTR( description, ud_mobp->description, "", "");
MPGETINT( alignment, ud_mobp->alignment,"" ,"");
MPGETINT( level, ud_mobp->level,"" ,"");
MPGETSTR( damtype, attack_table[ud_mobp->dam_type].name,"" ,"");
MPGETSTR( startpos, flag_bit_name(position_flags, ud_mobp->start_pos),"" ,"");
MPGETSTR( defaultpos, flag_bit_name(position_flags, ud_mobp->default_pos),"" ,"");
MPGETSTR( sex,
    ud_mobp->sex == SEX_NEUTRAL ? "neutral" :
    ud_mobp->sex == SEX_MALE    ? "male" :
    ud_mobp->sex == SEX_FEMALE  ? "female" :
    NULL,"" ,"");
MPGETSTR( race, race_table[ud_mobp->race].name,"" ,"");
MPGETSTR( size, flag_bit_name(size_flags, ud_mobp->size),"" ,"");
MPGETINT( count, ud_mobp->count, "", "");

static int MOBPROTO_get_area (lua_State *LS)
{
    if (push_AREA( LS, (check_MOBPROTO( LS, 1))->area))
        return 1;
    return 0;
}

static int MOBPROTO_get_mtrigs ( lua_State *LS)
{
    MOB_INDEX_DATA *ud_mid=check_MOBPROTO( LS, 1);
    MPROG_LIST *mtrig;
    
    int index=1;
    lua_newtable( LS );

    for ( mtrig = ud_mid->mprogs ; mtrig ; mtrig = mtrig->next )
    {
        if (push_MTRIG( LS, mtrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static int MOBPROTO_get_shop ( lua_State *LS)
{
    MOB_INDEX_DATA *ud_mid=check_MOBPROTO( LS, 1);
    if ( ud_mid->pShop )
    {
        if ( push_SHOP(LS, ud_mid->pShop) )
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

static const LUA_PROP_TYPE MOBPROTO_get_table [] =
{
    MPGET( vnum, 0),
    MPGET( name, 0),
    MPGET( shortdescr, 0),
    MPGET( longdescr, 0),
    MPGET( description, 0),
    MPGET( alignment, 0),
    MPGET( level, 0),
    MPGET( damtype, 0),
    MPGET( startpos, 0),
    MPGET( defaultpos, 0),
    MPGET( sex, 0),
    MPGET( race, 0),
    MPGET( size, 0),
    MPGET( area, 0),
    MPGET( mtrigs, 0),
    MPGET( shop, 0),
    MPGET( count,0),
    ENDPTABLE
};

static const LUA_PROP_TYPE MOBPROTO_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE MOBPROTO_method_table [] =
{
    MPMETH( act, 0),
    MPMETH( vuln, 0),
    MPMETH( immune, 0),
    MPMETH( offensive, 0),
    MPMETH( resist, 0),
    MPMETH( affected, 0),
    ENDPTABLE
}; 

/* end MOBPROTO section */

/* SHOP section */
#define SHOPGETINT( lfield, cfield ) static int SHOP_get_ ## lfield ( lua_State *LS )\
{\
    SHOP_DATA *ud_shop=check_SHOP( LS, 1);\
    lua_pushinteger( LS,\
            ud_shop->cfield );\
    return 1;\
}

SHOPGETINT( keeper, keeper)
SHOPGETINT( profitbuy, profit_buy)
SHOPGETINT( profitsell, profit_sell)
SHOPGETINT( openhour, open_hour)
SHOPGETINT( closehour, close_hour)

static int SHOP_buytype ( lua_State *LS )
{
    SHOP_DATA *ud_shop=check_SHOP(LS, 1);
    if (lua_isnone( LS, 2) ) // no arg
    {
        lua_newtable(LS);
        int i;
        int index=1;
        for (i=0; i<MAX_TRADE; i++)
        {
            if (ud_shop->buy_type[i] != 0)
            {
                lua_pushstring( LS,
                        flag_bit_name(type_flags, ud_shop->buy_type[i]));
                lua_rawseti( LS, -2, index++);
            }
        }
        return 1;
    } 

    // arg was given
    const char *arg=check_string(LS, 2, MIL);
    int flag=flag_lookup(arg, type_flags);
    if ( flag==NO_FLAG )
        luaL_error(LS, "No such type flag '%s'", arg);

    int i;
    for (i=0; i<MAX_TRADE ; i++)
    {
        if (ud_shop->buy_type[i] == flag)
        {
            lua_pushboolean( LS, TRUE);
            return 1;
        }
    }

    lua_pushboolean( LS, FALSE );
    return 1;
}
       
static const LUA_PROP_TYPE SHOP_get_table [] =
{
    SHOPGET( keeper, 0),
    SHOPGET( profitbuy, 0),
    SHOPGET( profitsell, 0),
    SHOPGET( openhour, 0),
    SHOPGET( closehour, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE SHOP_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE SHOP_method_table [] =
{
    SHOPMETH( buytype, 0),
    ENDPTABLE
};
/* end SHOP section */

/* AFFECT section */
static int AFFECT_get_where ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushstring( LS,
            flag_bit_name(apply_types, ud_af->where) );
    return 1;
}

static int AFFECT_get_type ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    if (ud_af->type < 1)
    {
        lua_pushliteral( LS, "none");
        return 1;
    }
    else
    {
        lua_pushstring( LS,
                skill_table[ud_af->type].name );
        return 1;
    }
}

static int AFFECT_get_location ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushstring( LS,
            flag_bit_name(apply_flags, ud_af->location) );
    return 1;
}

static int AFFECT_get_level ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->level);
    return 1;
}

static int AFFECT_get_duration ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->duration);
    return 1;
}

static int AFFECT_get_modifier ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->modifier);
    return 1;
}

static int AFFECT_get_bitvector ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    switch (ud_af->where)
    {
        case TO_AFFECTS:
            lua_pushstring(LS,
                    flag_bit_name( affect_flags, ud_af->bitvector ) ); 
            break;
        case TO_OBJECT:
            lua_pushstring(LS, 
                    flag_bit_name( extra_flags, ud_af->bitvector ) ); 
            break;
        case TO_WEAPON:
            /* TODO: make it return table of flags */
            lua_pushstring(LS,
                    flag_string( weapon_type2, ud_af->bitvector ) );
            break;
        case TO_IMMUNE:
            lua_pushstring(LS,
                    flag_bit_name( imm_flags, ud_af->bitvector ) );
            break;
        case TO_RESIST:
            lua_pushstring(LS,
                    flag_bit_name( res_flags, ud_af->bitvector ) );
            break;
        case TO_VULN:
            lua_pushstring(LS,
                    flag_bit_name( vuln_flags, ud_af->bitvector ) );
            break;
        default:
            luaL_error( LS, "Invalid where." );
    }
    return 1;
}

static const LUA_PROP_TYPE AFFECT_get_table [] =
{
    AFFGET( where, 0),
    AFFGET( type, 0),
    AFFGET( level, 0),
    AFFGET( duration, 0),
    AFFGET( location, 0),
    AFFGET( modifier, 0),
    AFFGET( bitvector, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE AFFECT_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE AFFECT_method_table [] =
{
    ENDPTABLE
};


/* end AFFECT section */

/* MPROG section */
static int MPROG_get_islua ( lua_State *LS )
{
    lua_pushboolean( LS,
            (check_MPROG( LS, 1) )->is_lua);
    return 1;
}

static int MPROG_get_vnum ( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_MPROG( LS, 1) )->vnum);
    return 1;
}

static int MPROG_get_code ( lua_State *LS )
{
    lua_pushstring( LS,
            (check_MPROG( LS, 1) )->code);
    return 1;
}

static int MPROG_get_security ( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_MPROG( LS, 1) )->security);
    return 1;
}

static const LUA_PROP_TYPE MPROG_get_table [] =
{
    MPROGGET( islua, 0),
    MPROGGET( vnum, 0),
    MPROGGET( code, 0),
    MPROGGET( security, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE MPROG_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE MPROG_method_table [] =
{
    ENDPTABLE
};
/* end MPROG section */

/* MTRIG section */
static int MTRIG_get_trigtype ( lua_State *LS )
{
    MPROG_LIST *ud_mpl=check_MTRIG(LS,1);

    lua_pushstring( LS,
            flag_bit_name(
                mprog_flags,
                ud_mpl->trig_type));
    return 1;
}

static int MTRIG_get_trigphrase ( lua_State *LS )
{
    MPROG_LIST *ud_mpl=check_MTRIG(LS,1);

    lua_pushstring( LS,
            ud_mpl->trig_phrase);
    return 1;
}

static int MTRIG_get_prog ( lua_State *LS )
{
    MPROG_LIST *ud_mpl=check_MTRIG(LS,1);

    lua_pushstring( LS,
            ud_mpl->code);
    return 1;
}    

static const LUA_PROP_TYPE MTRIG_get_table [] =
{
    MTRIGGET( trigtype, 0),
    MTRIGGET( trigphrase, 0),
    MTRIGGET( prog, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE MTRIG_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE MTRIG_method_table [] =
{
    ENDPTABLE
};

/* end TRIG section */

/* HELP section */
static int HELP_get_level( lua_State *LS )
{
    lua_pushinteger( LS, check_HELP( LS, 1 )->level );
    return 1;
}

static int HELP_get_keywords( lua_State *LS )
{
    lua_pushstring( LS, check_HELP( LS, 1 )->keyword );
    return 1;
}

static int HELP_get_text( lua_State *LS )
{
    lua_pushstring( LS, check_HELP( LS, 1 )->text );
    return 1;
}

static const LUA_PROP_TYPE HELP_get_table [] =
{
    GETP( HELP, level, 0 ),
    GETP( HELP, keywords, 0 ),
    GETP( HELP, text, 0 ),
    ENDPTABLE
};

static const LUA_PROP_TYPE HELP_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE HELP_method_table [] =
{
    ENDPTABLE
};

/* end HELP section */

/* DESCRIPTOR section */
static int DESCRIPTOR_get_character( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1 );

    if (!ud_d->character)
        return 0;

    if ( push_CH( LS,
                ud_d->original ? ud_d->original : ud_d->character ) )
        return 1;
    else
        return 0;
}

static const LUA_PROP_TYPE DESCRIPTOR_get_table [] =
{
    GETP( DESCRIPTOR, character, 0 ),
    ENDPTABLE
};

static const LUA_PROP_TYPE DESCRIPTOR_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE DESCRIPTOR_method_table [] =
{
    ENDPTABLE
};
/* end DESCRIPTOR section */

void type_init( lua_State *LS)
{
    int i;

    for ( i=0 ; type_list[i] ; i++ )
    {
        type_list[i]->reg( LS );
    }
}

void cleanup_uds()
{
    lua_newtable( g_mud_LS );
    lua_setglobal( g_mud_LS, "cleanup" );
} 

/* all the valid checks do the same ATM, fix this if they ever don't */
bool valid_UD( void *ud )
{
    return valid_CH( (CHAR_DATA*)ud );
}

#define REF_FREED -1

#define declb( LTYPE , CTYPE , TPREFIX ) \
typedef struct\
{\
    CTYPE a;\
    int ref;\
} LTYPE ## _wrapper;\
\
bool valid_ ## LTYPE ( CTYPE *ud )\
{\
    bool rtn;\
    lua_getfield( g_mud_LS, LUA_GLOBALSINDEX, "validuds" );\
    lua_pushlightuserdata( g_mud_LS, ud );\
    lua_gettable( g_mud_LS, -2 );\
    if (lua_isnil( g_mud_LS, -1 ))\
        rtn=FALSE;\
    else\
        rtn=TRUE;\
    lua_pop( g_mud_LS, 2 ); /* pop result and validuds */\
    return rtn;\
}\
\
CTYPE * check_ ## LTYPE ( lua_State *LS, int index )\
{\
    return luaL_checkudata( LS, index, #LTYPE );\
}\
\
bool    is_ ## LTYPE ( lua_State *LS, int index )\
{\
    lua_getmetatable( LS, index );\
    luaL_getmetatable( LS, #LTYPE );\
    bool result=lua_equal( LS, -1, -2 );\
    lua_pop( LS, 2 );\
    return result;\
}\
\
bool    push_ ## LTYPE ( lua_State *LS, CTYPE *ud )\
{\
    if (!ud)\
    {\
        bugf( "NULL ud passed to push_" #LTYPE );\
        return FALSE;\
    }\
    if ( ! valid_ ## LTYPE ( ud ) )\
    {\
        bugf( "Invalid " #CTYPE " in push_" #LTYPE );\
        return FALSE;\
    }\
    int ref=(( LTYPE ## _wrapper *)ud)->ref;\
    if (ref==REF_FREED)\
        return FALSE;\
    \
    lua_rawgeti( LS, LUA_REGISTRYINDEX, ref );\
    \
    return TRUE;\
}\
\
CTYPE * alloc_ ## LTYPE ( void )\
{\
    LTYPE ## _wrapper *wrap=lua_newuserdata( g_mud_LS, sizeof( LTYPE ## _wrapper ) );\
    luaL_getmetatable( g_mud_LS, #LTYPE );\
    lua_setmetatable( g_mud_LS, -2 );\
    wrap->ref=luaL_ref( g_mud_LS, LUA_REGISTRYINDEX );\
    LTYPE ## _type.count++;\
    memset( wrap, 0, sizeof( CTYPE ) );\
    /* register in validuds table for valid checks later on */\
    lua_getfield( g_mud_LS, LUA_GLOBALSINDEX, "validuds" );\
    lua_pushlightuserdata( g_mud_LS, wrap );\
    lua_pushboolean( g_mud_LS, TRUE );\
    lua_settable( g_mud_LS, -3 );\
    lua_pop( g_mud_LS, 1 );\
    return (CTYPE*)wrap;\
}\
\
void free_ ## LTYPE ( CTYPE * ud )\
{\
    if ( ! valid_ ## LTYPE ( ud ) )\
    {\
        bugf( "Invalid " #CTYPE " in free_" #LTYPE );\
        return;\
    }\
    LTYPE ## _wrapper *wrap = (LTYPE ## _wrapper *)ud;\
    int ref=wrap->ref;\
    if ( ref == REF_FREED )\
    {\
        bugf( "Tried to free already freed " #LTYPE );\
        return;\
    }\
    /* destroy env */\
    lua_getglobal( g_mud_LS, "envtbl" );\
    push_ ## LTYPE ( g_mud_LS, ud );\
    lua_pushnil( g_mud_LS );\
    lua_settable( g_mud_LS, -3 );\
    lua_pop( g_mud_LS, 1 ); /* pop envtbl */\
    \
    /* move to cleanup table */\
    lua_getglobal( g_mud_LS, "cleanup" );\
    push_ ## LTYPE ( g_mud_LS, ud );\
    luaL_ref( g_mud_LS, -2 );\
    lua_pop( g_mud_LS, 1 ); /* pop cleanup */\
    \
    wrap->ref=REF_FREED;\
    luaL_unref( g_mud_LS, LUA_REGISTRYINDEX, ref );\
    LTYPE ## _type.count--;\
    /* unregister from validuds table */\
    lua_getfield( g_mud_LS, LUA_GLOBALSINDEX, "validuds" );\
    lua_pushlightuserdata( g_mud_LS, ud );\
    lua_pushnil( g_mud_LS );\
    lua_settable( g_mud_LS, -3 );\
    lua_pop( g_mud_LS, 1 );\
}\
\
int count_ ## LTYPE ( void )\
{\
    int count=0;\
    luaL_getmetatable( g_mud_LS, #LTYPE );\
    lua_pushnil( g_mud_LS );\
    \
    while (lua_next( g_mud_LS, LUA_REGISTRYINDEX ))\
    {\
        if ( lua_isuserdata( g_mud_LS, -1 ) )\
        {\
            lua_getmetatable( g_mud_LS, -1 );\
            if (lua_equal( g_mud_LS, -1, -4 ))\
            {\
                count++;\
            }\
            lua_pop( g_mud_LS, 1 );\
        }\
        lua_pop( g_mud_LS, 1 );\
    }\
    \
    lua_pop( g_mud_LS, 1 );\
    \
    return count;\
}\
\
static int newindex_ ## LTYPE ( lua_State *LS )\
{\
    CTYPE * gobj = (CTYPE*)check_ ## LTYPE ( LS, 1 );\
    const char *arg=check_string( LS, 2, MIL );\
    \
    if (! valid_ ## LTYPE ( gobj ) )\
    {\
        luaL_error( LS, "Tried to index invalid " #LTYPE ".");\
    }\
    \
    lua_remove(LS, 2);\
    \
    const LUA_PROP_TYPE *set = TPREFIX ## _set_table ;\
    \
    int i;\
    for (i=0 ; set[i].field ; i++ )\
    {\
        if ( !strcmp(set[i].field, arg) )\
        {\
            if ( set[i].security > g_ScriptSecurity )\
                luaL_error( LS, "Current security %d. Setting field requires %d.",\
                        g_ScriptSecurity,\
                        set[i].security);\
            \
            if ( set[i].func )\
            {\
                lua_pushcfunction( LS, set[i].func );\
                lua_insert( LS, 1 );\
                lua_call(LS, 2, 0);\
                return 0;\
            }\
            else\
            {\
                bugf("No function entry for %s %s.",\
                        #LTYPE , arg );\
                luaL_error(LS, "No function found.");\
            }\
        }\
    }\
    \
    luaL_error(LS, "Can't set field '%s' for type %s.",\
            arg, #LTYPE );\
    \
    return 0;\
}\
\
static int index_ ## LTYPE ( lua_State *LS )\
{\
    CTYPE * gobj = (CTYPE*)check_ ## LTYPE ( LS, 1 );\
    const char *arg=luaL_checkstring( LS, 2 );\
    const LUA_PROP_TYPE *get = TPREFIX ## _get_table;\
    \
    bool valid=valid_ ## LTYPE ( gobj );\
    if (!strcmp("valid", arg))\
    {\
        lua_pushboolean( LS, valid );\
        return 1;\
    }\
    \
    if (!valid)\
    {\
        luaL_error( LS, "Tried to index invalid " #LTYPE ".");\
    }\
    \
    int i;\
    for (i=0; get[i].field; i++ )\
    {\
        if (!strcmp(get[i].field, arg) )\
        {\
            if ( get[i].security > g_ScriptSecurity )\
                luaL_error( LS, "Current security %d. Getting field requires %d.",\
                        g_ScriptSecurity,\
                        get[i].security);\
            \
            if (get[i].func)\
            {\
                int val;\
                val=(get[i].func)(LS, gobj);\
                return val;\
            }\
            else\
            {\
                bugf("No function entry for %s %s.",\
                        #LTYPE, arg );\
                luaL_error(LS, "No function found.");\
            }\
        }\
    }\
    \
    const LUA_PROP_TYPE *method= TPREFIX ## _method_table ;\
    \
    for (i=0; method[i].field; i++ )\
    {\
        if (!strcmp(method[i].field, arg) )\
        {\
            if ( method[i].security > g_ScriptSecurity )\
                luaL_error( LS, "Current security %d. Method requires %d.",\
                        g_ScriptSecurity,\
                        method[i].security);\
            \
            lua_pushcfunction(LS, method[i].func);\
            return 1;\
        }\
    }\
    \
    return 0;\
}\
\
void reg_ ## LTYPE (lua_State *LS)\
{\
    luaL_newmetatable( LS, #LTYPE );\
    \
    lua_pushcfunction( LS, index_ ## LTYPE );\
    lua_setfield( LS, -2, "__index");\
    \
    lua_pushcfunction( LS, newindex_ ## LTYPE );\
    lua_setfield( LS, -2, "__newindex");\
    \
    luaL_loadstring( LS, "return \"" #LTYPE "\"" );\
    lua_setfield( LS, -2, "__tostring");\
    \
    lua_pushlightuserdata( LS, (void *)  & LTYPE ## _type);\
    lua_setfield( LS, -2, "TYPE" );\
    \
    lua_pop( LS, 1 );\
}\
\
LUA_OBJ_TYPE LTYPE ## _type = { \
    .type_name= #LTYPE ,\
    .valid = valid_ ## LTYPE ,\
    .check = (void*(*)())check_ ## LTYPE ,\
    .is    = is_ ## LTYPE ,\
    .push  = push_ ## LTYPE ,\
    .alloc = (void*(*)())alloc_ ## LTYPE ,\
    .free  = free_ ## LTYPE ,\
    \
    .index = index_ ## LTYPE ,\
    .newindex= newindex_ ## LTYPE ,\
    \
    .reg   = reg_ ## LTYPE ,\
    \
    .get_table= TPREFIX ## _get_table ,\
    .set_table= TPREFIX ## _set_table ,\
    .method_table = TPREFIX ## _method_table ,\
    \
    .count=0 \
};

#define DECLARETYPE( LTYPE, CTYPE ) declb( LTYPE, CTYPE, LTYPE )

DECLARETYPE( CH, CHAR_DATA );
DECLARETYPE( OBJ, OBJ_DATA );
DECLARETYPE( AREA, AREA_DATA );
DECLARETYPE( ROOM, ROOM_INDEX_DATA );
DECLARETYPE( EXIT, EXIT_DATA );
DECLARETYPE( RESET, RESET_DATA );
DECLARETYPE( OBJPROTO, OBJ_INDEX_DATA );
DECLARETYPE( MOBPROTO, MOB_INDEX_DATA );
DECLARETYPE( SHOP, SHOP_DATA );
DECLARETYPE( AFFECT, AFFECT_DATA );
DECLARETYPE( MPROG, MPROG_CODE );
DECLARETYPE( MTRIG, MPROG_LIST );
DECLARETYPE( HELP, HELP_DATA );
DECLARETYPE( DESCRIPTOR, DESCRIPTOR_DATA );
