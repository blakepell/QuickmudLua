-- Lua startup file (MUD-wide) for SMAUG Fuss 
--   Written by Nick Gammon
--   16th July 2007

--     www.gammon.com.au


-- -----------------------------------------------------------------------------
--         HELPER FUNCTIONS
-- -----------------------------------------------------------------------------

local P={}
setmetatable(P, {__index = _G})
setfenv(1, P)

-- trim leading and trailing spaces from a string
function trim (s)
  return (string.gsub (s, "^%s*(.-)%s*$", "%1"))
end -- trim

-- round "up" to absolute value, so we treat negative differently
--  that is, round (-1.5) will return -2
local function round (x)
  if x >= 0 then
    return math.floor (x + 0.5)
  end  -- if positive

  return math.ceil (x - 0.5)
end -- function round

local function convert_time_helper (secs)

  -- handle negative numbers
  local sign = ""
  if secs < 0 then
    secs = math.abs (secs)
    sign = "-"
  end -- if negative seconds
  
  -- weeks
  if secs >= (60 * 60 * 24 * 6.5) then
    return sign .. round (secs / (60 * 60 * 24 * 7)), "w"
  end -- 6.5 or more days
  
  -- days
  if secs >= (60 * 60 * 23.5) then
    return sign .. round (secs / (60 * 60 * 24)), "d"
  end -- 23.5 or more hours
  
  -- hours
  if secs >= (60 * 59.5) then
   return sign .. round (secs / (60 * 60)), "h"
  end -- 59.5 or more minutes
  
  -- minutes
  if secs >= 59.5 then
   return sign .. round (secs / 60), "m"
  end -- 59.5 or more seconds
  
  -- seconds
  return sign .. round (secs), "s"    
end -- function convert_time_helper

local convert_time_long
-- eg. 4m
function convert_time (secs, long)
  if long==true then
    return convert_time_long(secs)
  end
  local n, u = convert_time_helper (secs)
  return n .. " " .. u 
end -- function convert_time

time_units = {
  w = "week",
  d = "day",
  h = "hour",
  m = "minute",
  s = "second"
  }
  
-- eg. 4 minutes
convert_time_long = function (secs)
  local n, u = convert_time_helper (secs)
  local long_units = time_units [u]
  local s = ""
  if math.abs (n) ~= 1 then
    s = "s"
  end -- if not 1
  return n .. " " .. long_units .. s
end -- function convert_time_long

function capitalize (s)
  return string.upper (string.sub (s, 1, 1)) .. string.sub (s, 2)
end -- capitalize 

-- Pluralize

-- Standard rules.
local rule1 = 
{
    ['Note'] = 'Singular noun ending in a sibilant, needing es.',
    ['EndsWith'] = { 's', 'sh', 'tch', 'se', 'ge', 'x' },
    ['Plural'] = 'es',
    ['Drop'] = 'e'
}

local rule2 =
{
    ['Note'] = 'Nouns ending in o preceded by a consonant, need es. Exceptions apply.',
    ['EndsWith'] = { '[^aeiou]o' }, 
    ['Plural'] = 'es',
    ['Drop'] = ''
}

local rule3 =
{
    ['Note'] = 'Nouns ending in y preceded by a consonant or quy replace y with ies.',
    ['EndsWith'] = { '[^aeiou]y', 'quy' },
    ['Plural'] = 'ies',
    ['Drop'] = 'y'
}

local ruleSet = { rule1, rule2, rule3 }

-- Produces a string with proper pluralization. Doesn't account
-- for a lot of exceptional words, so an optional override can
-- be provided.
-- (int) count: The number of items to determine if plural or not.
-- (string) noun: The item word to pluralize.
-- [string] override: Optional override word to use for plurals.
function pluralize(count, noun, override)
    if count == 1 then
        return count .. " " .. noun
    else
        -- Done if we have an override.
        if not (override == nil) then
            return string.format("%d %s", count, override)
        end

        -- Check against rules.
        for _,ruleSet in ipairs(ruleSet) do
            for _,endsWith in ipairs(ruleSet.EndsWith) do
                if not (string.match(noun, endsWith .. "$") == nil) then
                    return string.format("%d %s%s", count, string.gsub(noun, ruleSet.Drop .. "$", ''), ruleSet.Plural)
                end
            end
        end

        -- If we get here we didn't match and existing rule. Just add s.
        return string.format("%d %ss", count, noun)
    end
end

local function unpackArgs(items)
  if items == nil then
    return {}
  end
  local result = {}
  for i,v in pairs(items) do
    if type(v) == "table" then
      local unpacked = unpackArgs(v)
      for i2,v2 in pairs(unpacked) do
        table.insert(result, v2)
      end
    elseif not (v == nil) then
      table.insert(result, v)
    end
  end

  return result
end

function format_list(separator, ...)
  arg.n = nil
  items = unpackArgs(arg)
  string = table.concat(items, ", " .. separator .. " ")
  if #items > 2 then
    return string.gsub(string, ", "..separator .. " ", ", ", #items-2)
  elseif #items == 2 then
    return string.gsub(string, ", ", " ", #items)
  else
    return string
  end
end



return P
