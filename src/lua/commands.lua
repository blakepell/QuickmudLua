-- luareset section
local reset_table =
{
  "commands",
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
{Cluaquery <selection> from <type> [where <filter>] [order by <sort>] [width <width>] [limit <limit>]
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

]])

end

local lqtbl={

  area={ 
    getfun=getarealist,
    default_sel={"name"}
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
    default_sel={"room.vnum","dir","(toroom and toroom.vnum) as dest"}
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
          "{%s[%s] %-40s{x\n\r",
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
            "{%s[%d] %-40s{x\n\r",
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
            "{%s[%d] %-40s{x\n\r",
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
            "{%s[%d] %-40s{x\n\r",
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
