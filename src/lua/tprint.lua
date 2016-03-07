--
--  tprint.lua

--[[

For debugging what tables have in them, prints recursively

See forum thread:  http://www.gammon.com.au/forum/?id=4903

eg.

require "tprint"

tprint (some_table)

--]]

function tprint ( t)
  local function itprint ( resulttbl, t, indent, done)
    -- show strings differently to distinguish them from numbers
    local function show (val)
      if type (val) == "string" then
        return '"' .. val .. '"'
      else
        return tostring (val)
      end -- if
    end -- show
    if type (t) ~= "table" then
      table.insert( resulttbl, "tprint got " .. type (t) )
      table.insert( resulttbl, "\n" )
      return
    end -- not table
    -- entry point here
    done = done or {}
    indent = indent or 0
    for key, value in pairs (t) do
      table.insert( resulttbl, (string.rep (" ", indent)) ) -- indent it
      if type (value) == "table" and not done [value] then
        done [value] = true
        table.insert( resulttbl,  table.concat({show (key), ":"}) )
        table.insert( resulttbl, "\n" )
        tprint (resulttbl, value, indent + 2, done)
      else
        table.insert( resulttbl, table.concat({show (key), "="}) )
        table.insert( resulttbl, (show (value)) )
        table.insert( resulttbl, "\n" )
      end
    end
  end

  local resulttbl={}
  itprint( resulttbl, t, indent, done)
  return table.concat(resulttbl)
end

return tprint

