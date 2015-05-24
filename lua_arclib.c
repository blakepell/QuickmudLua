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
#include "mudconfig.h"
#include "religion.h"
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
    &PROG_type,
    &MTRIG_type,
    &OTRIG_type,
    &ATRIG_type,
    &RTRIG_type,
    &HELP_type,
    &DESCRIPTOR_type,
    &BOSSACHV_type,
    &BOSSREC_type,
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

#define PROGGET( field, sec) GETP( PROG, field, sec)

#define TRIGGET( field, sec) GETP( TRIG, field, sec)

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
            
static int godlib_helper_get_duration(lua_State* LS, int index)
{
    if (lua_isnone(LS, index))
    {
        return GOD_FUNC_DEFAULT_DURATION;
    }
    else
    {
        return luaL_checkinteger(LS, index);
    }
}

static int godlib_bless (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_bless( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_curse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_curse( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_heal (lua_State *LS)
{

    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_heal( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_speed (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_speed( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1; 
}

static int godlib_slow (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_slow( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1; 
}

static int godlib_cleanse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_cleanse( NULL, ch, "", GOD_FUNC_DEFAULT_DURATION ));
    return 1; 
}

static int godlib_defy (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_defy( NULL, ch, "", GOD_FUNC_DEFAULT_DURATION ));
    return 1; 
}

static int godlib_enlighten (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_enlighten( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1; 
}

static int godlib_protect (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_protect( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_fortune (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_fortune( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_haunt (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_haunt( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_plague (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    lua_pushboolean( LS,
            god_plague( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int godlib_confuse (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);

    

    lua_pushboolean( LS,
            god_confuse( NULL, ch, "", godlib_helper_get_duration(LS, 2) ));
    return 1;
}

static int glob_transfer (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *arg=check_string(LS,2,MIL);
    ROOM_INDEX_DATA *location=find_mp_location( ch, arg); 

    if (!location)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }
    
    transfer_char( ch, location );
    lua_pushboolean( LS, TRUE);
    return 1;
}

static int glob_gtransfer (lua_State *LS)
{
    CHAR_DATA *ch=check_CH(LS,1);
    const char *arg=check_string(LS,2,MIL);
    ROOM_INDEX_DATA *location=find_location( ch, arg);

    if (!location)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }

    CHAR_DATA *victim, *next_char;
    for ( victim=ch->in_room->people; victim; victim=next_char )
    {
        next_char=victim->next_in_room;
        if ( is_same_group( ch, victim ) )
        {
            transfer_char(victim, location);
        }
    }

    lua_pushboolean( LS, TRUE);
    return 1;
}

static int glob_gecho (lua_State *LS)
{
    DESCRIPTOR_DATA *d;
    const char *argument=check_fstring( LS, 1, MIL );

    for ( d=descriptor_list; d; d=d->next )
    {
        if ( IS_PLAYING(d->connected ) )
        {
            if ( IS_IMMORTAL(d->character) )
                send_to_char( "gecho> ", d->character );
            send_to_char( argument, d->character );
            send_to_char( "\n\r", d->character );
        }
    }

    return 0;
}

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

static int glob_dammessage (lua_State *LS)
{
    const char *vs;
    const char *vp;
    char punct;
    char punctstr[2];
    get_damage_messages( 
            luaL_checkinteger( LS, 1 ),
            0, &vs, &vp, &punct );

    punctstr[0]=punct;
    punctstr[1]='\0';

    lua_pushstring( LS, vs );
    lua_pushstring( LS, vp );
    lua_pushstring( LS, punctstr);

    return 3;
}

static int glob_do_luaquery ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "do_luaquery");
    lua_insert(LS, 1);
    lua_call( LS, top, LUA_MULTRET );

    return lua_gettop(LS);
}

static void push_mudconfig_val( lua_State *LS, CFG_DATA_ENTRY *en )
{
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
}

static int glob_mudconfig (lua_State *LS)
{
    int i;
    CFG_DATA_ENTRY *en;

    /* no arg, return the whole table */
    if (lua_isnone(LS,1))
    {
        lua_newtable(LS);
        for ( i=0 ; mudconfig_table[i].name ; i++ )
        {
            en=&mudconfig_table[i];
            push_mudconfig_val( LS, en );
            lua_setfield(LS, -2, en->name);
        }
        return 1;
    }

    const char *arg1=check_string(LS, 1, MIL);
    /* 1 argument, return the value */
    if (lua_isnone(LS, 2))
    {
        for ( i=0 ; mudconfig_table[i].name ; i++ )
        {
            en=&mudconfig_table[i];
            if (!strcmp(en->name, arg1))
            {
                push_mudconfig_val( LS, en );
                return 1;
            }
        }
        luaL_error(LS, "No such mudconfig value: %s", arg1);
    }

    /* 2 args, set the value */
    for ( i=0 ; mudconfig_table[i].name ; i++ )
    {
        en=&mudconfig_table[i];
        if (!strcmp(en->name, arg1))
        {
            switch( en->type )
            {
                case CFG_INT:
                    {
                        *((int *)(en->value))=luaL_checkinteger( LS, 2 );
                        break;
                    }
                case CFG_FLOAT:
                    {
                        *((float *)(en->value))=luaL_checknumber(LS, 2 );
                        break;
                    }
                case CFG_STRING:
                    {
                        const char *newval=check_string(LS, 2, MIL);
                        *((const char **)(en->value))=str_dup(newval);
                        break;
                    }
                case CFG_BOOL:
                    {
                        luaL_checktype( LS, 2, LUA_TBOOLEAN );
                        *((bool *)(en->value))=lua_toboolean(LS, 2 );
                        break;
                    }
                default:
                    {
                        luaL_error( LS, "Bad type.");
                    }
            }
            return 0;
        }
    }
    luaL_error(LS, "No such mudconfig value: %s", arg1);

    return 0;
}

static int glob_start_con_handler( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "start_con_handler");
    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
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
/* Mersenne Twister pseudo-random number generator */

static int mtlib_srand (lua_State *LS)
{
    int i;

    /* allow for table of seeds */

    if (lua_istable (LS, 1))
    {
        size_t length = lua_objlen (LS, 1);  /* size of table */
        if (length == 0)
            luaL_error (LS, "mt.srand table must not be empty");

        unsigned long * v = (unsigned long *) malloc (sizeof (unsigned long) * length);
        if (!v)
            luaL_error (LS, "Cannot allocate memory for seeds table");

        for (i = 1; i <= length; i++)
        {
            lua_rawgeti (LS, 1, i);  /* get number */
            if (!lua_isnumber (LS, -1))
            {
                free (v);  /* get rid of table now */
                luaL_error (LS, "mt.srand table must consist of numbers");
            }
            v [i - 1] = luaL_checknumber (LS, -1);
            lua_pop (LS, 1);   /* remove value   */
        }
        init_by_array (&v [0], length);
        free (v);  /* get rid of table now */
    }
    else
        init_genrand (luaL_checknumber (LS, 1));

    return 0;
} /* end of mtlib_srand */

static int mtlib_rand (lua_State *LS)
{
    lua_pushnumber (LS, (double)genrand ());
    return 1;
} /* end of mtlib_rand */

static int mudlib_luadir( lua_State *LS)
{
    lua_pushliteral( LS, LUA_DIR);
    return 1;
}

static int mudlib_userdir( lua_State *LS)
{
    lua_pushliteral( LS, USER_DIR);
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

static int glob_awardptitle( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal(LS, "glob_awardptitle");

    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_randnum ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_randnum");
    lua_insert( LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_rand ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_rand");
    lua_insert( LS, 1 );
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_tprintstr ( lua_State *LS)
{
    int top=lua_gettop(LS);
    lua_getglobal( LS, "glob_tprintstr");
    lua_insert(LS, 1);
    lua_call( LS, top, LUA_MULTRET );
    
    return lua_gettop(LS);
}

static int glob_getrandomroom ( lua_State *LS)
{
    ROOM_INDEX_DATA *room;

    int i;
    for ( i=0; i<10000; i++ ) // limit to 10k loops just in case
    {
        room=get_room_index(number_range(0,65535));
        if ( ( room )
                &&   is_room_ingame( room )
                &&   !room_is_private(room)
                &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
                &&   !IS_SET(room->room_flags, ROOM_SOLITARY)
                &&   !IS_SET(room->room_flags, ROOM_SAFE)
                &&   !IS_SET(room->room_flags, ROOM_JAIL)
                &&   !IS_SET(room->room_flags, ROOM_NO_TELEPORT) )
            break;
    }

    if (!room)
        luaL_error(LS, "Couldn't get a random room.");

    if (push_ROOM(LS,room))
        return 1;
    else
        return 0;

}

/* currently unused - commented out to avoid warning
static int glob_cancel ( lua_State *LS)
{
    return L_cancel(LS);
}
*/

static int glob_arguments ( lua_State *LS)
{   
    const char *argument=check_string( LS, 1, MIL );
    char buf[MIL];
    bool keepcase=FALSE;

    if (!lua_isnone(LS,2))
    {
        keepcase=lua_toboolean(LS, 2);
    }
    
    lua_newtable( LS );
    int index=1;

    while ( argument[0] != '\0' )
    {
        if (keepcase)
            argument=one_argument_keep_case( argument, buf);
        else
            argument = one_argument( argument, buf );
        lua_pushstring( LS, buf );
        lua_rawseti( LS, -2, index++ );
    }

    return 1;
}
        


#define ENDGTABLE { NULL, NULL, NULL, 0, 0 }
#define GFUN( fun, sec ) { NULL, #fun , glob_ ## fun , sec, STS_ACTIVE }
#define LFUN( lib, fun, sec) { #lib, #fun, lib ## lib_ ## fun , sec, STS_ACTIVE}
#define GODF( fun ) LFUN( god, fun, 9 )
#define DBGF( fun ) LFUN( dbg, fun, 9 )
#define UTILF( fun ) LFUN( util, fun, 0)

GLOB_TYPE glob_table[] =
{
    GFUN(hour,          0),
    GFUN(gettime,       0),
    GFUN(getroom,       0),
    GFUN(randnum,       0),
    GFUN(rand,          0),
    GFUN(tprintstr,     0),
    GFUN(getobjproto,   0),
    GFUN(getobjworld,   0),
    GFUN(getmobproto,   0),
    GFUN(getmobworld,   0),
    GFUN(getpc,         0),
    GFUN(getrandomroom, 0),
    GFUN(transfer,      0),
    GFUN(gtransfer,     0),
    GFUN(awardptitle,   5),
    GFUN(sendtochar,    0),
    GFUN(echoat,        0),
    GFUN(echoaround,    0),
    GFUN(gecho,         0),
    GFUN(pagetochar,    0),
    GFUN( arguments,    0),
    GFUN(log,           0),
    GFUN(getcharlist,   9),
    GFUN(getobjlist,    9),
    GFUN(getmoblist,    9),
    GFUN(getplayerlist, 9),
    GFUN(getarealist,   9),
    GFUN(getshoplist,   9),
    GFUN(gethelplist,   9),
    GFUN(getdescriptorlist, 9),
    GFUN(dammessage,    0),
    GFUN(clearloopcount,9),
    GFUN(mudconfig,     9),
    GFUN(start_con_handler, 9),
    GFUN(forceget,      SEC_NOSCRIPT),
    GFUN(forceset,      SEC_NOSCRIPT),
    GFUN(getluatype,    SEC_NOSCRIPT),
    GFUN(getglobals,    SEC_NOSCRIPT),
#ifdef TESTER
    GFUN(do_luaquery,   9),
#else
    GFUN(do_luaquery,   SEC_NOSCRIPT),
#endif

    GODF(confuse),
    GODF(curse),
    GODF(plague),
    GODF(bless),
    GODF(slow),
    GODF(speed),
    GODF(heal),
    GODF(enlighten),
    GODF(protect),
    GODF(fortune),
    GODF(haunt),
    GODF(cleanse),
    GODF(defy),

    UTILF(trim),
    UTILF(convert_time),
    UTILF(capitalize),
    UTILF(pluralize),
    UTILF(format_list),
    UTILF(strlen_color),
    UTILF(truncate_color_string),
    UTILF(format_color_string),
    
    DBGF(show),

    /* SEC_NOSCRIPT means aren't available for prog scripts */

    LFUN( mt, srand,        SEC_NOSCRIPT ),
    LFUN( mt, rand,         SEC_NOSCRIPT ),

    LFUN( mud, luadir,      SEC_NOSCRIPT ),
    LFUN( mud, userdir,     SEC_NOSCRIPT ),
    
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
LUA_EXTRA_VAL *new_luaval( int type, const char *name, const char *val, bool persist )
{
    LUA_EXTRA_VAL *new=alloc_mem(sizeof(LUA_EXTRA_VAL));
    new->type=type;
    new->name=name;
    new->val=val;
    new->persist=persist;

    return new;
}

void free_luaval( LUA_EXTRA_VAL *luaval)
{
    free_string(luaval->name);
    free_string(luaval->val);

    free_mem(luaval, sizeof(LUA_EXTRA_VAL) );
}

static void push_luaval( lua_State *LS, LUA_EXTRA_VAL *luaval )
{
    switch(luaval->type)
    {
        case LUA_TSTRING:
            lua_pushstring(LS, luaval->val);
            return;

        case LUA_TNUMBER:
            lua_getglobal( LS, "tonumber");
            lua_pushstring(LS, luaval->val);
            lua_call( LS, 1, 1 );
            return;

        case LUA_TBOOLEAN:
            lua_pushboolean( LS, !strcmp( luaval->val, "true" ) );
            return;

        default:
            luaL_error(LS, "Invalid type '%s'",
                    lua_typename( LS, luaval->type) );
    }
}

static int get_luaval( lua_State *LS, LUA_EXTRA_VAL **luavals )
{
    if (lua_isnone( LS, 1 ) )
    {
        /* no argument, send a table of all vals */
        lua_newtable( LS );
        LUA_EXTRA_VAL *luaval;

        for (luaval=*luavals; luaval; luaval=luaval->next)
        {
            lua_pushstring( LS, luaval->name);
            push_luaval( LS, luaval );
            lua_rawset( LS, -3 );
        }
        return 1;
    }

    const char *name=check_string(LS, 1, MIL );

    LUA_EXTRA_VAL *luaval;

    for ( luaval=*luavals; luaval; luaval=luaval->next )
    {
        if (!strcmp( name, luaval->name) )
        {
            push_luaval( LS, luaval );
            return 1;
        }
    }

    lua_pushnil( LS );
    return 1;
}

static int set_luaval( lua_State *LS, LUA_EXTRA_VAL **luavals )
{
    const char *name=check_string(LS, 1, MIL );
    int type=lua_type(LS, 2 );
    const char *val;
    bool persist=FALSE;
    if (!lua_isnone(LS,3))
    {
        persist=lua_toboolean(LS, 3);
        lua_remove(LS,3);
    }

    switch(type)
    {
        case LUA_TNONE:
        case LUA_TNIL:
            /* just break 
               clear value lower down if it's already set,
               otherwise do nothing 
             */
            val = NULL;
            break;

        case LUA_TSTRING:
        case LUA_TNUMBER:
            val=check_string(LS, 2, MIL );
            break;

        case LUA_TBOOLEAN:
            lua_getglobal( LS, "tostring");
            lua_insert( LS, -2 );
            lua_call( LS, 1, 1 );
            val=check_string( LS, 2, MIL );
            break;

        default:
            return luaL_error( LS, "Cannot set value type '%s'.",
                    lua_typename( LS, type ) );
    }

    LUA_EXTRA_VAL *luaval;
    LUA_EXTRA_VAL *prev=NULL;
    LUA_EXTRA_VAL *luaval_next=NULL;

    for ( luaval=*luavals; luaval; luaval=luaval_next )
    {
        luaval_next=luaval->next;

        if (!strcmp( name, luaval->name) )
        {
            /* sending nil as 2nd arg actually comes through
               as "no value" (LUA_TNONE) for whatever reason*/
            if ( type == LUA_TNONE || type == LUA_TNIL)
            {
                if (prev)
                {
                    prev->next=luaval->next;
                }
                else
                {
                    /* top of the list */
                    *luavals=luaval->next;
                }
                free_luaval(luaval);
                return 0;
            }
            
            free_string( luaval->val );
            luaval->val = str_dup(smash_tilde_cc(val));
            luaval->type = type;
            luaval->persist= persist;
            return 0;
        }

        prev=luaval;
    }
    
    if ( type != LUA_TNONE && type != LUA_TNIL )
    {        
        luaval=new_luaval( 
                type, 
                str_dup( name ), 
                str_dup( smash_tilde_cc(val) ),
                persist );
        luaval->next = *luavals;
        *luavals     = luaval;
    }
    return 0;
}

static int L_rvnum( lua_State *LS, AREA_DATA *area )
{
    if (!area)
        luaL_error(LS, "NULL area in L_rvnum.");

    int nr=luaL_checkinteger(LS,1);
    int vnum=area->min_vnum + nr;

    if ( vnum < area->min_vnum || vnum > area->max_vnum )
        luaL_error(LS, "Rvnum %d (%d) out of area vnum bounds.", vnum, nr );

    lua_pushinteger(LS, vnum);
    return 1;
}

static int set_flag( lua_State *LS,
        const char *funcname, 
        const struct flag_type *flagtbl, 
        tflag flagvar )
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
        SET_BIT( flagvar, flag );
    else
        REMOVE_BIT( flagvar, flag);
        
    return 0;
}

static int set_iflag( lua_State *LS,
        const char *funcname,
        const struct flag_type *flagtbl,
        int *intvar)
{
    const char *argument = check_string( LS, 2, MIL);
    bool set = TRUE;
    if (!lua_isnone( LS, 3 ) )
    {
        set=lua_toboolean(LS,3);
    }

    int flag=flag_lookup( argument, flagtbl);
    if ( flag == NO_FLAG )
        return luaL_error(LS, "'%s' invalid flag for %s", argument, funcname);

    if ( set )
        I_SET_BIT( *intvar, flag);
    else
        I_REMOVE_BIT( *intvar, flag);

    return 0;
}
        

static int check_tflag_iflag( lua_State *LS, 
        const char *funcname, 
        const struct flag_type *flagtbl, 
        tflag flagvar,
        int intvar )
{
    if (lua_isnone( LS, 2)) /* called with no string arg */
    {
        /* return array of currently set flags */
        int index=1;
        lua_newtable( LS );
        int i;
        for ( i=0 ; flagtbl[i].name ; i++)
        {

            if ( (flagvar && IS_SET(flagvar, flagtbl[i].bit) )
                    || I_IS_SET( intvar, flagtbl[i].bit) )
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
    
    if (flagvar)
        lua_pushboolean( LS, IS_SET( flagvar, flag ) );
    else
        lua_pushboolean( LS, I_IS_SET( intvar, flag ) );
    return 1;
}

static int check_flag( lua_State *LS,
        const char *funcname,
        const struct flag_type *flagtbl,
        tflag flagvar)
{
    return check_tflag_iflag( LS,
            funcname,
            flagtbl,
            flagvar,
            0);
}

static int check_iflag( lua_State *LS,
        const char *funcname,
        const struct flag_type *flagtbl,
        int iflagvar)
{
    return check_tflag_iflag( LS,
            funcname,
            flagtbl,
            NULL,
            iflagvar);
}

static int L_rundelay( lua_State *LS)
{
    lua_getglobal( LS, "delaytbl"); /*2*/
    if (lua_isnil( LS, -1) )
    {
        luaL_error( LS, "run_delayed_function: couldn't find delaytbl");
    }

    lua_pushvalue( LS, 1 );
    lua_gettable( LS, 2 ); /* pops key */ /*3, delaytbl entry*/

    if (lua_isnil( LS, 3) )
    {
        luaL_error( LS, "Didn't find entry in delaytbl");
    }

    lua_getfield( LS, -1, "udobj");
    lua_getfield( LS, -1, "valid");
    if ( !lua_toboolean(LS, -1) )
    {
        /* game object was invalidated/destroyed */
        /* kill the entry and get out of here */
        lua_pushvalue( LS, 1 ); /* lightud as key */
        lua_pushnil( LS ); /* nil as value */
        lua_settable( LS, 2 ); /* pops key and value */

        return 0;
    }
    else
        lua_pop(LS, 1); // pop valid

    lua_getfield( LS, -2, "security");
    int sec=luaL_checkinteger( LS, -1);
    lua_pop(LS, 1);

    lua_getfield( LS, -2, "func"); 

    /* kill the entry before call in case of error */
    lua_pushvalue( LS, 1 ); /* lightud as key */
    lua_pushnil( LS ); /* nil as value */
    lua_settable( LS, 2 ); /* pops key and value */ 

    if ( is_CH( LS, -2 ) )
    {
        lua_mob_program( NULL, RUNDELAY_VNUM, NULL,
                check_CH(LS, -2), NULL, 
                NULL, 0, NULL, 0,
                TRIG_CALL, sec );
    }
    else if ( is_OBJ( LS, -2 ) )
    {
        lua_obj_program( NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -2), NULL,
                NULL, NULL,
                TRIG_CALL, sec );
    }
    else if ( is_AREA( LS, -2 ) )
    {
        lua_area_program( NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -2), NULL,
                TRIG_CALL, sec );
    }
    else if ( is_ROOM( LS, -2 ) )
    {
        lua_room_program( NULL, RUNDELAY_VNUM, NULL, 
                check_ROOM(LS, -2), NULL,
                NULL, NULL, NULL, NULL,
                TRIG_CALL, sec );
    }
    else
        luaL_error(LS, "Bad udobj type." );

    return 0;
}

void run_delayed_function( TIMER_NODE *tmr )
{
    lua_pushcfunction( g_mud_LS, L_rundelay );
    lua_pushlightuserdata( g_mud_LS, (void *)tmr );

    if (CallLuaWithTraceBack( g_mud_LS, 1, 0) )
    {
        bugf ( "Error running delayed function:\n %s",
                lua_tostring(g_mud_LS, -1));
        return;
    }

}

int L_delay (lua_State *LS)
{
    /* delaytbl has timer pointers as keys
       value is table with 'tableid' and 'func' keys */
    /* delaytbl[tmr]={ tableid=tableid, func=func } */
    const char *tag=NULL;
    int val=luaL_checkint( LS, 2 );
    luaL_checktype( LS, 3, LUA_TFUNCTION);
    if (!lua_isnone( LS, 4 ) )
    {
       tag=check_string( LS, 4, MIL );
    }

    lua_getglobal( LS, "delaytbl");
    TIMER_NODE *tmr=register_lua_timer( val, tag );
    lua_pushlightuserdata( LS, (void *)tmr);
    lua_newtable( LS );

/*
    lua_pushliteral( LS, "tableid");
    lua_getfield( LS, 1, "tableid");
    lua_settable( LS, -3 );
*/
    lua_pushliteral( LS, "udobj");
    lua_pushvalue( LS, 1 );
    lua_settable( LS, -3 );

    lua_pushliteral( LS, "func");
    lua_pushvalue( LS, 3 );
    lua_settable( LS, -3 );

    lua_pushliteral( LS, "security");
    lua_pushinteger( LS, g_ScriptSecurity ); 
    lua_settable( LS, -3 );

    lua_settable( LS, -3 );

    return 0;
}

int L_cancel (lua_State *LS)
{
    /* http://pgl.yoyo.org/luai/i/next specifies it is safe
       to modify or clear fields during iteration */
    /* for k,v in pairs(delaytbl) do
            if v.udobj==arg1 then
                unregister_lua_timer(k)
                delaytbl[k]=nil
            end
       end
       */

    /* 1, game object */
    const char *tag=NULL;
    if (!lua_isnone(LS, 2))
    {
        tag=check_string( LS, 2, MIL );
        lua_remove( LS, 2 );
    }

    lua_getglobal( LS, "delaytbl"); /* 2, delaytbl */

    lua_pushnil( LS );
    while ( lua_next(LS, 2) != 0 ) /* pops nil */
    {
        /* key at 3, val at 4 */
        lua_getfield( LS, 4, "udobj");
        if (lua_equal( LS, 5, 1 )==1)
        {
            luaL_checktype( LS, 3, LUA_TLIGHTUSERDATA);
            TIMER_NODE *tmr=(TIMER_NODE *)lua_touserdata( LS, 3);
            if (unregister_lua_timer( tmr, tag ) ) /* return false if tag no match*/
            {
                /* set table entry to nil */
                lua_pushvalue( LS, 3 ); /* push key */
                lua_pushnil( LS );
                lua_settable( LS, 2 );
            }
        }
        lua_pop(LS, 2); /* pop udobj and value */
    }

    return 0;
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

OBJVGETINT( arrowcount, ITEM_ARROWS, 0 )

OBJVGETINT( arrowdamage, ITEM_ARROWS, 1 )

OBJVGETSTR( arrowdamtype, ITEM_ARROWS, 
        flag_bit_name(damage_type, ud_obj->value[2]) )

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

OBJVGETSTR( damtype, ITEM_WEAPON, 
        flag_bit_name(damage_type, attack_table[ud_obj->value[3]].damage) )

OBJVGETSTR( damnoun, ITEM_WEAPON, attack_table[ud_obj->value[3]].noun )

static int OBJ_get_damavg( lua_State *LS )
{
    OBJ_DATA *ud_obj=check_OBJ( LS, 1);
    if (ud_obj->item_type != ITEM_WEAPON )
        luaL_error(LS, "damavg for %s only.", 
                item_name( ITEM_WEAPON ) );
    
    lua_pushinteger( LS, average_weapon_dam( ud_obj ) );
    return 1;
}

static int OBJPROTO_get_damavg( lua_State *LS )
{
    OBJ_INDEX_DATA *ud_obj=check_OBJPROTO( LS, 1);
    if (ud_obj->item_type != ITEM_WEAPON )
        luaL_error(LS, "damavg for %s only.",
                item_name( ITEM_WEAPON ) );
    
    lua_pushinteger( LS, average_weapon_index_dam( ud_obj ) );
    return 1;
}

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
    return check_iflag( LS, #funcname, flagtbl, ud_obj->value[ vind ] );\
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
static int CH_rvnum ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_remove(LS,1);

    if (IS_NPC(ud_ch))
        return L_rvnum( LS, ud_ch->pIndexData->area );
    else if (!ud_ch->in_room)
        return luaL_error(LS, "%s not in a room.", ud_ch->name );
    else
        return L_rvnum( LS, ud_ch->in_room->area );
}

static int CH_setval ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_remove(LS, 1);
    return set_luaval( LS, &(ud_ch->luavals) );
}

static int CH_getval ( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_remove(LS,1);
    return get_luaval( LS, &(ud_ch->luavals) );
}

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
    if ( lua_isstring(LS, 2) )
        do_mpkill( check_CH(LS, 1), check_string( LS, 2, MIL));
    else
        mpkill( check_CH(LS, 1),
                check_CH(LS, 2) );

    return 0;
}

static int CH_assist (lua_State *LS)
{
    if ( lua_isstring(LS, 2) )
        do_mpassist( check_CH(LS, 1), check_string( LS, 2, MIL));
    else
        mpassist( check_CH(LS, 1), 
                check_CH(LS, 2) );
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

static int CH_echoaround (lua_State *LS)
{
    if ( !is_CH(LS, 2) )
    {
        /* standard 'mob echoaround' syntax */
        do_mpechoaround( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpechoaround( check_CH(LS, 1), check_CH(LS, 2), check_fstring( LS, 3, MIL) );

    return 0;
}

static int CH_echoat (lua_State *LS)
{
    if ( lua_isnone(LS, 3) )
    {
        /* standard 'mob echoat' syntax */
        do_mpechoat( check_CH(LS, 1), check_string(LS, 2, MIL));
        return 0;
    }

    mpechoat( check_CH(LS, 1), check_CH(LS, 2), check_fstring( LS, 3, MIL) );
    return 0;
}

static int CH_mload (lua_State *LS)
{
    CHAR_DATA *mob=mpmload( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    if ( mob && push_CH(LS,mob) )
        return 1;
    else
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

static int CH_transfer (lua_State *LS)
{

    do_mptransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_gtransfer (lua_State *LS)
{

    do_mpgtransfer( check_CH(LS, 1), check_fstring( LS, 2, MIL));

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
    
    bool kill;
    if ( !lua_isnone( LS, 4 ) )
    {
        kill=lua_toboolean( LS, 4 );
    }
    else
    {
        kill=TRUE;
    }

    int damtype;
    if ( !lua_isnone( LS, 5 ) )
    {
        const char *dam_arg=check_string( LS, 5, MIL );
        damtype=flag_lookup( dam_arg, damage_type );
        if ( damtype == NO_FLAG )
            luaL_error(LS, "No such damage type '%s'",
                    dam_arg );
    }
    else
    {
        damtype=DAM_NONE;
    }

    lua_pushboolean( LS,
            deal_damage(ud_ch, victim, dam, TYPE_UNDEFINED, damtype, FALSE, kill) );
    return 1;
}

static int CH_remove (lua_State *LS)
{

    do_mpremove( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_remort (lua_State *LS)
{
    if ( !is_CH(LS, 2) )
    {
        /* standard 'mob remort' syntax */
        do_mpremort( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpremort( check_CH(LS, 1), check_CH(LS, 2));

    return 0;
}

static int CH_qset (lua_State *LS)
{
    if ( !is_CH( LS, 2 ) )
    {
        /* standard 'mob qset' syntax */
        do_mpqset( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }


    mpqset( check_CH(LS, 1), check_CH(LS, 2),
            check_string(LS, 3, MIL), check_string(LS, 4, MIL),
            lua_isnone( LS, 5 ) ? 0 : (int)luaL_checknumber( LS, 5),
            lua_isnone( LS, 6 ) ? 0 : (int)luaL_checknumber( LS, 6) );

    return 0;
}

static int CH_qadvance (lua_State *LS)
{
    if ( !is_CH( LS, 2) )
    {
        /* standard 'mob qset' syntax */
        do_mpqadvance( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpqadvance( check_CH(LS, 1), check_CH(LS, 2),
            check_string(LS, 3, MIL),
            lua_isnone( LS, 4 ) ? "" : check_string(LS, 4, MIL) ); 


    return 0;
}

static int CH_reward (lua_State *LS)
{
    if ( !is_CH( LS, 2 ) )
    {
        /* standard 'mob reward' syntax */
        do_mpreward( check_CH(LS, 1), check_fstring( LS, 2, MIL));
        return 0;
    }

    mpreward( check_CH(LS, 1), check_CH(LS, 2),
            check_string(LS, 3, MIL),
            (int)luaL_checknumber(LS, 4) );
    return 0;
}

static int CH_peace (lua_State *LS)
{
    if ( lua_isnone( LS, 2) )
        do_mppeace( check_CH(LS, 1), "");
    else
        do_mppeace( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_restore (lua_State *LS)
{
    do_mprestore( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_hit (lua_State *LS)
{
    if (lua_isnone( LS, 2 ) )
    {
        do_mphit( check_CH(LS,1), "" );
    }
    else
    {
        do_mphit( check_CH(LS, 1), check_fstring( LS, 2, MIL));
    }

    return 0;

}

static int CH_mdo (lua_State *LS)
{
    interpret( check_CH(LS, 1), check_fstring( LS, 2, MIL));

    return 0;
}

static int CH_tell (lua_State *LS)
{
    if (lua_isstring(LS, 2))
    {
        char buf[MIL];
        sprintf( buf,
                "'%s' %s",
                check_string(LS, 2, 25),
                check_fstring(LS, 3, MIL-30) );
        do_tell( check_CH(LS, 1), buf );
        return 0;
    }

    tell_char( check_CH( LS, 1),
            check_CH( LS, 2),
            check_fstring( LS, 3, MIL) );
    return 0;
}

static int CH_mobhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, (bool) get_mob_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) ); 
    else
        lua_pushboolean( LS,  (bool) (get_char_room( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objhere (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS,(bool) get_obj_vnum_room( ud_ch, r_atoi(ud_ch, argument) ) );
    else
        lua_pushboolean( LS,(bool) (get_obj_here( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_mobexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS,(bool) (get_mp_char( ud_ch, argument) != NULL) );

    return 1;
}

static int CH_objexists (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1); 
    const char *argument = check_fstring( LS, 2, MIL);

    lua_pushboolean( LS, (bool) (get_mp_obj( ud_ch, argument) != NULL) );

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

    lua_pushboolean( LS,  ud_ch != NULL
            &&  is_affected_parse(ud_ch, argument) );

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
        return set_flag( LS, "act[NPC]", act_flags, ud_ch->act );
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
        return set_flag( LS, "immune", imm_flags, ud_ch->imm_flags );
    }
    else
        return luaL_error( LS, "'setimmune' for NPC only.");

}

static int CH_carries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string( LS, 2, MIL);
    int count=0;

    if ( is_number( argument ) )
    {
        int vnum=atoi( argument );
        OBJ_DATA *obj;

        for ( obj=ud_ch->carrying ; obj ; obj=obj->next_content )
        {
            if ( obj->pIndexData->vnum == vnum )
            {
                count++;
            }
        }

        if (count<1)
        {
            lua_pushboolean(LS, FALSE);
            return 1;
        }
        else
        {
            lua_pushinteger(LS, count);
            return 1;
        } 
    }
    else
    {
        OBJ_DATA *obj;
        bool exact=FALSE;
        if (!lua_isnone(LS,3))
        {
            exact=lua_toboolean(LS,3);
        }

        for ( obj=ud_ch->carrying ; obj ; obj=obj->next_content )
        {
            if (    obj->wear_loc == WEAR_NONE 
                 && is_either_name( argument, obj->name, exact))
            {
                count++;
            }
        }

        if (count<1)
        {
            lua_pushboolean(LS, FALSE);
            return 1;
        }
        else
        {
            lua_pushinteger(LS, count);
            return 1;
        }
    }
}

static int CH_wears (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
        lua_pushboolean( LS, ud_ch != NULL && has_item( ud_ch, r_atoi(ud_ch, argument), -1, TRUE ) );
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

static int CH_describe (lua_State *LS)
{
    bool cleanUp = false;
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    OBJ_DATA * ud_obj;

    if (lua_isnumber(LS, 2))
    {
        int num = (int)luaL_checknumber (LS, 2);
        OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

        if (!pObjIndex)
        {
            luaL_error(LS, "No object with vnum: %d", num);
        }

        ud_obj = create_object(pObjIndex);
        cleanUp = true;
    }
    else
    {
        ud_obj = check_OBJ(LS, 2);
    }

    describe_item(ud_ch, ud_obj);

    if (cleanUp)
    {
        extract_obj(ud_obj);
    }

    return 0;
}

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

static int CH_oload (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    OBJ_INDEX_DATA *pObjIndex = get_obj_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No object with vnum: %d", num);

    OBJ_DATA *obj = create_object(pObjIndex);
    check_enchant_obj( obj );

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
        return set_flag( LS, "vuln", vuln_flags, ud_ch->vuln_flags );
    }
    else
        return luaL_error( LS, "'setvuln' for NPC only." );

}

static int CH_qstatus (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, quest_status( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
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
        return set_flag( LS, "resist", res_flags, ud_ch->res_flags );
    }
    else
        return luaL_error( LS, "'setresist' for NPC only.");
}

static int CH_skilled (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_string( LS, 2, MIL);
    bool prac=FALSE;
    if (!lua_isnone(LS, 3))
    {
        prac=lua_toboolean(LS, 3);
    }

    int sn=skill_lookup(argument);
    if (sn==-1)
        luaL_error(LS, "No such skill '%s'", argument);

    int skill=get_skill(ud_ch, sn);
    
    if (skill<1)
    {
        lua_pushboolean( LS, FALSE);
        return 1;
    }

    if (prac)
    {
        lua_pushinteger( LS, get_skill_prac( ud_ch, sn) );
        return 1;
    }

    lua_pushinteger(LS, skill);
    return 1;
}

static int CH_ccarries (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    const char *argument = check_fstring( LS, 2, MIL);

    if ( is_r_number( argument ) )
    {
        lua_pushboolean( LS, ud_ch != NULL && has_item_in_container( ud_ch, r_atoi(ud_ch, argument), "zzyzzxzzyxyx" ) );
    }
    else
    {
        lua_pushboolean( LS, ud_ch != NULL && has_item_in_container( ud_ch, -1, argument ) );
    }

    return 1;
}

static int CH_qtimer (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);

    if ( ud_ch != NULL )
        lua_pushnumber( LS, qset_timer( ud_ch, num ) );
    else
        lua_pushnumber( LS, 0);

    return 1;
}

static int CH_delay (lua_State *LS)
{
    return L_delay( LS );
}

static int CH_cancel (lua_State *LS)
{
    return L_cancel( LS );
}

static int CH_get_ac (lua_State *LS)
{
    lua_pushinteger( LS,
            GET_AC( check_CH( LS, 1 ) ) );
    return 1;
}

static int CH_get_acbase (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_CH( LS, 1 ))->armor );
    return 1;
}

static int CH_set_acbase (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set acbase on PCs.");

    int val=luaL_checkinteger( LS, 2 );

    if ( val < -10000 || val > 10000 )
    {
        return luaL_error( LS, "Value must be between -10000 and 10000." );
    }
    ud_ch->armor=val;

    return 0;
}

static int CH_set_acpcnt (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Can't set acpcnt on PCs.");

    /* analogous to mob_base_ac */
    ud_ch->armor = 100 + ( ud_ch->level * -6 ) * luaL_checkinteger( LS, 2 ) / 100;
    return 0;
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

static int CH_set_stopcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    int val=luaL_checkinteger( LS, 2);

    if ( val < 0 || val > 10 )
    {
        return luaL_error( LS, "Valid stopcount range is 0 to 10");
    }
    
    ud_ch->stop=val;

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

static int CH_get_damtype (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH( LS, 1);
    lua_pushstring( LS,
            flag_bit_name(damage_type, attack_table[ud_ch->dam_type].damage) );
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

static int CH_set_level (lua_State *LS)
{
    CHAR_DATA * ud_ch = check_CH (LS, 1);
    if (!IS_NPC(ud_ch))
        luaL_error(LS, "Cannot set level on PC.");

    int num = (int)luaL_checknumber (LS, 2);
    if ( num < 1 || num > 200 )
        luaL_error( LS, "Invalid level: %d, range is 1 to 200.", num);

    float hppcnt= (float)ud_ch->hit/ud_ch->max_hit;
    float mppcnt= (float)ud_ch->mana/ud_ch->max_mana;
    float mvpcnt= (float)ud_ch->move/ud_ch->max_move;

    set_mob_level( ud_ch, num );

    ud_ch->hit  = UMAX(1,hppcnt*ud_ch->max_hit);
    ud_ch->mana = UMAX(0,mppcnt*ud_ch->max_mana);
    ud_ch->move = UMAX(0,mvpcnt*ud_ch->max_move);
    return 0;
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
CHGETSTAT( vit, STAT_VIT );
CHGETSTAT( agi, STAT_AGI );
CHGETSTAT( dex, STAT_DEX );
CHGETSTAT( int, STAT_INT );
CHGETSTAT( wis, STAT_WIS );
CHGETSTAT( dis, STAT_DIS );
CHGETSTAT( cha, STAT_CHA );
CHGETSTAT( luc, STAT_LUC );

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
CHSETSTAT( vit, STAT_VIT );
CHSETSTAT( agi, STAT_AGI );
CHSETSTAT( dex, STAT_DEX );
CHSETSTAT( int, STAT_INT );
CHSETSTAT( wis, STAT_WIS );
CHSETSTAT( dis, STAT_DIS );
CHSETSTAT( cha, STAT_CHA );
CHSETSTAT( luc, STAT_LUC );

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

static int CH_set_race (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
#ifndef TESTER
    if (!IS_NPC(ud_ch))
        luaL_error( LS, "Can't set race on PCs.");
#endif    
    const char * arg=check_string(LS, 2, MIL);
    int race=race_lookup(arg);
    if (race==0)
        luaL_error(LS, "No such race: %s", arg );

#ifdef TESTER
    if ( !IS_NPC(ud_ch) )
    {
        if ( !race_table[race].pc_race )
            luaL_error(LS, "Not a valid player race: %s", arg);
        ud_ch->race=race;
        reset_char( ud_ch );
        morph_update( ud_ch );
        return 0;
    }
#endif
    set_mob_race( ud_ch, race );
    return 0;
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

static int CH_get_stopcount (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    lua_pushinteger(LS, ud_ch->stop);
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

static int CH_get_clanrank( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get clanrank on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->clan_rank);
    return 1;
}

static int CH_get_remorts( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get remorts on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->remorts);
    return 1;
}

static int CH_get_explored( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get explored on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->explored->set);
    return 1;
}

static int CH_get_beheads( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get beheads on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->behead_cnt);
    return 1;
}

static int CH_get_pkills( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get pkills on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->pkill_count);
    return 1;
}

static int CH_get_pkdeaths( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get pkdeaths on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->pkill_deaths);
    return 1;
}

static int CH_get_questpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get questpoints on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->questpoints);
    return 1;
}

static int CH_set_questpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't set questpoints on NPCs.");

    ud_ch->pcdata->questpoints=luaL_checkinteger(LS, 2);
    return 0;
}

static int CH_get_achpoints( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get achpoints on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->achpoints);
    return 1;
}

static int CH_get_bank( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get bank on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->bank);
    return 1;
}

static int CH_get_mobkills( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get mobkills on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->mob_kills);
    return 1;
}

static int CH_get_mobdeaths( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get mobdeaths on NPCs.");

    lua_pushinteger( LS,
            ud_ch->pcdata->mob_deaths);
    return 1;
}

static int CH_get_bossachvs( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (IS_NPC(ud_ch)) luaL_error(LS, "Can't get bossachvs on NPCs.");

    BOSSREC *rec;
    int index=1;
    lua_newtable(LS);

    for ( rec=ud_ch->pcdata->boss_achievements ; rec; rec=rec->next)
    {
        if (push_BOSSREC(LS, rec))
            lua_rawseti(LS, -2, index++);
    }

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

static int CH_get_ingame( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    if (!IS_NPC(ud_ch)) luaL_error(LS, "Can't get ingame on PCs.");

    lua_pushboolean( LS, is_mob_ingame( ud_ch->pIndexData ) );
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

static int CH_get_ptitles( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS, 1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'ptitles' for NPC.");
    }

    push_ref( LS, ud_ch->pcdata->ptitles );
    return 1;
}

static int CH_set_ptitles( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'ptitles' for NPC.");
    }

    /* probably should check type and format of table in the future */

    save_ref( LS, 2, &(ud_ch->pcdata->ptitles));
    return 0;
}

static int CH_get_ptitle( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't get 'ptitle' for NPC.");
    }

    lua_pushstring(LS, ud_ch->pcdata->pre_title);
    return 1;
}

static int CH_set_ptitle( lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);

    if (IS_NPC(ud_ch))
    {
        return luaL_error(LS, "Can't set 'ptitle' for NPC.");
    }

    const char *new=check_string( LS, 2, MIL);
    free_string(ud_ch->pcdata->pre_title);
    ud_ch->pcdata->pre_title=str_dup(new);
    return 0;
}

static int CH_get_stance (lua_State *LS)
{
    lua_pushstring( LS,
            stances[ (check_CH( LS, 1) )->stance ].name );
    return 1;
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

static int CH_set_pet (lua_State *LS)
{
    CHAR_DATA *ud_ch=check_CH(LS,1);
    CHAR_DATA *pet=check_CH(LS,2);

    if (IS_NPC(ud_ch))
        luaL_error(LS,
                "Can only add pets to PCs.");
    else if (!IS_NPC(pet))
        luaL_error(LS,
                "Can only add NPCs as pets.");
    else if (ud_ch->pet)
        luaL_error(LS,
                "%s already has a pet.", ud_ch->name);
    else if (IS_AFFECTED(pet, AFF_CHARM))
        luaL_error(LS,
                "%s is already charmed.", pet->name);

    SET_BIT(pet->act, ACT_PET);
    SET_BIT(pet->affect_field, AFF_CHARM);
    flag_clear( pet->penalty );
    SET_BIT( pet->penalty, PENALTY_NOTELL );
    SET_BIT( pet->penalty, PENALTY_NOSHOUT );
    SET_BIT( pet->penalty, PENALTY_NOCHANNEL );
    add_follower( pet, ud_ch );
    pet->leader = ud_ch;
    ud_ch->pet = pet;

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
    CHGET(vit, 0),
    CHGET(agi, 0),
    CHGET(dex, 0),
    CHGET(int, 0),
    CHGET(wis, 0),
    CHGET(dis, 0),
    CHGET(cha, 0),
    CHGET(ac, 0),
    CHGET(acbase, 0),
    CHGET(hitroll, 0),
    CHGET(hitrollbase, 0),
    CHGET(damroll, 0),
    CHGET(damrollbase, 0),
    CHGET(attacktype, 0),
    CHGET(damnoun, 0),
    CHGET(damtype, 0),
    CHGET(luc, 0),
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
    CHGET(stopcount, 0),
    CHGET(waitcount, 0),
    CHGET(heshe, 0),
    CHGET(himher, 0),
    CHGET(hisher, 0),
    CHGET(inventory, 0),
    CHGET(room, 0),
    CHGET(groupsize, 0),
    CHGET(stance, 0),
    CHGET(description, 0),
    CHGET(pet, 0),
    CHGET(master, 0),
    CHGET(leader, 0),
    CHGET(affects, 0),
    CHGET(scroll, 0),
    CHGET(id, 0 ),
    /* PC only */
    CHGET(clanrank, 0),
    CHGET(remorts, 0),
    CHGET(explored, 0),
    CHGET(beheads, 0),
    CHGET(pkills, 0),
    CHGET(pkdeaths, 0),
    CHGET(questpoints, 0),
    CHGET(achpoints, 0),
    CHGET(bank, 0),
    CHGET(mobkills, 0),
    CHGET(mobdeaths, 0),
    CHGET(descriptor, 0),
    CHGET(bossachvs, 0),
    CHGET(ptitle, 0),
    CHGET(ptitles, 0),
    /* NPC only */
    CHGET(vnum, 0),
    CHGET(proto,0),
    CHGET(ingame,0),
    CHGET(shortdescr, 0),
    CHGET(longdescr, 0),    
    ENDPTABLE
};

static const LUA_PROP_TYPE CH_set_table [] =
{
    CHSET(name, 5),
    CHSET(level, 5),
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
    CHSET(vit, 5),
    CHSET(agi, 5),
    CHSET(dex, 5),
    CHSET(int, 5),
    CHSET(wis, 5),
    CHSET(dis, 5),
    CHSET(cha, 5),
    CHSET(luc, 5),
    CHSET(stopcount, 5),
    CHSET(waitcount, 5),
    CHSET(acpcnt, 5),
    CHSET(acbase, 5),
    CHSET(hrpcnt, 5),
    CHSET(hitrollbase, 5),
    CHSET(drpcnt, 5),
    CHSET(damrollbase, 5),
    CHSET(attacktype, 5),
    CHSET(race, 5),
    CHSET(shortdescr, 5),
    CHSET(longdescr, 5),
    CHSET(description, 5),
    CHSET(pet, 5),
    /* PC only */
    CHSET(questpoints, SEC_NOSCRIPT),
    CHSET(ptitles, SEC_NOSCRIPT),
    CHSET(ptitle, SEC_NOSCRIPT),
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
    CHMETH(qstatus, 0),
    CHMETH(resist, 0),
    CHMETH(vuln, 0),
    CHMETH(skilled, 0),
    CHMETH(ccarries, 0),
    CHMETH(qtimer, 0),
    CHMETH(cansee, 0),
    CHMETH(canattack, 0),
    CHMETH(destroy, 1),
    CHMETH(oload, 1),
    CHMETH(say, 1),
    CHMETH(emote, 1),
    CHMETH(mdo, 1),
    CHMETH(tell, 1),
    CHMETH(asound, 1),
    CHMETH(zecho, 1),
    CHMETH(kill, 1),
    CHMETH(assist, 1),
    CHMETH(junk, 1),
    CHMETH(echo, 1),
    /* deprecated in favor of global funcs */
    { "echoaround", CH_echoaround, 1, STS_DEPRECATED},
    { "echoat", CH_echoat, 1, STS_DEPRECATED},
    CHMETH(mload, 1),
    CHMETH(purge, 1),
    CHMETH(goto, 1),
    CHMETH(at, 1),
    /* deprecated in favor of global funcs */
    { "transfer", CH_transfer, 1, STS_DEPRECATED},
    { "gtransfer", CH_gtransfer, 1, STS_DEPRECATED},
    CHMETH(otransfer, 1),
    CHMETH(force, 1),
    CHMETH(gforce, 1),
    CHMETH(vforce, 1),
    CHMETH(cast, 1),
    CHMETH(damage, 1),
    CHMETH(remove, 1),
    CHMETH(remort, 1),
    CHMETH(qset, 1),
    CHMETH(qadvance, 1),
    CHMETH(reward, 1),
    CHMETH(peace, 1),
    CHMETH(restore, 1),
    CHMETH(setact, 1),
    CHMETH(setvuln, 1),
    CHMETH(setimmune, 1),
    CHMETH(setresist, 1),
    CHMETH(hit, 1),
    CHMETH(randchar, 0),
    CHMETH(loadprog, 1),
    CHMETH(loadscript, 1),
    CHMETH(loadstring, 1),
    CHMETH(loadfunction, 1),
    CHMETH(savetbl, 1),
    CHMETH(loadtbl, 1),
    CHMETH(tprint, 1),
    CHMETH(olc, 1),
    CHMETH(delay, 1),
    CHMETH(cancel, 1), 
    CHMETH(setval, 1),
    CHMETH(getval, 1),
    CHMETH(rvnum, 0),
    CHMETH(describe, 1),
    CHMETH(addaffect, 9),
    CHMETH(removeaffect,9),
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

    return set_iflag( LS, "exit_flags", exit_flags, &ud_obj->value[1] );
}
    

static int OBJ_rvnum ( lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_remove(LS,1);

    return L_rvnum( LS, ud_obj->pIndexData->area );
}

static int OBJ_loadfunction (lua_State *LS)
{
    lua_obj_program( NULL, RUNDELAY_VNUM, NULL,
                check_OBJ(LS, -2), NULL,
                NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int OBJ_setval (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_remove( LS, 1 );
    return set_luaval( LS, &(ud_obj->luavals) );
}

static int OBJ_getval (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_remove( LS, 1);

    return get_luaval( LS, &(ud_obj->luavals) );
}

static int OBJ_delay (lua_State *LS)
{
    return L_delay(LS);
}

static int OBJ_cancel (lua_State *LS)
{
    return L_cancel(LS);
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
    bool copy_luavals=FALSE;

    if (!lua_isnone(LS,2))
    {
        copy_luavals=lua_toboolean(LS,2);
    }

    OBJ_DATA *clone = create_object(ud_obj->pIndexData);
    clone_object( ud_obj, clone );

    if (copy_luavals)
    {
        LUA_EXTRA_VAL *luaval;
        LUA_EXTRA_VAL *cloneval;
        for ( luaval=ud_obj->luavals; luaval ; luaval=luaval->next )
        {
            cloneval=new_luaval(
                    luaval->type,
                    str_dup( luaval->name ),
                    str_dup( luaval->val ),
                    luaval->persist);

            cloneval->next=clone->luavals;
            clone->luavals=cloneval;
        }
    }

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

    OBJ_DATA *obj = create_object(pObjIndex);
    check_enchant_obj( obj );
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

    return set_iflag( LS, "weapon_type2", weapon_type2, &ud_obj->value[4]);
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


static int OBJ_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_OBJ(LS,1))->clan].name);
    return 1;
}

static int OBJ_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJ(LS,1))->rank);
    return 1;
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

static int OBJ_get_ingame (lua_State *LS)
{
    OBJ_DATA *ud_obj=check_OBJ(LS,1);
    lua_pushboolean( LS, is_obj_ingame( ud_obj->pIndexData ) );
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

static int OBJ_set_attacktype ( lua_State *LS)
{
    OBJ_DATA *ud_obj = check_OBJ(LS, 1);
    const char *attack_arg = check_string(LS, 2, MIL);
    int attack; 

    if (ud_obj->item_type != ITEM_WEAPON )
        return luaL_error(LS, "attacktype for weapon only.");

    attack=attack_exact_lookup(attack_arg);
    if ( attack == -1 )
        return luaL_error(LS, "No such attack type '%s'",
                attack_arg );

    ud_obj->value[3]=attack;

    return 0;
}

static const LUA_PROP_TYPE OBJ_get_table [] =
{
    OBJGET(name, 0),
    OBJGET(shortdescr, 0),
    OBJGET(description, 0),
    OBJGET(clan, 0),
    OBJGET(clanrank, 0),
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
    OBJGET(ingame, 0),
    OBJGET(timer, 0),
    OBJGET(affects, 0),
    
    /*light*/
    OBJGET(light, 0),

    /*arrows*/
    OBJGET(arrowcount, 0),
    OBJGET(arrowdamage, 0),
    OBJGET(arrowdamtype, 0),
    
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
    OBJGET( damtype, 0),
    OBJGET( damnoun, 0),
    OBJGET( damavg, 0),

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
    OBJSET(attacktype, 5),
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
    OBJMETH(loadprog, 1),
    OBJMETH(loadscript, 1),
    OBJMETH(loadstring, 1),
    OBJMETH(loadfunction, 1),
    OBJMETH(oload, 1),
    OBJMETH(savetbl, 1),
    OBJMETH(loadtbl, 1),
    OBJMETH(tprint, 1),
    OBJMETH(delay, 1),
    OBJMETH(cancel, 1),
    OBJMETH(setval, 1),
    OBJMETH(getval, 1),
    OBJMETH(rvnum, 1),
    
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
static int AREA_rvnum ( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    lua_remove(LS,1);

    return L_rvnum( LS, ud_area );
}

static int AREA_loadfunction( lua_State *LS)
{
    lua_area_program( NULL, RUNDELAY_VNUM, NULL,
                check_AREA(LS, -2), NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int AREA_delay (lua_State *LS)
{
    return L_delay(LS);
}

static int AREA_cancel (lua_State *LS)
{
    return L_cancel(LS);
}

static int AREA_savetbl (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, SAVETABLE_FUNCTION);

    /* Push original args into SaveTable */
    lua_pushvalue( LS, 2 );
    lua_pushvalue( LS, 3 );
    lua_pushstring( LS, ud_area->file_name );
    lua_call( LS, 3, 0);

    return 0;
}

static int AREA_loadtbl (lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS,1);

    lua_getfield( LS, LUA_GLOBALSINDEX, LOADTABLE_FUNCTION);

    /* Push original args into LoadTable */
    lua_pushvalue( LS, 2 );
    lua_pushstring( LS, ud_area->file_name );
    lua_call( LS, 2, 1);

    return 1;
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

static int AREA_purge( lua_State *LS)
{
    purge_area( check_AREA(LS,1) );
    return 0;
}

static int AREA_echo( lua_State *LS)
{
    AREA_DATA *ud_area = check_AREA(LS, 1);
    const char *argument = check_fstring( LS, 2, MSL);
    DESCRIPTOR_DATA *d;

    for ( d = descriptor_list; d; d = d->next )
    {
        if ( IS_PLAYING(d->connected) )
        {
            if ( !d->character->in_room )
                continue;
            if ( d->character->in_room->area != ud_area )
                continue;

            if ( IS_IMMORTAL(d->character) )
                send_to_char( "Area echo> ", d->character );
            send_to_char( argument, d->character );
            send_to_char( "\n\r", d->character );
        }
    }

    return 0;
}

static int AREA_tprint ( lua_State *LS)
{
    lua_getfield( LS, LUA_GLOBALSINDEX, TPRINTSTR_FUNCTION);

    /* Push original arg into tprintstr */
    lua_pushvalue( LS, 2);
    lua_call( LS, 1, 1 );

    lua_pushcfunction( LS, AREA_echo );
    /* now line up arguments for echo */
    lua_pushvalue( LS, 1); /* area */
    lua_pushvalue( LS, -3); /* return from tprintstr */

    lua_call( LS, 2, 0);

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

static int AREA_get_minlevel( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->minlevel);
    return 1;
}

static int AREA_get_maxlevel( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->maxlevel);
    return 1;
}

static int AREA_get_security( lua_State *LS)
{
    lua_pushinteger( LS, (check_AREA(LS, 1))->security);
    return 1;
}

static int AREA_get_ingame( lua_State *LS)
{
    lua_pushboolean( LS, is_area_ingame(check_AREA(LS, 1)));
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
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_mprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_oprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_oprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_aprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_aprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_rprogs( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA(LS, 1);
    int index=1;
    int vnum=0;
    lua_newtable(LS);
    PROG_CODE *prog;
    for ( vnum = ud_area->min_vnum ; vnum <= ud_area->max_vnum ; vnum++ )
    {
        if ((prog=get_rprog_index(vnum)) != NULL )
        {
            if (push_PROG(LS, prog))
                lua_rawseti(LS, -2, index++);
        }
    }
    return 1;
}

static int AREA_get_atrigs ( lua_State *LS)
{
    AREA_DATA *ud_area=check_AREA( LS, 1);
    PROG_LIST *atrig;

    int index=1;
    lua_newtable( LS );

    for ( atrig = ud_area->aprogs ; atrig ; atrig = atrig->next )
    {
        if (push_ATRIG( LS, atrig) )
            lua_rawseti(LS, -2, index++);
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
    AREAGET(minlevel, 0),
    AREAGET(maxlevel, 0),
    AREAGET(security, 0),
    AREAGET(vnum, 0),
    AREAGET(minvnum, 0),
    AREAGET(maxvnum, 0),
    AREAGET(credits, 0),
    AREAGET(builders, 0),
    AREAGET(ingame, 0),
    AREAGET(rooms, 0),
    AREAGET(people, 0),
    AREAGET(players, 0),
    AREAGET(mobs, 0),
    AREAGET(mobprotos, 0),
    AREAGET(objprotos, 0),
    AREAGET(mprogs, 0),
    AREAGET(oprogs, 0),
    AREAGET(aprogs, 0),
    AREAGET(rprogs, 0),
    AREAGET(atrigs, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE AREA_method_table [] =
{
    AREAMETH(flag, 0),
    AREAMETH(echo, 1),
    AREAMETH(reset, 5),
    AREAMETH(purge, 5),
    AREAMETH(loadprog, 1),
    AREAMETH(loadscript, 1),
    AREAMETH(loadstring, 1),
    AREAMETH(loadfunction, 1),
    AREAMETH(savetbl, 1),
    AREAMETH(loadtbl, 1),
    AREAMETH(tprint, 1),
    AREAMETH(delay, 1),
    AREAMETH(cancel, 1),
    AREAMETH(rvnum, 1),
    ENDPTABLE
}; 

/* end AREA section */

/* ROOM section */
static int ROOM_rvnum ( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM(LS,1);
    lua_remove(LS,1);

    return L_rvnum( LS, ud_room->area );
}

static int ROOM_loadfunction ( lua_State *LS)
{
    lua_room_program( NULL, RUNDELAY_VNUM, NULL,
                check_ROOM(LS, -2), NULL,
                NULL, NULL, NULL, NULL,
                TRIG_CALL, 0 );
    return 0;
}

static int ROOM_mload (lua_State *LS)
{
    ROOM_INDEX_DATA * ud_room = check_ROOM (LS, 1);
    int num = (int)luaL_checknumber (LS, 2);
    MOB_INDEX_DATA *pObjIndex = get_mob_index( num );

    if (!pObjIndex)
        luaL_error(LS, "No mob with vnum: %d", num);

    CHAR_DATA *mob=create_mobile( pObjIndex);
    arm_npc( mob );
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

    OBJ_DATA *obj = create_object(pObjIndex);
    check_enchant_obj( obj );
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

static int ROOM_purge( lua_State *LS)
{
    purge_room( check_ROOM(LS,1) );
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

static int ROOM_delay (lua_State *LS)
{
    return L_delay(LS);
}

static int ROOM_cancel (lua_State *LS)
{
    return L_cancel(LS);
}

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

static int ROOM_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[check_ROOM(LS,1)->clan].name);
    return 1;
}

static int ROOM_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_ROOM(LS,1))->clan_rank);
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
ROOM_dir(northeast, DIR_NORTHEAST)
ROOM_dir(northwest, DIR_NORTHWEST)
ROOM_dir(southeast, DIR_SOUTHEAST)
ROOM_dir(southwest, DIR_SOUTHWEST)
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

static int ROOM_get_ingame( lua_State *LS )
{
    lua_pushboolean( LS,
            is_room_ingame( check_ROOM(LS,1) ) );
    return 1;
}

static int ROOM_get_rtrigs ( lua_State *LS)
{
    ROOM_INDEX_DATA *ud_room=check_ROOM( LS, 1);
    PROG_LIST *rtrig;

    int index=1;
    lua_newtable( LS );

    for ( rtrig = ud_room->rprogs ; rtrig ; rtrig = rtrig->next )
    {
        if (push_RTRIG( LS, rtrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
}

static const LUA_PROP_TYPE ROOM_get_table [] =
{
    ROOMGET(name, 0),
    ROOMGET(vnum, 0),
    ROOMGET(clan, 0),
    ROOMGET(clanrank, 0),
    ROOMGET(healrate, 0),
    ROOMGET(manarate, 0),
    ROOMGET(owner, 0),
    ROOMGET(description, 0),
    ROOMGET(ingame, 0),
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
    ROOMGET(northwest, 0),
    ROOMGET(northeast, 0),
    ROOMGET(southwest, 0),
    ROOMGET(southeast, 0),
    ROOMGET(up, 0),
    ROOMGET(down, 0),
    ROOMGET(resets, 0),
    ROOMGET(rtrigs, 0),
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
    ROOMMETH(purge, 5),
    ROOMMETH(oload, 1),
    ROOMMETH(mload, 1),
    ROOMMETH(echo, 1),
    ROOMMETH(loadprog, 1),
    ROOMMETH(loadscript, 1),
    ROOMMETH(loadstring, 1),
    ROOMMETH(loadfunction, 1),
    ROOMMETH(tprint, 1),
    ROOMMETH(delay, 1),
    ROOMMETH(cancel, 1),
    ROOMMETH(savetbl, 1),
    ROOMMETH(loadtbl, 1),
    ROOMMETH(rvnum, 1),
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
static int OBJPROTO_adjustdamage( lua_State *LS)
{

    OBJ_INDEX_DATA *ud_objp = check_OBJPROTO(LS, 1);
    if ( ud_objp->item_type != ITEM_WEAPON )
        luaL_error( LS, "adjustdamage for weapon only");

    lua_pushboolean( LS, adjust_weapon_dam( ud_objp ) );
    return 1;
}

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

static int OBJPROTO_get_clan (lua_State *LS)
{
    lua_pushstring( LS,
            clan_table[(check_OBJPROTO(LS,1))->clan].name);
    return 1;
}

static int OBJPROTO_get_clanrank (lua_State *LS)
{
    lua_pushinteger( LS,
            (check_OBJPROTO(LS,1))->rank);
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

static int OBJPROTO_get_ingame ( lua_State *LS )
{
    lua_pushboolean( LS,
            is_obj_ingame( check_OBJPROTO(LS,1) ) );
    return 1;
}

static int OBJPROTO_get_area ( lua_State *LS )
{
    if (push_AREA(LS, (check_OBJPROTO(LS,1))->area) )
        return 1;
    return 0;
}

static int OBJPROTO_get_otrigs ( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_oid=check_OBJPROTO( LS, 1);
    PROG_LIST *otrig;

    int index=1;
    lua_newtable( LS );

    for ( otrig = ud_oid->oprogs ; otrig ; otrig = otrig->next )
    {
        if (push_OTRIG( LS, otrig) )
            lua_rawseti(LS, -2, index++);
    }
    return 1;
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

static int OBJPROTO_get_rating ( lua_State *LS)
{
    OBJ_INDEX_DATA *ud_oid=check_OBJPROTO( LS, 1);

    lua_pushinteger( LS,
            ud_oid->diff_rating);

    return 1;
}

static const LUA_PROP_TYPE OBJPROTO_get_table [] =
{
    OPGET( name, 0),
    OPGET( shortdescr, 0),
    OPGET( description, 0),
    OPGET( clan, 0),
    OPGET( clanrank, 0),
    OPGET( level, 0),
    OPGET( cost, 0),
    OPGET( material, 0),
    OPGET( vnum, 0),
    OPGET( otype, 0),
    OPGET( weight, 0),
    OPGET( rating, 0),
    OPGET( v0, 0),
    OPGET( v1, 0),
    OPGET( v2, 0),
    OPGET( v3, 0),
    OPGET( v4, 0),
    OPGET( area, 0),
    OPGET( ingame, 0),
    OPGET( otrigs, 0),
    OPGET( affects, 0),

    /*light*/
    OPGET(light, 0),

    /*arrows*/
    OPGET(arrowcount, 0),
    OPGET(arrowdamage, 0),
    OPGET(arrowdamtype, 0),
    
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
    OPGET( damtype, 0),
    OPGET( damnoun, 0),
    OPGET( damavg, 0),

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
    OPMETH(adjustdamage, 9),
    
    /* container only */
    OPMETH(containerflag, 0),
    
    ENDPTABLE
}; 

/* end OBJPROTO section */

/* MOBPROTO section */
static int MOBPROTO_affected (lua_State *LS)
{
    MOB_INDEX_DATA *ud_mobp = check_MOBPROTO (LS, 1);
    return check_flag( LS, "affected", affect_flags, ud_mobp->affect_field );
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
MPGETINT( hppcnt, ud_mobp->hitpoint_percent,"" ,"");
MPGETINT( mnpcnt, ud_mobp->mana_percent,"" ,"");
MPGETINT( mvpcnt, ud_mobp->move_percent,"" ,"");
MPGETINT( hrpcnt, ud_mobp->hitroll_percent,"" ,"");
MPGETINT( drpcnt, ud_mobp->damage_percent,"" ,"");
MPGETINT( acpcnt, ud_mobp->ac_percent,"" ,"");
MPGETINT( savepcnt, ud_mobp->saves_percent,"" ,"");
MPGETSTR( damtype, attack_table[ud_mobp->dam_type].name,"" ,"");
MPGETSTR( startpos, flag_bit_name(position_flags, ud_mobp->start_pos),"" ,"");
MPGETSTR( defaultpos, flag_bit_name(position_flags, ud_mobp->default_pos),"" ,"");
MPGETSTR( sex,
    ud_mobp->sex == SEX_NEUTRAL ? "neutral" :
    ud_mobp->sex == SEX_MALE    ? "male" :
    ud_mobp->sex == SEX_FEMALE  ? "female" :
    ud_mobp->sex == SEX_BOTH    ? "random" :
    NULL,"" ,"");
MPGETSTR( race, race_table[ud_mobp->race].name,"" ,"");
MPGETINT( wealthpcnt, ud_mobp->wealth_percent,"" ,"");
MPGETSTR( size, flag_bit_name(size_flags, ud_mobp->size),"" ,"");
MPGETSTR( stance, stances[ud_mobp->stance].name,
    "Mob's default stance." ,
    "See 'stances' table.");
MPGETBOOL( ingame, is_mob_ingame( ud_mobp ),"" ,"");
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
    PROG_LIST *mtrig;
    
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

static int MOBPROTO_get_bossachv ( lua_State *LS)
{
    MOB_INDEX_DATA *ud_mid=check_MOBPROTO( LS, 1);
    if ( ud_mid->boss_achieve )
    {
        if ( push_BOSSACHV(LS, ud_mid->boss_achieve) )
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
    MPGET( hppcnt, 0),
    MPGET( mnpcnt, 0),
    MPGET( mvpcnt, 0),
    MPGET( hrpcnt, 0),
    MPGET( drpcnt, 0),
    MPGET( acpcnt, 0),
    MPGET( savepcnt, 0),
    MPGET( damtype, 0),
    MPGET( startpos, 0),
    MPGET( defaultpos, 0),
    MPGET( sex, 0),
    MPGET( race, 0),
    MPGET( wealthpcnt, 0),
    MPGET( size, 0),
    MPGET( stance, 0),
    MPGET( area, 0),
    MPGET( ingame, 0),
    MPGET( mtrigs, 0),
    MPGET( shop, 0),
    MPGET( bossachv, 0),
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

static int AFFECT_get_detectlevel ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    lua_pushinteger( LS,
            ud_af->detect_level);
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
            /* tbc, make it return table of flags */
            lua_pushstring(LS,
                    i_flag_bits_name( weapon_type2, ud_af->bitvector ) );
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
        case TO_SPECIAL:
            lua_pushinteger( LS, ud_af->bitvector );
            break;
        default:
            luaL_error( LS, "Invalid where." );
    }
    return 1;
}

static int AFFECT_get_tag ( lua_State *LS )
{
    AFFECT_DATA *ud_af=check_AFFECT(LS,1);

    if (ud_af->type != gsn_custom_affect)
        luaL_error(LS, "Can only get tag for custom_affect type.");

    lua_pushstring( LS, ud_af->tag);
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
    AFFGET( detectlevel, 0),
    AFFGET( tag, 0),
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

/* PROG section */
static int PROG_get_islua ( lua_State *LS )
{
    lua_pushboolean( LS,
            (check_PROG( LS, 1) )->is_lua);
    return 1;
}

static int PROG_get_vnum ( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_PROG( LS, 1) )->vnum);
    return 1;
}

static int PROG_get_code ( lua_State *LS )
{
    lua_pushstring( LS,
            (check_PROG( LS, 1) )->code);
    return 1;
}

static int PROG_get_security ( lua_State *LS )
{
    lua_pushinteger( LS,
            (check_PROG( LS, 1) )->security);
    return 1;
}

static const LUA_PROP_TYPE PROG_get_table [] =
{
    PROGGET( islua, 0),
    PROGGET( vnum, 0),
    PROGGET( code, 0),
    PROGGET( security, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE PROG_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE PROG_method_table [] =
{
    ENDPTABLE
};
/* end PROG section */

/* TRIG section */
static int TRIG_get_trigtype ( lua_State *LS )
{
    const struct flag_type *tbl;

    lua_getmetatable(LS,1);
    lua_getfield(LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );

    if (type==&MTRIG_type)
    {
        tbl=mprog_flags;
    }
    else if (type==&OTRIG_type)
    {
        tbl=oprog_flags;
    }
    else if (type==&ATRIG_type)
    {
        tbl=aprog_flags;
    }
    else if (type==&RTRIG_type)
    {
        tbl=rprog_flags;
    }
    else
    {
        return luaL_error( LS, "Invalid type: %s.", type->type_name );
    }

    lua_pushstring( LS,
            flag_bit_name(
                tbl,
                ((PROG_LIST *) type->check( LS, 1 ) )->trig_type ) );
    return 1;
}

static int TRIG_get_trigphrase ( lua_State *LS )
{
    lua_getmetatable(LS,1);
    lua_getfield(LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );

    if ( type != &MTRIG_type
            && type != &OTRIG_type
            && type != &ATRIG_type
            && type != &RTRIG_type )
        luaL_error( LS,
                "Invalid type: %s.",
                type->type_name);

    lua_pushstring( LS,
            ((PROG_LIST *) type->check( LS, 1 ) )->trig_phrase);
    return 1;
}

static int TRIG_get_prog ( lua_State *LS )
{
    lua_getmetatable(LS,1);
    lua_getfield(LS, -1, "TYPE");
    LUA_OBJ_TYPE *type=lua_touserdata( LS, -1 );

    if ( type != &MTRIG_type
            && type != &OTRIG_type
            && type != &ATRIG_type
            && type != &RTRIG_type )
        luaL_error( LS,
                "Invalid type: %s.",
                type->type_name);

    if ( push_PROG( LS,
            ((PROG_LIST *)type->check( LS, 1 ) )->script ) )
        return 1;
    return 0;
}    


static const LUA_PROP_TYPE TRIG_get_table [] =
{
    TRIGGET( trigtype, 0),
    TRIGGET( trigphrase, 0),
    TRIGGET( prog, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE TRIG_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE TRIG_method_table [] =
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

static int HELP_get_delete( lua_State *LS )
{
    lua_pushboolean( LS, check_HELP( LS, 1 )->delete );
    return 1;
}

static const LUA_PROP_TYPE HELP_get_table [] =
{
    GETP( HELP, level, 0 ),
    GETP( HELP, keywords, 0 ),
    GETP( HELP, text, 0 ),
    GETP( HELP, delete, 0 ),
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

static int DESCRIPTOR_get_constate( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);

    int state=con_state(ud_d);

    const char *name = name_lookup( state, con_states );

    if (!name)
    {
        bugf( "Unrecognized con state: %d", state );
        lua_pushstring( LS, "ERROR" );
    }
    else
    {
        lua_pushstring( LS, name );
    }
    return 1;
}

static int DESCRIPTOR_set_constate( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);
    const char *name=check_string(LS, 2, MIL);

    int state=flag_lookup( name, con_states );

    if ( state == -1 )
        return luaL_error( LS, "No such constate: %s", name );

    if (!is_settable(state, con_states))
        return luaL_error( LS, "constate cannot be set to %s", name );

    set_con_state( ud_d, state );
    return 0;
}

static int DESCRIPTOR_get_inbuf( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);
    lua_pushstring( LS, ud_d->inbuf);
    return 1;
}

static int DESCRIPTOR_set_inbuf( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);
    const char *arg=check_string( LS, 2, MAX_PROTOCOL_BUFFER );

    strcpy( ud_d->inbuf, arg );
    return 0;
}

static int DESCRIPTOR_get_conhandler( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);

    if (!is_set_ref(ud_d->conhandler))
        return 0;

    push_ref( LS, ud_d->conhandler); 
    return 1;
}

static int DESCRIPTOR_set_conhandler( lua_State *LS )
{
    DESCRIPTOR_DATA *ud_d=check_DESCRIPTOR( LS, 1);

    if (is_set_ref(ud_d->conhandler))
        release_ref( LS, &ud_d->conhandler);

    save_ref( LS, 2, &ud_d->conhandler);
    return 0;
}

static const LUA_PROP_TYPE DESCRIPTOR_get_table [] =
{
    GETP( DESCRIPTOR, character, 0 ),
    GETP( DESCRIPTOR, constate, SEC_NOSCRIPT ),
    GETP( DESCRIPTOR, inbuf, SEC_NOSCRIPT ),
    GETP( DESCRIPTOR, conhandler, SEC_NOSCRIPT ),
    ENDPTABLE
};

static const LUA_PROP_TYPE DESCRIPTOR_set_table [] =
{
    SETP( DESCRIPTOR, constate, SEC_NOSCRIPT),
    SETP( DESCRIPTOR, inbuf, SEC_NOSCRIPT),
    SETP( DESCRIPTOR, conhandler, SEC_NOSCRIPT),
    ENDPTABLE
};

static const LUA_PROP_TYPE DESCRIPTOR_method_table [] =
{
    ENDPTABLE
};
/* end DESCRIPTOR section */

/* BOSSACHV section */
static int BOSSACHV_get_qp( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->quest_reward);
    return 1;
}

static int BOSSACHV_get_exp( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->exp_reward);
    return 1;
}
static int BOSSACHV_get_gold( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->gold_reward);
    return 1;
}
static int BOSSACHV_get_achp( lua_State *LS )
{
    lua_pushinteger( LS,
            check_BOSSACHV( LS, 1 )->ach_reward);
    return 1;
}

static const LUA_PROP_TYPE BOSSACHV_get_table [] =
{
    GETP( BOSSACHV, qp, SEC_NOSCRIPT ),
    GETP( BOSSACHV, exp, SEC_NOSCRIPT ),
    GETP( BOSSACHV, gold, SEC_NOSCRIPT ),
    GETP( BOSSACHV, achp, SEC_NOSCRIPT ), 
    ENDPTABLE
};

static const LUA_PROP_TYPE BOSSACHV_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE BOSSACHV_method_table [] =
{
    ENDPTABLE
};
/* end BOSSACHV section */

/* BOSSREC section */

static int BOSSREC_get_vnum( lua_State *LS )
{
    lua_pushinteger( LS, check_BOSSREC(LS,1)->vnum);
    return 1;
} 

static int BOSSREC_get_timestamp( lua_State *LS )
{
    lua_pushinteger( LS, check_BOSSREC(LS,1)->timestamp);
    return 1;
}

static const LUA_PROP_TYPE BOSSREC_get_table [] =
{
    GETP( BOSSREC, vnum, 0),
    GETP( BOSSREC, timestamp, 0),
    ENDPTABLE
};

static const LUA_PROP_TYPE BOSSREC_set_table [] =
{
    ENDPTABLE
};

static const LUA_PROP_TYPE BOSSREC_method_table [] =
{
    ENDPTABLE
};
/* end BOSSREC section */

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
#define DECLARETRIG( LTYPE, CTYPE ) declb( LTYPE, CTYPE, TRIG ) 

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
DECLARETYPE( PROG, PROG_CODE );

DECLARETRIG( MTRIG, PROG_LIST );
DECLARETRIG( OTRIG, PROG_LIST );
DECLARETRIG( ATRIG, PROG_LIST );
DECLARETRIG( RTRIG, PROG_LIST );

DECLARETYPE( HELP, HELP_DATA );
DECLARETYPE( DESCRIPTOR, DESCRIPTOR_DATA );
DECLARETYPE( BOSSACHV, BOSSACHV );
DECLARETYPE( BOSSREC, BOSSREC );
