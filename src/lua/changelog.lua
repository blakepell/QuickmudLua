changelog_table = changelog_table or {}
local PAGE_SIZE=30
local disable_save=false

local function add_change( chg )
    --table.insert( changelog_table, chg )
    --table.sort( changelog_table, function(a,b) return a.date<b.date end)
    -- We don't want to insert then sort to prevent reordering of entries 
    -- with identical dates
    local ind 
    for i=1,#changelog_table do
        if changelog_table[i].date > chg.date then
            ind=i
            break
        end
    end

    if ind then
        table.insert( changelog_table, ind, chg )
    else
        table.insert( changelog_table, chg ) 
    end
        
end
local function handle_changelog_con( d )
    local change={}
    local stat
    local cmd=""

    local funcs={
        getdate=function( self, d, cmd )
            if not(stat=="getdate") then
                stat="getdate"
                -- Set the default date
                change.date=os.time()
                sendtochar(d.character,
                        ("Enter date: [%s]"):format( os.date("%Y/%m/%d")))
                return
            end

            if not(cmd=="") then
                local Y,M,D=cmd:match("(%d%d%d%d)/(%d%d)/(%d%d)")

                if not Y then
                    sendtochar(d.character, "Date format is: YYYY/MM/DD\n\r")
                    return
                end
                change.date=os.time{year=Y, month=M, day=D}
            end

            return self:getauthor( d, "" )
        end,
        getauthor=function( self, d, cmd )
            if not(stat=="getauthor") then
                stat="getauthor"
                -- Set the default author
                change.author=d.character.name
                sendtochar( d.character,
                        ("Enter author name(s) delimited by forward slash, e.g Bill/Bob: [%s]"):format( change.author))
                return
            end

            if not(cmd=="") then
                change.author=cmd
            end

            return self:getdesc( d, "" )
        end,
        getdesc=function( self, d, cmd )
            if not(stat=="getdesc") then
                stat="getdesc"
                sendtochar(d.character, "Enter the change description:\n\r")
                return
            end

            change.desc=cmd
            return self:confirm( d, "" )
        end,
        confirm=function( self, d, cmd )
            if not(stat=="confirm") then
                stat="confirm"
                -- Show the summary
                sendtochar(d.character, ([[
Timestamp:
%s

Author:
%s

Description:
%s

Save change? [Y/n]
]]):format( os.date("%Y/%m/%d %H:%M", change.date), change.author, change.desc))
                return
            end

            if cmd=="n" then
                sendtochar(d.character, "Change aborted!\n\r")
                return true
            elseif cmd=="Y" then
                add_change(change)
                sendtochar(d.character, "Change saved!\n\r")
                return true
            else
                sendtochar(d.character, "Save change? [Y/n]\n\r")
                return
            end
        end
    }

    funcs:getdate( d, "")
    while true do
        cmd=coroutine.yield()
        --sendtochar( d.character, "You entered: "..cmd.."\n\r")

        if funcs[stat](funcs, d, cmd) then break end
    end

    return
end

local function getlines(text, length)
    local len=text:len()
    if len<=length then
        return { text }
    end
    local lines={}

    local startind=1
    local endind
    
    repeat
        while text:sub(startind, startind)==" " do
            startind=startind+1
        end
        
        if startind>len then break end

        endind=math.min(len, startind-1+length)

        while endind>startind 
        and not(endind>=len)
        and not(text:sub(endind+1,endind+1) == " ") do
            endind=endind-1
        end

        table.insert(lines, text:sub(startind,endind))
        startind=endind+1
    until startind>len

    return lines

end

local function getindexcolor( index )
    if index%2 == 0 then
        return "{+"
    else
        return "{x"
    end
end

local function show_change_entry( ch, i, color)
    local ent=changelog_table[i]
    local indent=29
    local textwidth=80-indent
    local desc=ent.desc
    local len=desc:len()

    local lines=getlines(desc, textwidth)


    for lineInd,v in ipairs(lines) do
        if lineInd==1 then
            sendtochar( ch, ("%s%5d. %s %-10s "):format(
                    color,
                    i,
                    os.date("%Y/%m/%d", ent.date),
                    ent.author))

        else
            sendtochar( ch, (" "):rep(indent))
        end

        sendtochar(ch, v.."\n\r")
    end
    sendtochar(ch, "{x")
end

local function show_change_page( ch, pagenum, indices )
    local start,fin
    local lastind=indices and #indices or #changelog_table

    start = 1 + ((pagenum-1) * PAGE_SIZE)
    fin = math.min(start-1+PAGE_SIZE, lastind)

    if indices then
    -- Only show some indices
        for i=start,fin do
            show_change_entry( ch, indices[i], getindexcolor(i))
        end
    else
        for i=start,fin do
            show_change_entry( ch, i, getindexcolor(i))
        end
    end
                
end

local function changelog_browse_con( d, indices )
    local cmd
    local pagenum=1
    local lastpage=math.ceil((indices and #indices or #changelog_table)/PAGE_SIZE)
    local show=true

    while true do
        if show then
            sendtochar( d.character, "Page "..pagenum.."/"..lastpage.."\n\r")
            show_change_page( d.character, pagenum, indices )  
        end
        show=true
        sendtochar( d.character,
            "[q]uit, [n]ext, [p]rev, [f]irst, [l]ast, or #\n\r")
        
        cmd=coroutine.yield()
        
        if cmd=="q" then
            -- quit
            return
        elseif cmd=="n" then
            -- next page
            local newnum=pagenum+1
            if newnum<1 or newnum>lastpage then
                sendtochar(d.character, "Already at last page.\n\r")
                show=false
            else
                pagenum=newnum
            end
        elseif cmd=="p" then
            -- previous page
            if pagenum<2 then
                sendtochar(d.character, "Already at first page.\n\r")
                show=false
            else
                pagenum=pagenum-1
            end
        elseif cmd=="f" then
            -- first page
            if pagenum==1 then
                sendtochar(d.character, "Already at first page.\n\r")
                show=false
            else
                pagenum=1
            end
        elseif cmd=="l" then
            -- last page
            if pagenum==lastpage then
                sendtochar(d.character, "Already at last page.\n\r")
                show=false
            else
                pagenum=lastpage
            end
        elseif tonumber(cmd) then
            local newnum=tonumber(cmd)
            if newnum<1 or newnum>lastpage then
                sendtochar(d.character, "No such page.\n\r")
                show=false
            elseif pagenum==newnum then
                sendtochar(d.character, "Already at page "..newnum..".\n\r")
                show=false
            else
                pagenum=newnum
            end
        end
    end
end    

local function changelog_remove_con( d, ind )
    local cmd
    local ent=changelog_table[ind]
    if not ent then
        sendtochar( d.character, "No such entry.\n\r")
        return
    end

    show_change_entry( d.character, ind, getindexcolor(ind))
    sendtochar( d.character, "Delete this entry? 'Y' to confirm.\n\r")

    cmd=coroutine.yield()

    if not(cmd=="Y") then
        sendtochar( d.character, "Cancelled. Change not removed.\n\r")
        return
    end

    -- Make sure indexes haven't shifted since the confirmation
    if not changelog_table[ind]==ent then
        sendtochar( d.character, "Indexes have shifted, please try again.\n\r")
        return
    end

    table.remove(changelog_table, ind)
    sendtochar( d.character, "Change removed.\n\r")
end

local function changelog_usage( ch )
    sendtochar( ch, [[
changelog show              -- Show 30 most recent changes.
changelog show [page]       -- Show a specific page of changes.
changelog browse            -- Browse changes page by page.
changelog find [text]       -- Show all entries that contain the given text.
changelog pattern [pattern] -- Show all entries that match the given pattern 
                               (lua pattern matching).
changelog author [name]     -- Show all entries from the given author.
changelog authors           -- List all changelog authors.
]])
    if ch.level>=108 then
        sendtochar( ch, [[

changelog add             -- Add a change.
changelog remove [#index] -- Remove a change
]])
    end
end

function do_changelog( ch, argument )
    if not ch.ispc then return end

    local args=arguments(argument)
    
    if args[1]=="show" then
        local ttl=#changelog_table
        if ttl<1 then return end
        local page=tonumber(args[2])
        local start
        local fin
        if page then
            local lastpage=math.ceil(ttl/PAGE_SIZE)

            if page<1 or page>lastpage then
                sendtochar(ch, "Invalid page number.\n\r")
                return
            end
            
            sendtochar( ch, "Page "..page.."/"..lastpage.."\n\r") 
            show_change_page( ch, page)
            return
        else
            fin=ttl
            start=math.max(1,fin-PAGE_SIZE+1)
        end

        for i=start,fin do
            show_change_entry( ch, i, getindexcolor(i))
        end
        return

    elseif args[1]=="browse" then
        start_con_handler( ch.descriptor, changelog_browse_con, ch.descriptor)
        return 
    elseif args[1]=="find" then
        -- find argument comprises the rest of the argument
        local text=argument:sub(("find "):len()+1)
        local result_indices={}
        for k,v in pairs(changelog_table) do
            if string.find(v.desc:lower(), text:lower(), 1, true) then
                table.insert(result_indices, k)
            end
        end
        
        if #result_indices<1 then
            sendtochar(ch, "No results found.\n\r")
            return
        end
        start_con_handler( ch.descriptor, 
                changelog_browse_con, 
                ch.descriptor, 
                result_indices)
        return
    elseif args[1]=="pattern" then
        local text=argument:sub(("pattern "):len()+1)
        local result_indices={}
        for k,v in pairs(changelog_table) do
            if string.find(v.desc, text) then
                table.insert(result_indices, k)
            end
        end

        if #result_indices<1 then
            sendtochar(ch, "No results found.\n\r")
            return
        end
        start_con_handler( ch.descriptor, 
                changelog_browse_con, 
                ch.descriptor, 
                result_indices)
        return
    elseif args[1]=="author" then
        local target=args[2]:lower()
        if not target then
            sendtochar(ch, "You have to provide an author name!\n\r")
            return
        end
        local result_indices={}
        for k,v in pairs(changelog_table) do
            for auth in string.gmatch(v.author, "%a+") do
                if auth:lower()==target then
                    table.insert(result_indices, k)
                    break -- on the off chance the same author is listed twice on the same entry
                end
            end
        end

        if #result_indices<1 then
            sendtochar(ch, "No results found.\n\r")
            return
        end

        start_con_handler( ch.descriptor, 
                changelog_browse_con, 
                ch.descriptor, 
                result_indices)
        return 
    elseif args[1]=="authors" then
        local authors={}
        for k,v in pairs(changelog_table) do
            for auth in string.gmatch(v.author, "[^/]+") do
                authors[auth]=true
            end
        end

        local array={}
        for k,v in pairs(authors) do
            table.insert(array, k)
        end

        table.sort(array)

        local out={}
        for i,v in ipairs(array) do
            table.insert(out, string.format("%3d. %s\n\r", i, v))
        end

        pagetochar( ch, table.concat(out))

        return
    elseif args[1]=="remove" then
        local num=args[2] and tonumber(args[2])
        if not num then
            changelog_usage( ch )
            return
        end

        start_con_handler( ch.descriptor, changelog_remove_con, ch.descriptor, num) 
        return
    end

    if ch.level>=108 then
        if args[1]=="add" then
            local d=ch.descriptor
            start_con_handler( d, handle_changelog_con, d)
            return
        end

        if args[1]=="remove" then
            sendtochar( ch, "Not implemented yet.\n\r")
            return
        end 

        --[[ temporary for debug stuff
        if args[1]=="nosave" then
            disable_save=not(disable_save)
            sendtochar( ch, "Saving disabled: "..tostring(disable_save).."\n\r")
            return
        end

        if args[1]=="reload" then
            load_changelog()
            sendtochar( ch, "Changelog force reloaded from file.")
            return
        end
        -- end debug --]]
    end

    changelog_usage(ch)

end

function load_changelog()
    changelog_table={}
    local f=loadfile("changelog_table.lua")

    if f==nil then return end

    local tmp=f()
    if tmp then changelog_table=tmp end
end


function save_changelog()
    if disable_save then return end

    local f=io.open("changelog_table.lua", "w")
    out,saved=serialize.save("changelog_table", changelog_table)
    f:write(out)

    f:close()
end
