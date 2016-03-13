# README #

This project is an effort create a QuickMUD variant with support for Lua scripting.
The Lua implementation is based on implementation on Aarchon MUD.

## Lua progs ##
Mprogs 


## New commands ##
**luai**

Enter interactive Lua shell.
```
luai

Entered lua interpreter mode for CH Vodur
Use @ on a blank line to exit.
Use do and end to create multiline chunks.
Use 'mob' or 'self' to access target's self.
lua> mob:say("Hi")
You say 'Hi'
lua> 
```



**luahelp**

Show information on the game's Lua API.

```
SECTIONS: 

global
CH
OBJ
AREA
ROOM
EXIT
RESET
OBJPROTO
MOBPROTO
SHOP
AFFECT
MPROG
MTRIG
HELP
DESCRIPTOR
Syntax: 
    luahelp <section>
    luahelp <section> <get|set|meth>
    luahelp dump <section> -- Dump for pasting to dokuwiki

Examples:
    luahelp ch
    luahelp global
    luahelp objproto meth
```


**luaquery**

```
luaquery [selection] from [type] <where [filter]> <order by [sort]> <width [width]> <limit [limit]>
    Execute a query and show results.

Types:
    area            - AREAs (area_list)
    objproto or op  - OBJPROTOs
    objs            - OBJs (object_list, live objects)
    mobproto or mp  - MOBPROTOs
    mobs            - CHs (all mobs from char_list)
    room            - ROOMs
    exit            - EXITs (includes 'dir', 'area', and 'room')
    reset           - RESETs (includes 'area' and 'room')
    help            - HELPs
    mprog           - MPROGs (all mprogs, includes 'area')
    mtrig           - MTRIGs (all mprog triggers, includes 'area' and 'mobproto')


Selection:
    Determines which fields are shown on output. If * or default then default
    values are used, otherwise fields supplied in a list separated by ',' 
    character.
    Additionally, the 'x as y' syntax can be used to define an alias. For instance: name:sub(1,10) as shortname,#spells as spellcount.

Filter (optional):
    Expression used to filter which results are shown. Argument is a statement 
    that can be evaluated to a boolean result. 'x' can be used optionally to
    qualify referenced fields. It is necessary to use 'x' when invoking methods.
    Using 'thisarea' in the filter produces results from current area only.

Sort (optional):
    One or more values determining the sort order of the output. Format is same
    as Selection, except aliases cannot be declared (but can be referenced).

Width (optional):
    An integer value which limits the width of the output columns to the given
    number of characters.

Limit (optional):
    An integer value which limits the number of results printed. If the value
    is positive then top results are printed. If the value is negative then
    bottom results are printed.

Notes: 
    'x' can be used optionally to qualify fields and methods. See examples.

    A field must be in the selection in order to be used in sort.

    For types listed above with "includes" in the description, these are variables
    included in the query (available for selection/filter/sort) that are not
    fields of the specified object type by default. For instance, MPROG type does
    not have a field called 'area' but for the purposes of MPROG queries it is
    accessible.

Examples:
    luaq level,vnum,shortdescr,extra("glow") as glow,area.name from op where otype=="weapon" and x:weaponflag("flaming") order by level,glow width 20

    Shows level, vnum, shortdescr, glow flag (true/false), and area.name for all
    OBJPROTOs that are weapons with flaming wflag. Sorted by level then by glow
    flag, with each column limited to 20 characters width.

    luaq * from mtrig where mobproto.level==90 width 15
    Shows default selection for all mprog trigs that are attached to mobs of level 90.
    Results are unsorted but columns are limited to 15 characters.

    luaq * from reset where command=="M" and arg1==10256
    Show default selection for all resets of mob vnum 10256.

```

```
luaq * from objproto where name:find("blue") order by level

Query: * from objproto where name:find("blue") order by level
|vnum  |lev|shortdescr                     |
|2370  |25 |a pair of blue steel greaves   |
|2369  |25 |a blue steel helm and visor    |
|2371  |25 |a pair of blue steel vambraces |
|7204  |17 |an neon blue potion            |
|924   |17 |a blue robe                    |
|1366  |11 |a blue potion                  |
|6309  |10 |a blue potion                  |
|5110  |9  |a silvery blue wand            |
|1363  |8  |a scroll written on blue paper |
|5231  |5  |a scarlet and blue stone       |
|5232  |5  |an incandescent blue stone     |
|5230  |5  |a pale blue stone              |
|10439 |4  |a figurine of a blue dragon    |
|9570  |1  |a glowing blue vial            |
|10422 |0  |the blueprint                  |
|3014  |0  |a blueberry danish             |
|9567  |0  |a glass of blue oasis          |
|9574  |0  |a dark blue potion             |
|1616  |0  |a dark blue cloak              |
Results 1 through 19 (of 19).

```



**luareset**

Reload specific Lua files.

```
Syntax:
    luareset <file>

Valid args:
commands
```