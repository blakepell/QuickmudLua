package.path = mud.luadir() .. "?.lua"

glob_tprintstr=require "tprint"

require "commands"
glob_util=require "utilities"

envtbl={} -- game object script environments
interptbl={} -- key is game object pointer, table of desc=desc pointer, name=char name
delaytbl={} -- used on the C side mostly

cleanup={}
validuds={}

function UdCnt()
    local reg=debug.getregistry()

    local cnt=0
    for k,v in pairs(reg) do
        if type(v)=="userdata" then
            cnt=cnt+1
        end
    end

    return cnt
end

function EnvCnt()
    local cnt=0
    for k,v in pairs(envtbl) do
        cnt=cnt+1
    end
    return cnt
end

function UnregisterDesc(desc)
    for k,v in pairs(interptbl) do
        if v.desc==desc then
            interptbl[k]=nil
        end
    end
end

function glob_rand(pcnt)
    return ( (mt.rand()*100) < pcnt)
end

function glob_randnum(low, high)
    return math.floor( (mt.rand()*(high+1-low) + low)) -- people usually want inclusive
end

function linenumber( text )
    local cnt=1
    local rtn={}

    for line in text:gmatch(".-\n\r?") do
        table.insert(rtn, string.format("%3d. %s", cnt, line))
        cnt=cnt+1
    end
            
    return table.concat(rtn)
end

-- Standard functionality available for any env type
-- doesn't require access to env variables
function MakeLibProxy(tbl)
    local mt={
        __index=tbl,
        __newindex=function(t,k,v)
            error("Cannot alter library functions.")
            end,
        __metatable=0 -- any value here protects it
    }
    local proxy={}
    setmetatable(proxy, mt)
    return proxy
end

main_lib={  require=require,
		assert=assert,
		error=error,
		ipairs=ipairs,
		next=next,
		pairs=pairs,
		--pcall=pcall, -- remove so can't bypass infinite loop check
        print=print,
        select=select,
		tonumber=tonumber,
		tostring=tostring,
		type=type,
		unpack=unpack,
		_VERSION=_VERSION,
		--xpcall=xpcall, -- remove so can't bypass infinite loop check
		coroutine={create=coroutine.create,
					resume=coroutine.resume,
					running=coroutine.running,
					status=coroutine.status,
					wrap=coroutine.wrap,
					yield=coroutine.yield},
		string= {byte=string.byte,
				char=string.char,
				find=string.find,
				format=string.format,
				gmatch=string.gmatch,
				gsub=string.gsub,
				len=string.len,
				lower=string.lower,
				match=string.match,
				rep=string.rep,
				reverse=string.reverse,
				sub=string.sub,
				upper=string.upper},
				
		table={insert=table.insert,
				maxn=table.maxn,
				remove=table.remove,
				sort=table.sort,
				getn=table.getn,
				concat=table.concat},
				
		math={abs=math.abs,
				acos=math.acos,
				asin=math.asin,
				atan=math.atan,
				atan2=math.atan2,
				ceil=math.ceil,
				cos=math.cos,
				cosh=math.cosh,
				deg=math.deg,
				exp=math.exp,
				floor=math.floor,
				fmod=math.fmod,
				frexp=math.frexp,
				huge=math.huge,
				ldexp=math.ldexp,
				log=math.log,
				log10=math.log10,
				max=math.max,
				min=math.min,
				modf=math.modf,
				pi=math.pi,
				pow=math.pow,
				rad=math.rad,
				random=math.random,
				sin=math.sin,
				sinh=math.sinh,
				sqrt=math.sqrt,
				tan=math.tan,
				tanh=math.tanh},
		os={time=os.time,
            date=os.date,
			clock=os.clock,
			difftime=os.difftime},
        -- this is safe because we protected the game object and main lib
        --  metatables.
		setmetatable=setmetatable,
        --getmetatable=getmetatable
}

-- add script_globs to main_lib
for k,v in pairs(script_globs) do
    print("Adding "..k.." to main_lib.")
    if type(v)=="table" then
        for j,w in pairs(v) do
            print(j)
        end
    end
    main_lib[k]=v
end

-- Need to protect our library funcs from evil scripters
function ProtectLib(lib)
    for k,v in pairs(lib) do
        if type(v) == "table" then
            ProtectLib(v) -- recursion in case we add some nested libs
            lib[k]=MakeLibProxy(v)
        end
    end
    return MakeLibProxy(lib)
end

-- Before we protect it, we want to make a lit of names for syntax highligting
main_lib_names={}
for k,v in pairs(main_lib) do
    if type(v) == "function" then
        table.insert(main_lib_names, k)
    elseif type(v) == "table" then
        for l,w in pairs(v) do
            table.insert(main_lib_names, k.."."..l)
        end
    end
end
main_lib=ProtectLib(main_lib)


-- First look for main_lib funcs, then mob/area/obj funcs
env_meta={
    __index=function(tbl,key)
        if main_lib[key] then
            return main_lib[key]
        elseif type(tbl.self[key])=="function" then 
            return function(...) 
                        table.insert(arg, 1, tbl.self)
                        return tbl.self[key](unpack(arg)) 
                   end
        end
    end
}

function new_script_env(ud, objname)
    local env=setmetatable( {}, {
            __index=function(t,k)
                if k=="self" or k==objname then
                    return ud
                else
                    return env_meta.__index(t,k)
                end
            end,
            __metatable=0 })
    return env
end

function mob_program_setup(ud, f)
    if envtbl[ud]==nil then
        envtbl[ud]=new_script_env(ud, "mob", CH_env_meta) 
    end
    setfenv(f, envtbl[ud])
    return f
end

function obj_program_setup(ud, f)
    if envtbl[ud]==nil then
        envtbl[ud]=new_script_env(ud, "obj", OBJ_env_meta)
    end
    setfenv(f, envtbl[ud])
    return f
end

function area_program_setup(ud, f)
    if envtbl[ud]==nil then
        envtbl[ud]=new_script_env(ud, "area", AREA_env_meta)
    end
    setfenv(f, envtbl[ud])
    return f
end

function room_program_setup(ud, f)
    if envtbl[ud]==nil then
        envtbl[ud]=new_script_env(ud, "room", ROOM_env_meta)
    end
    setfenv(f, envtbl[ud])
    return f
end

function interp_setup( ud, typ, desc, name)
    if interptbl[ud] then
        return 0, interptbl[ud].name
    end

    if envtbl[ud]== nil then
        if typ=="mob" then
            envtbl[ud]=new_script_env(ud,"mob", CH_env_meta)
        elseif typ=="obj" then
            envtbl[ud]=new_script_env(ud,"obj", OBJ_env_meta)
        elseif typ=="area" then
            envtbl[ud]=new_script_env(ud,"area", AREA_env_meta)
        elseif typ=="room" then
            envtbl[ud]=new_script_env(ud,"room", ROOM_env_meta)
        else
            error("Invalid type in interp_setup: "..typ)
        end
    end

    interptbl[ud]={name=name, desc=desc}
    return 1,nil
end

function run_lua_interpret(env, str )
    local f,err
    interptbl[env.self].incmpl=interptbl[env.self].incmpl or {}

    table.insert(interptbl[env.self].incmpl, str)
    f,err=loadstring(table.concat(interptbl[env.self].incmpl, "\n"))

    if not(f) then
        -- Check if incomplete, same way the real cli checks
        local ss,sf=string.find(err, "<eof>")
        if sf==err:len()-1 then
            return 1 -- incomplete
        else
           interptbl[env.self].incmpl=nil
           error(err)
        end
    end

    interptbl[env.self].incmpl=nil
    setfenv(f, env)
    f()
    return 0
end

function list_files ( path )
    local f=assert(io.popen('find ../player -type f -printf "%f\n"', 'r'))
    
    local txt=f:read("*all")
    f:close()
    
    local rtn={}
    for nm in string.gmatch( txt, "(%a+)\n") do
        table.insert(rtn, nm)
    end

    return rtn
end

function colorize( text )
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
                table.insert(rtn,"{x"..char)
            elseif waitfor==']]' 
                and waitfor==text:sub(i,i+1) 
                then
                table.insert(rtn,"]]{x")
                waitfor=nil
                i=i+1
            elseif waitfor=='--]]'
                and waitfor==text:sub(i,i+3)
                then
                table.insert(rtn,"--]]{x")
                waitfor=nil
                i=i+3
            elseif char==waitfor then
                -- ends up handling ' and "
                waitfor=nil
                table.insert(rtn, char.."{x")
            else
                -- waitfor didn't match, just push the char
                table.insert(rtn, char)
            end
        -- Literal strings
        elseif char=='"' or char=="'" then
            table.insert(rtn, "{"..('r')..char)
            waitfor=char
        -- Multiline strings
        elseif char=='[' and text:sub(i+1,i+1) == '[' then
            table.insert(rtn, "{"..('r').."[[")
            i=i+1
            waitfor=']]'
        -- Multiline comments
        elseif char=='-' and text:sub(i+1,i+3) == "-[[" then
            table.insert(rtn, "{"..('c').."--[[")
            i=i+3
            waitfor='--]]'
        -- Single line comments
        elseif char=='-' and text:sub(i+1,i+1) == '-' then
            table.insert(rtn, "{"..('c').."--")
            i=i+1
            waitfor='\n'
        elseif char=='\t' then
            table.insert(rtn, "    ")
        elseif char=='{' then
            table.insert(rtn, "{{")
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
            table.insert(rtn, "{"..('G')..char.."{x")
        -- Words
        elseif string.find(char, "%a") then
            local start,finish,word=string.find(text,"(%a[%w_%.]*)",i)
            i=finish
            if word=="function" then
                table.insert(funtrack,1,nestlevel)
                nestlevel=nestlevel+1
                table.insert(rtn, "{"..('C')..word.."{x")
            -- these two words account for do, while, if, and for
            elseif word=="do" or word=="if" then
                nestlevel=nestlevel+1
                table.insert(rtn, "{"..('Y')..word.."{x")
            elseif word=="end" then
                nestlevel=nestlevel-1
                if funtrack[1] and funtrack[1]==nestlevel then
                    table.remove(funtrack,1)
                    table.insert(rtn, "{"..('C')..word.."{x")
                else
                    table.insert(rtn, "{"..('Y')..word.."{x")
                end
            -- boolean
            elseif word=="true" or word=="false" then
                table.insert(rtn, "{"..('r')..word.."{x")
            -- 'keywords'
            elseif word=="and" or word=="in" or word=="repeat"
                or word=="break" or word=="local" or word=="return"
                or word=="for" or word=="then" or word=="else"
                or word=="not" or word=="elseif" or word=="if"
                or word=="or" or word=="until" or word=="while"
                then
                table.insert(rtn, "{"..('Y')..word.."{x")
            -- nil
            elseif word=="nil" then
                table.insert(rtn, "{"..('r')..word.."{x")
            else
                -- Search globals
                local found=false
                for k,v in pairs(main_lib_names) do
                    if word==v then
                        table.insert(rtn, "{"..('C')..word.."{x")
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
            table.insert(rtn, "{"..('m')..word.."{x")
        else
            -- Whatever else
            table.insert(rtn,char)
        end
    end

    return table.concat(rtn)

end

function start_con_handler( d, fun, ... )
    forceset(d, "constate", "lua_handler")
    forceset(d, "conhandler", coroutine.create( fun ) )

    lua_con_handler( d, unpack(arg) )
end

function lua_con_handler( d, ...)
    if not forceget(d,"conhandler") then
        error("No conhandler for "..d.character.name)
    end

    local res,err=coroutine.resume(forceget(d,"conhandler"), unpack(arg))
    if res == false then
        forceset( d, "conhandler", nil )
        forceset( d, "constate", "playing" )
        error(err)
    end

    if coroutine.status(forceget(d, "conhandler"))=="dead" then
        forceset( d, "conhandler", nil )
        forceset( d, "constate", "playing" )
    end

end
