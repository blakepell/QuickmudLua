-- luareset section
local reset_table =
{
--    "interp",
    "commands",
--    "arclib",
--    "scripting",
--    "utilities",
    "leaderboard",
    "changelog"
}

local function luareset_usage( ch )
    sendtochar( ch,
[[
Syntax:
    luareset <file>

Valid args:
]])
    for _,arg in pairs(reset_table) do
        sendtochar( ch, arg.."\n\r")
    end
end

function do_luareset(ch, argument)
    local found=false
    for _,arg in pairs(reset_table) do
        if arg == argument then
            found=true
        end     
    end
    if not found then
        luareset_usage( ch )
        return
    end

    sendtochar(ch, "Loading "..argument..".lua\n\r")
    local f,err=loadfile( mud.luadir() .. argument..".lua")
    if not(f) then
        error(err)
    end
    sendtochar(ch, "Loaded.\n\r")
    sendtochar(ch, "Running "..argument..".lua\n\r")
    f()
    sendtochar(ch, argument..".lua reloaded successfully.\n\r")
end
-- end luareset section

-- luaquery section
local function luaquery_usage( ch )
    pagetochar( ch,
[[
{Cluaquery [selection] from [type] <where [filter]> <order by [sort]> <width [width]> <limit [limit]>
    {xExecute a query and show results.

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

    mprog   - PROGs (all mprogs, includes 'area')
    oprog   - PROGS (all oprogs, includes 'area')
    aprog   - PROGs (all aprogs, includes 'area')
    rprog   - PROGs (all rprogs, includes 'area')

    mtrig   - MTRIGs (all mprog triggers, includes 'area' and 'mobproto')
    otrig   - OTRIGs (all oprog triggers, includes 'area' and 'objproto')
    atrig   - ATRIGs (all aprog triggers, includes 'area')
    rtrig   - RTRIGs (all rprog triggers, includes 'area' and 'room')


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
    fields of the specified object type by default. For instance, PROG type does
    not have a field called 'area' but for the purposes of PROG queries it is
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

]])

end

local lqtbl={

    area={ 
        getfun=getarealist,
        default_sel="name"
    },

    op={
        getfun=function()
            local ops={}
            for _,area in pairs(getarealist()) do
                for _,op in pairs(area.objprotos) do
                    table.insert(ops, op)
                end
            end
            return ops
        end ,
        default_sel={"vnum","level","shortdescr"}
    },

    mp={
        getfun=function()
            local mps={}
            for _,area in pairs(getarealist()) do
                for _,mp in pairs(area.mobprotos) do
                    table.insert(mps, mp)
                end
            end
            return mps
        end,
        default_sel={"vnum","level","shortdescr"}
    },

    mobs={
        getfun=getmoblist,
        default_sel={"vnum","level","shortdescr"}
    },

    objs={
        getfun=getobjlist,
        default_sel={"vnum","level","shortdescr"}
    },

    room={
        getfun=function()
            local rooms={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    table.insert( rooms, room )
                end
            end
            return rooms
        end,
        default_sel={"area.name","vnum","name"}
    },


    reset={
        getfun=function()
            local resets={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    for _,reset in pairs(room.resets) do
                        table.insert( resets,
                                setmetatable( { ["area"]=area, ["room"]=room },
                                              {__index=reset} ) )
                    end
                end
            end
            return resets
        end,
        default_sel={
            "area.name",
            "room.name",
            "room.vnum",
            "command",
            "arg1","arg2","arg3","arg4"
        }
    },

    exit={
        getfun=function()
            local exits={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    for _,exit in pairs(room.exits) do
                        table.insert( exits, 
                            setmetatable( { ["area"]=area, 
                                            ["room"]=room,
                                            ["dir"]=exit  },
                                          { __index=room[exit]} ) )
                    end
                end
            end
            return exits
        end,
        default_sel={"room.vnum","dir","toroom.vnum"}
    },
    mprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.mprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel={"vnum"}
    },

    oprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.oprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel={"vnum"}
    },

    aprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.aprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel={"vnum"}
    },

    rprog={
        getfun=function()
            local progs={}
            for _,area in pairs(getarealist()) do
                for _,prog in pairs(area.rprogs) do
                    table.insert( progs,
                            setmetatable( { ["area"]=area}, {__index=prog}) )
                end
            end
            return progs
        end,
        default_sel={"vnum"}
    },

    mtrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,mp in pairs(area.mobprotos) do
                    for _,trig in pairs(mp.mtrigs) do
                        table.insert( trigs,
                                setmetatable( { ["area"]=area, ["mobproto"]=mp },
                                    {__index=trig}) )
                    end
                end
            end
            return trigs
        end,
        default_sel={
            "mobproto.vnum",
            "mobproto.shortdescr",
            "trigtype",
            "trigphrase",
            "prog.vnum"
        }
    },
    otrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,op in pairs(area.objprotos) do
                    for _,trig in pairs(op.otrigs) do
                        table.insert( trigs,
                                setmetatable( { ["area"]=area, ["objproto"]=op },
                                    {__index=trig}) )
                    end
                end
            end
            return trigs
        end,
        default_sel={
            "objproto.vnum",
            "objproto.shortdescr",
            "trigtype",
            "trigphrase",
            "prog.vnum"
        }
    },
   
    atrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,trig in pairs(area.atrigs) do
                    table.insert( trigs,
                            setmetatable( { ["area"]=area },
                                {__index=trig}) )
                end
            end
            return trigs
        end,
        default_sel={
            "area.name",
            "trigtype",
            "trigphrase",
            "prog.vnum"
        }
    },
    
    rtrig={
        getfun=function()
            local trigs={}
            for _,area in pairs(getarealist()) do
                for _,room in pairs(area.rooms) do
                    for _,trig in pairs(room.rtrigs) do
                        table.insert( trigs,
                                setmetatable( { ["area"]=area, ["room"]=room },
                                    {__index=trig}) )
                    end
                end
            end
            return trigs
        end,
        default_sel={
            "room.vnum",
            "room.name",
            "trigtype",
            "trigphrase",
            "prog.vnum"
        }
    },

    help={
        getfun=gethelplist,
        default_sel={"level","keywords"}
    }


}
lqtbl.objproto=lqtbl.op
lqtbl.mobproto=lqtbl.mp

local function luaq_results_con( ch, argument, header, result )
    local ind=1
    local scroll=(ch.scroll == 0 ) and 100 or ch.scroll
    local total=#result

    sendtochar(ch, "Query: "..argument.."\n\r")
    while true do
        local toind=math.min(ind+scroll-3, total ) -- -2 is normal, -3 for extra line at bottom, -4 for query at top
        local out={}
        table.insert(out, header)
        for i=ind,toind do
            table.insert(out, result[i] )
        end

        table.insert( out, ("Results %d through %d (of %d)."):format( ind, toind, total) )

        sendtochar( ch, table.concat( out, "\n\r").."\n\r")
        
        if toind==total then
            return
        end
        
        while true do
            sendtochar( ch, "[n]ext, [q]uit: ")
            local cmd=coroutine.yield()
            
            if cmd=="n" or cmd=="" then
                ind=toind+1
                break
            elseif cmd=="q" then
                return
            else 
                sendtochar( ch, "\n\rInvalid: "..cmd.."\n\r")
            end
        end
    end
end

local function get_tokens(str)
    local tokens={}
    local len=str:len()
    local ind=1
    while true do
        local token=""
        local instring=false
        local indoubquote=false
        local insingquote=false
        local parenslevel=0
        while true do
            local char=str:sub(ind,ind)
            
            if ind>len 
            or (char=="," and not(instring) and not(parenslevel>0)) then
                table.insert(tokens,token)
                break
            --elseif char==" " and not(instring) and not(parenslevel>0) then
                -- ignore non literal whitespace
            elseif char=="(" and not(instring) then
                parenslevel=parenslevel+1
                token=token..char
            elseif char==")" and not(instring) then
                parenslevel=parenslevel-1
                token=token..char
            elseif char=="\"" then
                if instring then
                    if indoubquote then
                        instring=false
                        doubquote=false
                    end
                else 
                    instring=true
                    indoubquote=true
                end
                token=token..char
            elseif char=="'" then
                if instring then
                    if insingquote then
                        instring=false
                        insingquote=false
                    end
                else
                    instring=true
                    insingquote=true
                end
                token=token..char
            else
                token=token..char
            end

            ind=ind+1
        end
        if ind>=len then break end
        ind=ind+1
    end

    return tokens
end

function do_luaquery( ch, argument)
    if argument=="" or argument=="help" then
        luaquery_usage(ch)
        return
    end

    local typearg
    local columnarg
    local filterarg
    local sortarg
    local widtharg
    local limitarg

    local getfun
    local selection
    local aliases={}
    local sorts


    local ind=string.find(argument, " from ")
    if not ind then
        sendtochar(ch, "expected 'from' keyword\n\r")
        return
    end


    local slct_list=argument:sub( 1, ind-1)
    selection=get_tokens(slct_list)


    typearg=string.match( argument:sub(ind+6), "%w+")
    -- what type are we searching ?
    local lqent=lqtbl[typearg]
    if lqent then
        getfun=lqent.getfun
    else
        sendtochar(ch,"Invalid type arg: "..typearg)
        return
    end

    -- handle * or default in select list
    local old_selection=selection
    selection={}
    for i,v in ipairs(old_selection) do
        if v=="*" or v=="default" then
            for _,field in ipairs(lqent.default_sel) do
                table.insert(selection, field)
            end
        else
            table.insert(selection,v)
        end
    end

    -- check for aliases in selection
    for k,v in pairs(selection) do
        local start,fin=v:find(" as ")
        if start then
            aliases[v:sub(fin+1)]=v:sub(1,start-1)
            selection[k]=v:sub(fin+1)
        end
    end

    local rest_arg=argument:sub(ind+(" from "):len()+typearg:len())

    local whereind=string.find(rest_arg, " where ")
    local orderbyind=string.find(rest_arg, " order by ")
    local widthind=string.find(rest_arg, " width ")
    local limitind=string.find(rest_arg, " limit ")

    filterarg=whereind and rest_arg:sub(whereind+7, orderbyind or widthind or limitind) or ""
    
    if orderbyind then
        local last=widthind or limitind
        last=last and (last-1)
        sortarg=rest_arg:sub(orderbyind+10, last)     
        sorts=get_tokens(sortarg)
    end

    if widthind then
        local last=limitind
        last=last and (last-1)
        widtharg=tonumber(rest_arg:sub(widthind+7, last))
        if not widtharg then
            sendtochar(ch, "width argument must be a number\n\r")
            return
        end
    end

    if limitind then
        local last
        last=last and (last-1)
        limitarg=tonumber(rest_arg:sub(limitind+7, last))
        if not limitarg then
            sendtochar(ch, "limit argument must be a number\n\r")
            return
        end 
    end


    -- let's get our result
    local lst=getfun()

    local rslt={}
    if not(filterarg=="") then
        local alias_funcs={}
        for k,v in pairs(aliases) do
            alias_funcs[k]=loadstring("return function(x) return "..v.." end")()
        end

        local filterfun=function(gobj)
            local vf,err=loadstring("return function(x) return "..filterarg.." end" )
            if err then error(err) return end
            local fenv={ pairs=pairs }
            local mt={ 
                __index=function(t,k)
                    if k=="thisarea" then
                        return ch.room.area==gobj.area
                    elseif alias_funcs[k] then
                        return alias_funcs[k](gobj) 
                    elseif type(gobj[k])=="function" then
                        return function(...)
                            return gobj[k](gobj, ...)
                        end
                    else
                        return gobj[k]
                    end
                end,
                __newindex=function ()
                    error("Can't set values with luaquery")
                end
            } 
            setmetatable(fenv,mt)
            setfenv(vf, fenv)
            for k,v in pairs(aliases) do
                setfenv(alias_funcs[k], fenv)
            end
            local val=vf()(gobj)
            if val then return true
            else return false end
        end

        for k,v in pairs(lst) do
            if filterfun(v) then table.insert(rslt, v) end
        end
    else
        rslt=lst
    end

    -- now populate output table based on our column selection
    local output={}
    for _,gobj in pairs(rslt) do
        local line={}
        for _,sel in ipairs(selection) do
            local vf,err=loadstring("return function(x) return "..(aliases[sel] or sel).." end")
            if err then sendtochar(ch, err) return  end
            setfenv(vf,
                    setmetatable(
                        {
                            pairs=pairs
                        }, 
                        {   
                            __index=function(t,k)
                                if type(gobj[k])=="function" then
                                    return function(...)
                                        return gobj[k](gobj, ...)
                                    end
                                else
                                    return gobj[k]
                                end
                            end,
                            __newindex=function () 
                                error("Can't set values with luaquery") 
                            end
                        } 
                    )
            )
            table.insert(line, { col=sel, val=tostring(vf()(gobj))} )
        end
        table.insert(output, line)

    end

    if #output<1 then
        sendtochar( ch, "No results.\n\r")
        return
    end

    -- now sort
    if sorts then

        local fun
        fun=function(a,b,lvl)
            local aval
            for k,v in pairs(a) do
                if v.col==sorts[lvl] then
                    aval=v.val
                    break
                end
            end
            
            if not(aval) then
                error("Bad sort argument '"..sorts[lvl].."'\n\r")
            end

            local bval
            for k,v in pairs(b) do
                if v.col==sorts[lvl] then
                    bval=v.val
                    break
                end
            end

            if tonumber(aval) then aval=tonumber(aval) end
            if tonumber(bval) then bval=tonumber(bval) end
            if aval==bval and sorts[lvl+1] then
                return fun(a, b, lvl+1)
            else
                return aval>bval
            end
        end

        local status,err=pcall( function()
            table.sort(output, function(a,b) return fun(a,b,1) end )
        end)
        if not(status) then
            sendtochar(ch,err.."\n\r")
            return
        end

    end


    -- NOW PRINT
     -- first scan through to determine column widths
    local widths={}
    local hdr={}
    for _,v in pairs(output) do
        for _,v2 in ipairs(v) do
            if not(widths[v2.col]) then
                widths[v2.col]=widtharg and 
                    math.min(util.strlen_color(v2.val), widtharg) or
                    util.strlen_color(v2.val)
                table.insert(hdr, v2.col)
            else
                local ln=widtharg and
                    math.min(util.strlen_color(v2.val), widtharg) or
                    util.strlen_color(v2.val)
                if ln>widths[v2.col] then
                    widths[v2.col]=ln
                end
            end
        end
    end

    local printing={}
    -- header
    local hdrstr={}
    for _,v in ipairs(hdr) do
        table.insert(hdrstr,
                util.format_color_string( v, widths[v]+1))
    end
    local hdr="|"..table.concat(hdrstr,"|").."|"

    local iter
    if not(limitarg) then
        iter=ipairs
    elseif limitarg<0 then
        iter=function(tbl)
            local i=math.max(1,#tbl+limitarg) -- limitarg is negative
            local limit=#tbl
            return function()
                i=i+1
                if i<=limit then return i,tbl[i] end
            end
        end
    else
        iter=function(tbl)
            local i=0
            local limit=math.min(#tbl, limitarg)
            return function()
                i=i+1
                if i<=limit then return i,tbl[i] end
            end
        end
    end

    for _,v in iter(output) do
        local line={}
        for _,v2 in ipairs(v) do
            local cc=v2.val:len() - util.strlen_color(v2.val)
            local width=widths[v2.col]
            table.insert( line,
                    util.format_color_string(
                        v2.val,
                        widths[v2.col]
                    )
                    .." {x"
            )
        end
        local ln=table.concat(line,"|")
        table.insert(printing, 
                "|"..ln.."|")
    end

    start_con_handler( ch.descriptor, luaq_results_con, 
            ch,
            argument, 
            hdr,
            printing)

end
-- end luaquery section

-- luaconfig section
local syn_cfg_tbl=
{
    "keywords",
    "boolean",
    "nil",
    "function",
    "operator",
    "global",
    "comment",
    "string",
    "number"
}

configs_table = configs_table or {}

local function show_luaconfig_usage( ch )
    pagetochar( ch,
[[

luaconfig list
luaconfig <name> <color char>

]])

end

local xtermcols=
{
    "r",
    "R",
    "g",
    "G",
    "y",
    "Y",
    "b",
    "B",
    "m",
    "M",
    "c",
    "C",
    "w",
    "W",
    "a",
    "A",
    "j",
    "J",
    "l",
    "L",
    "o",
    "O",
    "p",
    "P",
    "t",
    "T",
    "v",
    "V"
}
function do_luaconfig( ch, argument )
    local args=arguments(argument, true)

    if not(args[1]) then
        show_luaconfig_usage( ch )
        return
    end

    local cfg=configs_table[ch] -- maybe nil

    if (args[1] == "list") then
        sendtochar(ch,
                string.format("%-15s %s\n\r",
                    "Setting Name",
                    "Your setting") )

        for k,v in pairs(syn_cfg_tbl) do
            local char=cfg and cfg[v]

            sendtochar(ch,
                    string.format("%-15s %s\n\r",
                        v,
                        ( (char and ("\t"..char..char.."\tn")) or "") ) )
        end

        sendtochar( ch, "\n\rSupported colors:\n\r")
        for k,v in pairs(xtermcols) do
            sendtochar( ch, "\t"..v..v.." ")
        end
        sendtochar( ch, "\tn\n\r")
        return
    end

    for k,v in pairs(syn_cfg_tbl) do
        if (args[1]==v) then
            configs_table[ch]=configs_table[ch] or {}
            if not(args[2]) then
                configs_table[ch][v]=nil
                sendtochar( ch, "Config cleared for "..v.."\n\r")
                return
            end

            for l,w in pairs(xtermcols) do
                if w==args[2] then
                    configs_table[ch][v]=w
                    sendtochar( ch, "Config for "..v.." set to \t"..w..w.."\tn\n\r")
                    return
                end
            end
            
            sendtochar(ch, "Invalid argument: "..args[2].."\n\r")    
            return
        end
    end

    show_luaconfig_usage( ch )

end

function colorize( text, ch )
    config=(ch and configs_table[ch]) or {}
    local rtn={}
    local len=#text
    local i=0
    local word
    local waitfor
    local funtrack={}
    local nestlevel=0 -- how many functions are we inside

    while (i < len) do
        i=i+1
        local char=text:sub(i,i)

        if waitfor then
            if waitfor=='\n' 
                and waitfor==char 
                then
                waitfor=nil
                table.insert(rtn,"\tn"..char)
            elseif waitfor==']]' 
                and waitfor==text:sub(i,i+1) 
                then
                table.insert(rtn,"]]\tn")
                waitfor=nil
                i=i+1
            elseif waitfor=='--]]'
                and waitfor==text:sub(i,i+3)
                then
                table.insert(rtn,"--]]\tn")
                waitfor=nil
                i=i+3
            elseif char==waitfor then
                -- ends up handling ' and "
                waitfor=nil
                table.insert(rtn, char.."\tn")
            else
                -- waitfor didn't match, just push the char
                table.insert(rtn, char)
            end
        -- Literal strings
        elseif char=='"' or char=="'" then
            table.insert(rtn, "\t"..(config["string"] or 'r')..char)
            waitfor=char
        -- Multiline strings
        elseif char=='[' and text:sub(i+1,i+1) == '[' then
            table.insert(rtn, "\t"..(config["string"] or 'r').."[[")
            i=i+1
            waitfor=']]'
        -- Multiline comments
        elseif char=='-' and text:sub(i+1,i+3) == "-[[" then
            table.insert(rtn, "\t"..(config["comment"] or 'c').."--[[")
            i=i+3
            waitfor='--]]'
        -- Single line comments
        elseif char=='-' and text:sub(i+1,i+1) == '-' then
            table.insert(rtn, "\t"..(config["comment"] or 'c').."--")
            i=i+1
            waitfor='\n'
        elseif char=='\t' then
            table.insert(rtn, "    ")
        -- Operators
        elseif char=='[' or char==']'
            or char=='(' or char==')'
            or char=='=' or char=='%'
            or char=='<' or char=='>'
            or char=='{' or char=='}'
            or char=='/' or char=='*'
            or char=='+' or char=='-'
            or char==',' or char=='.'
            or char==":" or char==";"
            then
            table.insert(rtn, "\t"..(config["operator"] or 'G')..char.."\tn")
        -- Words
        elseif string.find(char, "%a") then
            local start,finish,word=string.find(text,"(%a[%w_%.]*)",i)
            i=finish
            if word=="function" then
                table.insert(funtrack,1,nestlevel)
                nestlevel=nestlevel+1
                table.insert(rtn, "\t"..(config["function"] or 'C')..word.."\tn")
            -- these two words account for do, while, if, and for
            elseif word=="do" or word=="if" then
                nestlevel=nestlevel+1
                table.insert(rtn, "\t"..(config["keywords"] or 'Y')..word.."\tn")
            elseif word=="end" then
                nestlevel=nestlevel-1
                if funtrack[1] and funtrack[1]==nestlevel then
                    table.remove(funtrack,1)
                    table.insert(rtn, "\t"..(config["function"] or 'C')..word.."\tn")
                else
                    table.insert(rtn, "\t"..(config["keywords"] or 'Y')..word.."\tn")
                end
            -- boolean
            elseif word=="true" or word=="false" then
                table.insert(rtn, "\t"..(config["boolean"] or 'r')..word.."\tn")
            -- 'keywords'
            elseif word=="and" or word=="in" or word=="repeat"
                or word=="break" or word=="local" or word=="return"
                or word=="for" or word=="then" or word=="else"
                or word=="not" or word=="elseif" or word=="if"
                or word=="or" or word=="until" or word=="while"
                then
                table.insert(rtn, "\t"..(config["keywords"] or 'Y')..word.."\tn")
            -- nil
            elseif word=="nil" then
                table.insert(rtn, "\t"..(config["nil"] or 'r')..word.."\tn")
            else
                -- Search globals
                local found=false
                for k,v in pairs(main_lib_names) do
                    if word==v then
                        table.insert(rtn, "\t"..(config["global"] or 'C')..word.."\tn")
                        found=true
                        break
                    end
                end

                -- Nothing special, just shove it
                if not(found) then
                    table.insert(rtn,word)
                end
            end
        -- Numbers
        elseif string.find(char, "%d") then
            local start,finish=string.find(text,"([%d%.]+)",i)
            word=text:sub(start,finish)
            i=finish
            table.insert(rtn, "\t"..(config["number"] or 'm')..word.."\tn")
        else
            -- Whatever else
            table.insert(rtn,char)
        end
    end

    return table.concat(rtn)

end

function save_luaconfig( ch )
    if not(configs_table[ch]) then return nil end

    rtn=serialize.save("cfg",configs_table[ch])
    return rtn
end

function load_luaconfig( ch, text )
    configs_table[ch]=loadstring(text)()
end

-- end luaconfig section

-- scriptdump section
local function scriptdumpusage( ch )
    sendtochar(ch, [[
scriptdump <userdir> <scriptname> [true|false]

Third argument (true/false) prints line numbers if true. Defaults to true
if not provided.
                   
Example: scriptdump vodur testscript false 
]])
end


function do_scriptdump( ch, argument )
    args=arguments(argument, true)
    if #args < 2 or #args > 3  then
        scriptdumpusage(ch)
        return
    end

    if not(args[3]=="false") then
        pagetochar( ch, linenumber(colorize(GetScript( args[1], args[2] ), ch)).."\n\r", true )
    else
        pagetochar( ch, colorize(GetScript( args[1], args[2] )).."\n\r", true )
    end

end
-- end scriptdump section

-- wizhelp section
function wizhelp( ch, argument, commands )
    local args=arguments(argument)
    if args[1]=="level" or args[1]=="name" then
        table.sort( commands, function(a,b) return a[args[1]]<b[args[1]] end )

        if args[1]=="level" and args[2] then
            local old=commands
            commands=nil
            commands={}
            for i,v in ipairs(old) do
                if v.level==tonumber(args[2]) then
                    table.insert(commands, v)
                end
            end
        end
    elseif args[1]=="find" then
        local old=commands
        commands=nil
        commands={}
        for i,v in ipairs(old) do
            if string.find( v.name, args[2] ) then
                table.insert(commands, v)
            end
        end
    end

    local columns={}
    local numcmds=#commands
    local numrows=math.ceil(numcmds/3)
    
    for i,v in pairs(commands) do
        local row
        row=i%numrows
        if row==0 then row=numrows end

        columns[row]=columns[row] or ""
        columns[row]=string.format("%s %4s %-10s (%3d) ",
                columns[row],
                i..".",
                v.name,
                v.level)

    end

    pagetochar( ch, table.concat(columns, "\n\r")..
[[ 

   wizhelp <name|level>   -- List commands sorted by name or level.
   wizhelp level [number] -- List commands at given level.
   wizhelp find [pattern] -- Find commands matching given pattern (lua pattern matching).
]] )

end
--end wizhelp section

-- alist section
local alist_col={
    "vnum",
    "name",
    "builders",
    "minvnum",
    "maxvnum",
    "security"
}

local function alist_usage( ch )
    sendtochar( ch,
[[
Syntax:  alist
         alist [column name] <ingame>
         alist unused
         alist find [text]


'alist' with no argument shows all areas, sorted by area vnum.

'alist unused' shows which vnum ranges are unused.

With a column name argument, the list is sorted by the given column name.
'ingame' is used as an optional 3rd argument to show only in game areas.

With 'find' argument, the area names and builders are searched for the
given text (case sensitive)  and only areas that match are shown.

Columns:
]])

    for k,v in pairs(alist_col) do
        sendtochar( ch, v.."\n\r")
    end
end

function do_alist( ch, argument )
    local args=arguments(argument, true)
    local sort
    local filterfun

    if not(args[1]) then
        sort="vnum"
    elseif args[1]=="find" then
        sort="vnum"
        if not(args[2]) then
            alist_usage( ch )
            return
        end
        filterfun=function(area)
            if area.name:find(args[2])
                or area.builders:find(args[2])
                then
                return true
            end
            return false
        end
    elseif args[1]=="unused" then
        local alist=getarealist()
        table.sort(alist, function(a,b) return a.minvnum<b.minvnum end)

        -- assume no gap at the beginning (vnum 1)
        sendtochar(ch, " Min   -  Max   [    count   ]\n\r")
        sendtochar(ch, ("-"):rep(80).."\n\r")
        for i,area in ipairs(alist) do
            local area_next=alist[i+1]
            if not(area_next) then break end

            local gap=area_next.minvnum-area.maxvnum-1
            if gap>0 then
                sendtochar(ch, string.format("%6s - %6s [%6s vnums]\n\r",
                            area.maxvnum+1,
                            area_next.minvnum-1,
                            gap))
            end
        end

        return
    else
        for k,v in pairs(alist_col) do
            if v==args[1] then
                sort=v
                break
            end
        end
        if args[2]=="ingame" then
            filterfun=function(area)
                return area.ingame
            end
        end
    end

    if not(sort) then
        alist_usage( ch )
        return
    end

    local data={}
    for _,area in pairs(getarealist()) do
        local fil
        if filterfun then
            fil=filterfun(area)
        else
            fil=true
        end

        if fil then
            local row={}
            for _,col in pairs(alist_col) do
                row[col]=area[col]
            end
            table.insert(data, row)
        end
    end

    table.sort( data, function(a,b) return a[sort]<b[sort] end )

    local output={}
    --header
    table.insert( output,
            string.format("[%3s] [%-26s] [%-20s] (%-6s-%6s) [%-3s]",
                "Num", "Name", "Builders", "Lvnum", "Mvnum", "Sec"))
    for _,v in ipairs(data) do
        table.insert( output,
                string.format("[%3d] [%-26s] [%-20s] (%-6d-%6d) [%-3d]",
                    v.vnum,
                    util.format_color_string(v.name,25).." {x",
                    v.builders,
                    v.minvnum,
                    v.maxvnum,
                    v.security)
        )
    end

    pagetochar( ch, table.concat(output, "\n\r").."\n\r" )

end
--end alist section

--mudconfig section
local function mudconfig_usage(ch)
    sendtochar(ch,
[[
Syntax:
   mudconfig                  -- get the current value for all options
   mudconfig <option>         -- get the current value for given option
   mudconfig <option> <value> -- set a new value for the given option
]])
end
function do_mudconfig( ch, argument )
    local args=arguments(argument)
    local nargs=#args
    -- setting
    if nargs==2 then
        -- lua won't parse/convert booleans for us
        if args[2]=="true" or args[2]=="TRUE" then
            args[2]=true
        elseif args[2]=="false" or args[2]=="FALSE" then
            args[2]=false
        end

        mudconfig(unpack(args))
        -- show result
        do_mudconfig( ch, args[1])
        sendtochar(ch, "Done.\n\r")
        return
    end

    if nargs==1 then
        local rtn=mudconfig(args[1])
        sendtochar(ch, "%-20s %s\n\r", args[1], tostring(rtn))
        return
    end

    if nargs==0 then
        local sorted={}
        for k,v in pairs(mudconfig()) do
            table.insert(sorted, { key=k, value=v } )
        end

        table.sort( sorted, function(a,b) return a.key<b.key end )

        for _,v in ipairs(sorted) do
            sendtochar(ch, "%-20s %s\n\r", v.key, tostring(v.value))
        end
        return
    end

    mudconfig_usage(ch)
    return
end
--end mudconfig section

-- perfmon section
local lastpulse
local pulsetbl={}
local pulseind=1
local sectbl={}
local secind=1
local mintbl={}
local minind=1
local hourtbl={}
local hourind=1
function log_perf( val )
    lastpulse=val
    pulsetbl[pulseind]=val

    if pulseind<4 then 
        pulseind=pulseind+1
        return 
    else
        pulseind=1
    end

    -- 1 second complete, add to table
    local total=0
    for i=1,4 do
        total=total+pulsetbl[i]
    end

    sectbl[secind]=total/4

    if secind<60 then
        secind=secind+1
        return
    else
        secind=1
    end

    -- 1 minute complete, add to table
    total=0
    for i=1,60 do
        total=total+sectbl[i]
    end

    mintbl[minind]=total/60

    if minind<60 then
        minind=minind+1
        return
    else
        minind=1
    end

    -- 1 hour complete, add to table
    total=0
    for i=1,60 do
        total=total+mintbl[i]
    end

    hourtbl[hourind]=total/60

    if hourind<24 then
        hourind=hourind+1
        return
    else
        hourind=1
    end
        
end

function do_perfmon( ch, argument )
    local function avg( tbl )
        local cnt=0
        local ttl=0
        for k,v in pairs(tbl) do
            cnt=cnt+1
            ttl=ttl+v
        end

        return ttl/cnt
    end

    pagetochar( ch,
([[
Averages
  %3d Pulse:    %f
  %3d Pulses:   %f
  %3d Seconds:  %f
  %3d Minutes:  %f
  %3d Hours:    %f
]]):format( 1, lastpulse,
            #pulsetbl, avg(pulsetbl),
            #sectbl, avg(sectbl),
            #mintbl, avg(mintbl),
            #hourtbl, avg(hourtbl) ) )

end

--end perfmon section

-- findreset section
local function find_obj_noreset( ch, arg )
    local ingame= ( arg == "ingame" )
    -- build reset lookup
    local lookup={}
    for _,area in pairs(getarealist()) do
        for _,room in pairs(area.rooms) do
            for _,reset in pairs(room.resets) do
                local cmd=reset.command
                if cmd=="O" or
                   cmd=="P" or
                   cmd=="G" or
                   cmd=="E" then 
                    lookup[reset.arg1]=true
                end
            end
        end
    end

    -- now do the check
    local result={}
    for _,area in pairs(getarealist()) do
        for _,op in pairs(area.objprotos) do
            if not(lookup[op.vnum]) then
                if not(ingame) and true or op.ingame then
                table.insert( result,
                        ("%s{x [%6d] %s{x"):format(
                                util.format_color_string( area.name, 15 ),
                                op.vnum,
                                op.shortdescr) )
                end
            end
        end
    end

    local out={}
    table.insert(out, "Objs with no resets")
    for i,v in ipairs(result) do
        table.insert( out, ("%3d. "):format(i)..v )
    end
    pagetochar( ch, table.concat( out, "\n\r" ).."\n\r" )
end

local function find_mob_noreset( ch, arg )
    local ingame= ( arg == "ingame" )
    -- build reset lookup
    local lookup={}
    for _,area in pairs(getarealist()) do
        for _,room in pairs(area.rooms) do
            for _,reset in pairs(room.resets) do
                if reset.command=="M" then
                    lookup[reset.arg1]=true
                end
            end
        end
    end

    -- now do the check
    local result={}
    for _,area in pairs(getarealist()) do
        for _,mp in pairs(area.mobprotos) do
            if not(lookup[mp.vnum]) then
                if not(ingame) and true or mp.ingame then
                table.insert( result,
                        ("%s{x [%6d] %s{x"):format(
                                util.format_color_string( area.name, 15 ),
                                mp.vnum,
                                mp.shortdescr) )
                end
            end
        end
    end

    local out={}
    table.insert(out, "Mobs with no resets")
    for i,v in ipairs(result) do
        table.insert( out, ("%3d. "):format(i)..v )
    end
    pagetochar( ch, table.concat( out, "\n\r" ).."\n\r" )
end

local function find_mob_reset( ch, vnum )
    local mp=getmobproto( vnum )
    if not(mp) then
        sendtochar( ch, "Invalid mob vnum "..vnum..".\n\r")
        return
    end

    vnum=tonumber(vnum)

    local result={}

    for _,area in pairs(getarealist()) do
        for _,room in pairs(area.rooms) do
            for _,reset in pairs(room.resets) do
                if reset.command=="M" then
                    if reset.arg1==vnum then
                        table.insert( result,
                                ("[%6d] %s"):format(room.vnum, room.name) )
                    end
                end
            end
        end
    end

    if #result<1 then
        sendtochar(ch, "No reset found.\n\r")
        return
    end
  
    local out={}
    table.insert( out, "Resets for mob '"..mp.shortdescr.."' ("..vnum..")")
    for i,v in ipairs(result) do
        table.insert( out, ("%3d. "):format(i)..v )
    end 
    pagetochar( ch, table.concat( out, "\n\r" ).."\n\r" )   
end

local function find_obj_reset( ch, vnum )
    local op=getobjproto( vnum )
    if not(op) then
        sendtochar( ch, "Invalid obj vnum "..vnum..".\n\r")
        return
    end

    vnum=tonumber(vnum)

    local result={}

    for _,area in pairs(getarealist()) do
        for _,room in pairs(area.rooms) do
            for _,reset in pairs(room.resets) do
                local cmd=reset.command
                if (cmd=='O' or
                   cmd=='P' or
                   cmd=='G' or
                   cmd=='E') and
                   reset.arg1==vnum 
                   then
                   table.insert( result,
                           ("[%6d] %s"):format(room.vnum, room.name) )
                end
            end
        end
    end

    if #result<1 then
        sendtochar(ch, "No reset found.\n\r")
        return
    end

    local out={}
    table.insert( out, "Resets for obj '"..op.shortdescr.."' ("..vnum..")")
    for i,v in ipairs(result) do
        table.insert( out, ("%3d. "):format(i)..v )
    end
    pagetochar( ch, table.concat( out, "\n\r" ).."\n\r" )
end

local function findreset_usage( ch )
    pagetochar( ch,
[[
Find resets for given mob or object:
    findreset mob [vnum]
    findreset obj [vnum]

Find mobs or objects with no resets:
    findreset mob noreset <ingame>
    findreset obj noreset <ingame>
]])

end

function do_findreset( ch, argument )
    local args=arguments(argument)

    if args[1] == "mob" then
        if args[2] == "noreset" then
            find_mob_noreset( ch, args[3] )
        else
            find_mob_reset( ch, args[2] )
        end
        return
    elseif args[1] == "obj" then
        if args[2] == "noreset" then
            find_obj_noreset( ch, args[3] )
        else
            find_obj_reset( ch, args[2] )
        end
        return
    end

    findreset_usage( ch )
end
-- end findreset section

-- diagnostic section
local function CH_diag( ch, args )
    local chs={}
    local reg=debug.getregistry()
    for k,v in pairs(reg) do
        if tostring(v)=="CH" then
            chs[v]=true
        end
    end

    for k,v in pairs(reg) do
        if tostring(v)=="DESCRIPTOR" then
            if v.character then
                chs[v.character]=nil
            end
        end
    end

    for k,v in pairs(getcharlist()) do
        chs[v]=nil
    end

    sendtochar( ch, "Leaked CHAR_DATAs:\n\r")
    local cnt=0
    for k,v in pairs(chs) do
        sendtochar( ch, k.name.."\n\r")
        cnt=cnt+1
    end

    sendtochar(ch, "\n\rCount: "..cnt.."\n\r")

    sendtochar( ch, "\n\rCHAR_DATAs with no room:\n\r")
    cnt=0
    for k,v in pairs(getcharlist()) do
        if not(v.room) then
            sendtochar( ch, string.format(
                        "%s %s\n\r",
                        v.isnpc and ("["..v.vnum.."]") or "",
                        v.name) )
            cnt=cnt+1
        end
    end
    sendtochar(ch, "\n\rCount: "..cnt.."\n\r")
end

local function DESCRIPTOR_diag( ch, args )
    local ds={}
    local reg=debug.getregistry()
    for k,v in pairs(reg) do
        if tostring(v)=="DESCRIPTOR" then
            ds[v]=true
        end
    end

    for k,v in pairs(getdescriptorlist()) do
        ds[v]=nil
    end

    sendtochar( ch, "Leaked DESCRIPTORs:\n\r")
    local cnt=0
    for k,v in pairs(ds) do
        if v.character then
            sendtochar( ch, v.character.name.."\n\r")
        else
            sendtochar( ch, "NULL character\n\r")
        end
        cnt=cnt+1
    end

    sendtochar(ch, "\n\rCount: "..cnt.."\n\r")
end

local function OBJ_diag( ch, args )
    local objs={}
    local reg=debug.getregistry()
    for k,v in pairs(reg) do
        if tostring(v)=="OBJ" then
            objs[v]=true
        end
    end

    for k,v in pairs(getobjlist()) do
        objs[v]=nil
    end

    sendtochar( ch, "Leaked OBJ_DATAs:\n\r")
    local cnt=0
    for k,v in pairs(objs) do
        sendtochar( ch, ("[%5d] %s\n\r"):format( v.vnum, v.shortdescr) )
        cnt=cnt+1
    end

    sendtochar(ch, "\n\rCount: "..cnt.."\n\r")

    sendtochar( ch, "\n\rOBJ_DATAs with no room:\n\r")
    cnt=0
    for k,v in pairs(getobjlist()) do
        if not(v.room) and not(v.inobj) and not(v.carriedby) then
            sendtochar( ch, ("[%5d] %s\n\r"):format( v.vnum, v.shortdescr )  )
            cnt=cnt+1
        end
    end
    sendtochar(ch, "\n\rCount: "..cnt.."\n\r")
end

local function rexit_diag_here( ch, args )
    local out={}
    local here=ch.room

    table.insert( out, "Room exits linking here: \n\r" )
    local cnt=0
    for _,area in pairs(getarealist()) do
        for _,room in pairs(area.rooms) do
            for _,exit in pairs(room.exits) do
                if room[exit].toroom==here then
                    cnt=cnt+1
                    table.insert( out, 
                      ("%3d. %s{x [%6d] %s{x %s\n\r"):format(
                        cnt,
                        util.format_color_string( area.name, 20 ),
                        room.vnum,
                        util.format_color_string( room.name, 20 ),
                        exit)
                    )
                end
            end
        end
    end

    table.insert( out, "\n\r")

    table.insert( out, "Portals linking here: \n\r" )
    cnt=0
    for _,area in pairs(getarealist()) do
        for _,op in pairs(area.objprotos) do
            if op.otype=="portal" then
                if op.toroom==here.vnum then
                    cnt=cnt+1
                    table.insert( out,
                      ("%3d. %s{x [%6d] %s{x\n\r"):format(
                        cnt,
                        util.format_color_string( area.name, 20 ),
                        op.vnum,
                        op.shortdescr)
                    )
                end
            end
        end
    end
    
    pagetochar( ch, table.concat( out ).."\n\r" )
end

local function rexit_diag( ch, args )
    local ingame=args[1]=="ingame"
    local unlinked={}
    local rooms={}
    local portals={}

    local reg=debug.getregistry()
    for k,v in pairs(reg) do
        if tostring(v)=="ROOM" then
            unlinked[v]=true
            table.insert(rooms, v )
        end
    end

    for _,room in pairs(rooms) do
        for _,exit in pairs(room.exits) do
            if room[exit].toroom then
                unlinked[room[exit].toroom]=nil
            end
        end
    end

    for k,v in pairs(reg) do
        if tostring(v)=="OBJPROTO" then
            if v.otype=="portal" then
                table.insert(portals, v)
            end
        end
    end

    for _,portal in pairs(portals) do
        local r=getroom(portal.toroom)
        if r then
            unlinked[r]=nil
        end
    end

    local cnt=0
    local out={}
    local sorted={}
    for k,v in pairs(unlinked) do
        if (not(ingame)  or k.ingame) then
            table.insert(sorted, k)
        end
    end
    table.sort(sorted, function(a,b) return a.area.name<b.area.name end )
    for k,v in pairs(sorted) do
        cnt=cnt+1
        table.insert( out, ("%3d. [%6d] %s{x %s{x\n\r"):format( 
                cnt, 
                v.vnum, 
                util.format_color_string(v.area.name,20), 
                v.name ) )
    end
    sendtochar( ch, cnt.."\n\r")
    pagetochar( ch, table.concat(out).."\n\r" )
end

local function diagnostic_usage( ch )
    pagetochar( ch, [[
diagnostic ch   -- Run CHAR_DATA diagnostic
diagnostic desc -- Run DESCRIPTOR_DATA diagnostic
diagnostic obj  -- Run OBJ diagnostic.
diagnostic rexit -- List rooms that have no link to them from room or portal
                    Optional arg 'ingame' to show only ingame rooms.
diagnostic rexit here -- List all exits leading to current room.
]])
end
function do_diagnostic( ch, argument )
    local args=arguments(argument)
    local arg1=args[1]
    table.remove(args,1)
    if arg1=="ch" then
        CH_diag( ch, args )
    elseif arg1=="desc" then
        DESCRIPTOR_diag( ch, args )
    elseif arg1=="obj" then
        OBJ_diag( ch, args )
    elseif arg1=="rexit" then
        if args[1]=="here" then
            rexit_diag_here( ch, args )
        else
            rexit_diag( ch, args )
        end
    else
        diagnostic_usage( ch )
    end
end
-- end diagnostic section

-- path section
local pathshort={
    north="n",
    south="s",
    east="e",
    west="w",
    up="u",
    down="d",
    northeast="ne",
    southeast="se",
    southwest="sw",
    northwest="nw"
}
local function pretty_path( path )
    local p={}
    local ind=0
    local cnt
    local last
    for i,v in pairs(path) do
        if v==last then
            p[ind].count=p[ind].count+1
        else
            ind=ind+1
            p[ind]={count=1, dir=pathshort[v] and pathshort[v] or "\""..v.."\""}
            last=v
        end
    end

    local rtn={}
    for i,v in ipairs(p) do
        table.insert(rtn, ( (v.count>1) and v.count or "" )..v.dir)
    end

    return rtn
end

local function path_usage( ch )
    sendtochar( ch, [[
Print the path between two rooms.

Syntax:
  path <fromvnum> <tovnum>
  path <tovnum> -- Uses current room as the 'from' room.

Examples:

  path 10204 10256
  path 1234
]] )
end

function do_path( ch, argument )
    local args=arguments(argument)
    
    local fromroom
    local toroom

    if #args==1 then
        fromroom=ch.room
        toroom=getroom(tonumber(args[1]))
    elseif #args==2 then
        fromroom=getroom(tonumber(args[1]))
        toroom=getroom(tonumber(args[2]))
    else
        path_usage( ch )
        return
    end
    assert(fromroom, "Invalid start room.")
    assert(toroom, "Invalid target room.")


    local path=findpath( fromroom, toroom )

    if not(path) then
        sendtochar( ch, ("No path from %d to %d\n\r"):format( 
                    fromroom.vnum, toroom.vnum) )
        return
    end

    pagetochar( ch, ("Path from %d to %d:\n\r"):format(
                fromroom.vnum, toroom.vnum)
                ..table.concat(pretty_path(path), " " ).."\n\r" )

end

-- luahelp section
local luatypes=getluatype() -- list of type names
local function luahelp_usage( ch )
    local out={}
    table.insert( out, "SECTIONS: \n\r\n\r" )
    table.insert( out, "global\n\r" )
    for _,v in ipairs(luatypes) do
        table.insert( out, v.."\n\r" )
    end

    table.insert( out, [[
Syntax: 
    luahelp <section>
    luahelp <section> <get|set|meth>
    luahelp dump <section> -- Dump for pasting to dokuwiki

Examples:
    luahelp ch
    luahelp global
    luahelp objproto meth
]])

    pagetochar( ch, table.concat(out) )
end

local rowtoggle
local function nextrowcolor()
    rowtoggle=not(rowtoggle)
    return rowtoggle and 'w' or 'D'
end

local SEC_NOSCRIPT=99

local function luahelp_dump( ch, args )
    if args[1]=="glob" or args[1]=="global" then
        local out={}
        local g=getglobals()
        table.insert( out, "<sortable 1>\n\r")
        table.insert( out, "^Function^Security^\n\r")
        for i,v in ipairs(g) do
            table.insert( out, string.format(
                        "| [[.%s|%s]] | %s |\n\r",
                        v.lib and (v.lib..":"..v.name) or v.name,
                        v.lib and (v.lib.."."..v.name) or v.name,
                        v.security == SEC_NOSCRIPT and 'X' or v.security)
            )
        end

        table.insert( out, "</sortable>\n\r" )

        pagetochar(ch, table.concat(out) )
        return
    else
        local t=getluatype(args[1])
        if not(t) then
            sendtochar( ch, "No such type.\n\r")
            return
        end

        local props={}
        for k,v in pairs(t.get) do
            props[v.field]=props[v.field] or {}
            props[v.field].get=v.security == SEC_NOSCRIPT and 'X' or v.security
        end
        for k,v in pairs(t.set) do
            props[v.field]=props[v.field] or {}
            props[v.field].set=v.security == SEC_NOSCRIPT and 'X' or v.security
        end
        local out={}

        table.insert( out, "Properties\n\r")
        table.insert( out, "<sortable 1>\n\r")
        table.insert( out, "^Field^Get^Set^\n\r")
        for k,v in pairs(props) do
            table.insert( out, string.format(
                        "| [[.%s|%s]] | %s | %s |\n\r",
                        k,
                        k,
                        v.get and v.get or "",
                        v.set and v.set or "")
            )
        end

        table.insert( out, "</sortable>\n\r")

        table.insert( out, "Methods\n\r")
        table.insert( out, "<sortable 1>\n\r")
        table.insert( out, "^Method^Security^\n\r")
        for k,v in pairs(t.method) do
            table.insert( out, string.format(
                        "| [[.%s|%s]] | %s |\n\r",
                        v.field,
                        v.field,
                        v.security == SEC_NOSCRIPT and 'X' or v.security)
            )
        end
        table.insert( out, "</sortable>\n\r")

        pagetochar( ch, table.concat( out ) )

    end
end

function do_luahelp( ch, argument )
    if argument=="" then
        luahelp_usage( ch )
        return
    end

    local args=arguments(argument)

    if #args<1 then
        luahelp_usage( ch )
        return
    end

    if args[1] == "dump" then
        table.remove(args, 1 )
        luahelp_dump( ch, args )
        return
    end

    if args[1] == "global" or args[1] == "glob" then
        local show_all= (args[2]=="all")
        local out={}
        
        table.insert(out, "GLOBAL functions\n\r" )
        local g=getglobals()
        for i,v in ipairs(g) do
            if not(v.security==SEC_NOSCRIPT) or show_all then
            table.insert( out, string.format(
                    "{%s[%s] %-40s - \t<a href=\"http://rooflez.com/dokuwiki/doku.php?id=lua:%s:%s\">Reference\t</a>{x\n\r",
                    nextrowcolor(),
                    v.security,
                    v.lib and (v.lib.."."..v.name) or v.name,
                    "global",
                    v.lib and (v.lib..":"..v.name) or v.name)
            )
            end
        end

        pagetochar( ch, table.concat(out) ) 

        return
    else
        local out={}
        local t=getluatype(args[1])
        if not(t) then
            sendtochar( ch, "No such section: "..args[1].."\n\r")
            return
        end
        
        local subsect
        local show_all

        if args[2] then
            if args[2]=="all" then
                -- no subsect
                show_all=true
            else
                subsect=args[2]
                show_all=(args[3]=="all")
            end
        end

        if not(subsect) or subsect=="get" then
            table.insert( out, "\n\rGET properties\n\r")
            for i,v in ipairs(t.get) do
                if not(v.security==SEC_NOSCRIPT) or show_all then
                table.insert( out, string.format(
                            "{%s[%d] %-40s - \t<a href=\"http://rooflez.com/dokuwiki/doku.php?id=lua:%s:%s\">Reference\t</a>{x\n\r",
                            nextrowcolor(),
                            v.security,
                            v.field,
                            args[1],
                            v.field)
                )
                end
            end
        end            

        if not(subsect) or subsect=="set" then
            table.insert( out, "\n\rSET properties\n\r")
            for i,v in ipairs(t.set) do
                if not(v.security==SEC_NOSCRIPT) or show_all then
                table.insert( out, string.format(
                            "{%s[%d] %-40s - \t<a href=\"http://rooflez.com/dokuwiki/doku.php?id=lua:%s:%s\">Reference\t</a>{x\n\r",
                            nextrowcolor(),
                            v.security,
                            v.field,
                            args[1],
                            v.field)
                )
                end
            end
        end

        if not(subsect) or subsect=="meth" then
            table.insert( out, "\n\rMETHODS\n\r")
            for i,v in ipairs(t.method) do
                if not(v.security==SEC_NOSCRIPT) or show_all then
                table.insert( out, string.format(
                            "{%s[%d] %-40s - \t<a href=\"http://rooflez.com/dokuwiki/doku.php?id=lua:%s:%s\">Reference\t</a>{x\n\r",
                            nextrowcolor(),
                            v.security,
                            v.field,
                            args[1],
                            v.field)
                )
                end
            end
        end
 
        pagetochar( ch, table.concat(out) )                
    
    end
end

-- end luahelp section

-- do_achievements_boss section
local boss_table

function update_bossachv_table()
    boss_table={}
    for _,area in pairs(getarealist()) do
        for _,mp in pairs(area.mobprotos) do
            if mp.bossachv then
                table.insert(boss_table, mp)
            end
        end
    end

    table.sort( boss_table, function(a,b) 
            return a.area.minlevel < b.area.minlevel 
    end )
end

function do_achievements_boss( ch, victim)
    local plr={}
    for k,v in pairs(victim.bossachvs) do
        plr[v.vnum] = v.timestamp
    end

    if not(boss_table) then update_bossachv_table() end

    local columns={}
    local nummobs=#boss_table
    local numrows=math.ceil(nummobs/2)
    
    for i,v in pairs(boss_table) do
        local row
        row=i%numrows
        if row==0 then row=numrows end

        columns[row]=columns[row] or "{G|{x"
        columns[row]=string.format("%s{w%4s %s{x %s{G|{x",
                columns[row],
                i..".",
                util.format_color_string(getmobproto(v.vnum).shortdescr, 20),
                util.format_color_string(
                    (plr[v.vnum] and os.date("{y%x{x", plr[v.vnum]) or "{DLocked{x"),
                    9))
        

    end

    local total=#boss_table
    local unlocked=#victim.bossachvs
    local locked=total-unlocked
    pagetochar( ch, "{WBoss Achievements for "..victim.name..
            "\n\r{w----------------------------\n\r"..
            table.concat( columns, "\n\r").."\n\r"..
            "\n\r{wTotal Achievements: "..total..", Total Unlocked: "..unlocked..", Total Locked: "..locked.."\n\r"..
            "{x(Use 'achievement boss rewards' to see rewards table.)\n\r"
    )
end

function do_achievements_boss_reward( ch )
    if not(boss_table) then update_bossachv_table() end

    sendtochar( ch, ("{w%-40s {W%5s{G| {W%5s{G| {W%5s{G| {W%5s{x\n\r"):format(
                "Boss", "QP", "Exp", "Gold", "AchP"))
    sendtochar( ch, ("-"):rep(80).."\n\r")
    for i,v in ipairs(boss_table) do
        sendtochar(ch, ("{w%s {y%5d{G| {c%5d{G| {y%5d{G| {c%5d{x\n\r"):format(
                    util.format_color_string(v.shortdescr, 40),
                    v.bossachv.qp,
                    v.bossachv.exp,
                    v.bossachv.gold,
                    v.bossachv.achp))
    end 
        
end
-- end do_achievements_boss section

-- do_qset section
local function do_qset_usage( ch )
    sendtochar( ch, [[
Syntax:
  For regular qsets:
    qset [player] [id] [value] [timer] [limit]

    Example:
    qset astark 31404 3

  For lua setval:
    qset [player] [setval] [value]

    Use value of 'nil' to clear.

    Examples:
    qset astark blah123 true
    qset astark blah123 nil
]])
    return
end

function do_qset(ch, argument)
    local args=arguments(argument, true)

    if #args<3 then
        do_qset_usage(ch)
        return
    end 
        
    local vic=getpc(args[1])
    if not vic then
        sendtochar( ch, "No such player: "..args[1].."\n\r")
        return
    end

    -- if arg2 is number, it's a qset, otherwise it's setval
    if tonumber(args[2]) then
        ch:qset(vic, unpack(args,2))
        sendtochar( ch, "You have successfully changed "..vic.name.."'s qstatus.\n\r")
        return
    else
        -- make sure we save the right types
        local val

        if tonumber(args[3]) then
            val=tonumber(args[3])
        elseif args[3]=="true" then
            val=true
        elseif args[3]=="false" then
            val=false
        elseif args[3]=="nil" then
            val=nil
        else
            val=args[3]
        end

        vic:setval(args[2], val, true)

        sendtochar( ch, string.format("Setval complete on %s.\n\r%s set to %s (type %s)\n\r", 
                    vic.name, 
                    args[2], 
                    tostring(vic:getval(args[2])), 
                    type(vic:getval(args[2]))))
    end
end
-- end do_qset section

-- do_ptitle section
function save_ptitles( ch )
    local pt=ch.ptitles

    if not pt then return nil end
    
    return table.concat(pt, "|")
end

function load_ptitles( ch, text )
    local tbl={}

    for title in string.gmatch( text, "[^|]+") do
        table.insert(tbl, title)
    end 

    ch.ptitles=tbl
end

local function ptitle_usage( ch )
    sendtochar( ch, [[
ptitle set [title] -- Set your ptitle
ptitle list        -- List your available ptitles
ptitle clear       -- Clear your current ptitle
]])
end

-- make sure that current ptitle is in the list of awarded ptitles
function fix_ptitles(ch)
    -- trim current ptitle
    if ch.ptitle=="" then return end
    
    local ttl=string.match(ch.ptitle, "[^%s]+") -- trim the appended space 

    if not ch.ptitles then
        forceset(ch, "ptitles", {[1]=ttl})
        return
    end

    local lst=ch.ptitles

    for k,v in pairs(ch.ptitles) do
        if v==ttl then return end
    end

    table.insert(lst, ttl) 
    table.sort(lst)
end

function do_ptitle( ch, argument)
    local args=arguments(argument)

    if #args<1 then
        ptitle_usage( ch )
        return
    end

    if args[1]=="list" then
        if not ch.ptitles or #ch.ptitles<1 then
            sendtochar(ch, "You have no ptitles.\n\r")
            return
        end 
        pagetochar( ch, table.concat(ch.ptitles,"\n\r").."\n\r")
        return
    elseif args[1]=="clear" then
        ch.ptitle=""
        sendtochar(ch, "Ptitle cleared.\n\r")
        return
    elseif args[1]=="set" then 
        if not ch.ptitles or #ch.ptitles<1 then
            sendtochar(ch, "You don't have any ptitles available!\n\r")
            return
        end

        for k,v in pairs(ch.ptitles) do
            if args[2]:lower()==v:lower() then
                ch.ptitle=v.." "
                sendtochar(ch, "Ptitle set to "..v.."\n\r")
                return
            end
        end
        
        sendtochar(ch, "Sorry, "..args[2].." is not one of your available ptitles.\n\r")
        return
    else
        ptitle_usage( ch )
        return
    end
end

local ptitle_list={
    "Sir",
    "Maid",
    "Baron",
    "Baroness",
    "Lady",
    "Mistress",
    "Professor",
    "Mr.",
    "Mrs.",
    "Ms.",
    "Miss",
    "Dr.",
    "Doctor",
    "Doc",
    "Herr",
    "Frau",
    "The",
    "Master",
    "Darth",
    "El",
    "La",
    "Prince",
    "Princess",
    "Tzar",
    "Grandpa",
    "Grandma",
    "Hunter",
    "Huntress",
    "Lord",
    "Champion",
    "Warlord",
    "Commander",
    "Uncle",
    "Auntie",
    "Father",
    "Mother",
    "Brother",
    "Sister",
}
local ptitle_cost=200
function quest_buy_ptitle(ch, argument)
    local args=arguments(argument)

    if not(args[1]) or args[1]=="" then
        sendtochar( ch, [[
quest buy ptitle list -- List ptitles available for purchase.
quest buy <title>     -- Purchase a ptitle
]])
        return

    elseif args[1]=="list" then
        local out={}
        table.insert(out, ("%-15s %4s\n\r"):format("Title", "Cost") )

        for i,v in ipairs(ptitle_list) do
            table.insert(out, ("%-15s %4d\n\r"):format(v, ptitle_cost) )
        end

        pagetochar( ch, table.concat(out))
        return
    else
        if ch.questpoints<ptitle_cost then
            sendtochar(ch, "Sorry, you need "..ptitle_cost.." quest points.\n\r")
            return
        end
        for k,v in pairs(ptitle_list) do
            if args[1]:lower()==v:lower() then
                ch.questpoints=ch.questpoints-200
                awardptitle(ch, v)
                sendtochar( ch, ("You have purchased the '%s' ptitle. Use the 'ptitle' command to set it!\n\r"):format(v) )
                return
            end
        end

        sendtochar( ch, "No such ptitle on the list: "..args[1].."\n\r")
        return
    end
end
-- end do_ptitle section
