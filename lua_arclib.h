#ifndef LUA_ARCLIB_H
#define LUA_ARCLIB_H

struct lua_prop_type;
typedef struct lua_obj_type
{
    const char *type_name;

    bool (*valid)();
    void *(*check)();
    bool (*is)();
    bool (*push)();
    void *(*alloc)();
    void (*free)();

    int (*index)();
    int (*newindex)();

    void (*reg)();

    const struct lua_prop_type * const get_table;
    const struct lua_prop_type * const set_table;
    const struct lua_prop_type * const method_table;

    int count;
} LUA_OBJ_TYPE;

typedef struct lua_extra_val
{
    struct lua_extra_val *next;
    const char *name;

    int type;

    const char *val;
    bool persist;

} LUA_EXTRA_VAL;

void type_init();

extern LUA_OBJ_TYPE CH_type;
extern LUA_OBJ_TYPE OBJ_type;
extern LUA_OBJ_TYPE AREA_type;
extern LUA_OBJ_TYPE ROOM_type;
extern LUA_OBJ_TYPE EXIT_type;
extern LUA_OBJ_TYPE RESET_type;
extern LUA_OBJ_TYPE OBJPROTO_type;
extern LUA_OBJ_TYPE MOBPROTO_type;
extern LUA_OBJ_TYPE SHOP_type;
extern LUA_OBJ_TYPE PROG_type;
extern LUA_OBJ_TYPE MTRIG_type;
extern LUA_OBJ_TYPE OTRIG_type;
extern LUA_OBJ_TYPE ATRIG_type;
extern LUA_OBJ_TYPE RTRIG_type;
extern LUA_OBJ_TYPE AFFECT_type;
extern LUA_OBJ_TYPE HELP_type;
extern LUA_OBJ_TYPE DESCRIPTOR_type;
extern LUA_OBJ_TYPE BOSSACHV_type;
extern LUA_OBJ_TYPE BOSSREC_type;

void register_globals( lua_State *LS );
LUA_EXTRA_VAL *new_luaval( int type, const char *name, const char *val, bool persist );
void free_luaval( LUA_EXTRA_VAL *luaval );
void cleanup_uds();

/* moved to merc.h cause what if a file calls 
   valid_CH without including lua_arclib.h?
   It assumes int and doesn't work right.
   */
/*
#define declf( ltype, ctype ) \
ctype * check_ ## ltype ( lua_State *LS, int index ); \
bool    is_ ## ltype ( lua_State *LS, int index ); \
bool    push_ ## ltype ( lua_State *LS, ctype *ud );\
ctype * alloc_ ## ltype (void) ;\
void    free_ ## ltype ( ctype * ud );\
bool valid_ ## ltype ( ctype *ud );

declf(CH, CHAR_DATA)
declf(OBJ, OBJ_DATA)
declf(AREA, AREA_DATA)
declf(ROOM, ROOM_INDEX_DATA)
declf(EXIT, EXIT_DATA)
declf(RESET, RESET_DATA)
declf(MOBPROTO, MOB_INDEX_DATA)
declf(OBJPROTO, OBJ_INDEX_DATA)
declf(PROG, PROG_CODE)
declf(MTRIG, PROG_LIST)
declf(OTRIG, PROG_LIST)
declf(ATRIG, PROG_LIST)
declf(RTRIG, PROG_LIST)
declf(SHOP, SHOP_DATA)
declf(AFFECT, AFFECT_DATA)
declf(HELP, HELP_DATA)
declf(DESCRIPTOR, DESCRIPTOR_DATA)
*/
#undef declf

#endif
