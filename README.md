# README #

This project is an effort create a QuickMUD variant with support for Lua scripting.
The Lua implementation is based on implementation on Aarchon MUD.

## Lua progs ##
Mprogs 


## New commands ##
**luai**
Enter interactive Lua shell.

**luahelp**
Show information on the game's Lua API.

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
[Hit Return to continue]

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


**luareset**