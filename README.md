The QuickMUD + Lua project is an effort create a QuickMUD variant with support for Lua scripting.
The Lua interface is based on the implementation on Aarchon MUD.

# Credits/License #
See other README files and docs folders for QuickMUD, Rom, Merc, Diku credits and license details.

The Lua interface code in this project is based primarily on the implementation by Vodur at Aarchon MUD, which implementation is partially based on Nick Gammon's code for embedding Lua in SMAUG codebase (see http://www.gammon.com.au/files/smaug/lua/, http://gammon.com.au/forum/?id=8000).

The QuickMUD + Lua project doesn't impose any additional license restrictions to those inherited from QuickMUD, Rom, Merc, and Diku.

# Disclaimers

* This code is not guaranteed to work cross platform and has only been tested thoroughly on Linux.


# Infrastructure changes #
Most core game objects are now allocated through Lua instead of malloc.

# Lua mprogs #
Now mprogs can be Lua scripts. The same 'mpedit' command is used to create and edit Lua mprogs. The mprog editor also conveniently has Lua syntax highlighting support for Lua scripts.

## Lua script errors ##
Turn on 'wiznet luaerror' to see script error details when they occur. 

## Security ##
Each Lua script has a security value setting. This determines which API functionality this script can call. The security levels for API members can be seen using the 'luahelp' command. If a script tries to access an API member with a security level higher than the script, an error will occur:

    :::text
    LUA mprog error for priest cleric(3719), mprog 3700:
     [string "return function (ch,obj1,obj2,trigger,text1..."]:1: Current security 1. Function requires 9.
    stack traceback:
            [C]: in function 'getarealist'
            [string "return function (ch,obj1,obj2,trigger,text1..."]:1: in function <[string "return function (ch,obj1,obj2,trigger,text1..."]:1>

## Script environments ##
When a Lua mprog runs, it runs in a script environment tied to the mob that the mprog is attached to. This script environment represents the global scope for any mprog that the mob will run. This means any global values that are assigned during a script will persist and be accessible by other scripts that the same mob instance may run.

Consider this prog:
```
#!lua
if not count then
  count = 1
else
  count = count + 1
end
say("Count: "..count)
```
'count' is treated as a global variable here and so will persist in this mob's script environment. If we assign this script to a GRALL trigger, then we will see the mob say a higher number each time somebody enters the room.

## Script Arguments ##
When an mprog runs, certain arguments are passed into the Lua script that is run. These arguments are:

* ch
* obj1
* obj2
* trigger 
* text1
* text2
* victim
* trigtype

'trigtype' is always the type of trigger that has caused the script to run ('grall' for instance). The other arguments will have different meaning depending on what type of trigger has caused the script to run.

The global variables 'mob' and 'self' are also accessible to the script, which both reference to the mob that is running the script.

## Accessing the game API ##
The 'mob'/'self' variable points to a CH type object. CH has certain properties and methods that can be accessed from a script (as long as the security is sufficient). A list of properties and methods for CH can be seen with 'luahelp CH'.

Reading a property value is as simple as indexing the CH with the property name:
```
#!lua
-- Both notations do the exact same thing
say(mob.hp)
say(mob['hp'])
```

Some properties are settable. Setting a property value is as simple as assigning a value:
```
#!lua
-- Both notations do the exact same thing
mob.hp = 100
mob['hp'] = 100
```

Methods are invoked using the ':' operator. However, through some special magic, methods of the mob that are running the script do not have to be qualified with a CH reference.
```
#!lua
-- These all do the exact same thing
mob:say("Hello!")
self:say("Hello!")
say("Hello!")

-- But if want to use methods on any other CH type object besides the mob running the script, 
-- we will always have to specify the actual object
a_new_mob = mob.room:mload(700)
a_new_mob:say("I'm a new mob")
```

A list of global functions and be seen with 'luahelp global'. These can be called directly from scripts as long as the security is sufficient.

Examples:
```
#!lua
-- Load mob 700 in 100 random rooms
for i=1,100 do
  local room = getrandomroom()
  local new_mob = room:mload(700)
  new_mob:say("Here I am!")
end
```
```
#!lua
-- Find all instances of mob 700 and make them dance
for _,mobby in pairs(getmobworld(700)) do
  mobby:mdo("dance")
end
```
```
#!lua
-- Send a fun message to all players
for _,player in pairs(getplayerlist()) do
  sendtochar(player, "What if hippos lived in Antarctica?\r\n")
end
```

## Inifinite loop protection ##
Scripts are prevented from going into infinite loops by limiting execution to a maximum of 1000000 instructions per script.


# New commands #
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

    :::text
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

**luaquery**

```
luaquery <selection> from <type> [where <filter>] [order by <sort>] [width <width>] [limit <limit>]
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