
-- there are probably 2 dirt recipes that are implied by name
-- one is "dirt_with_X" and the other is "X_dirt"
-- we should be able to automatically assemble recipes for these dirts

local all_nodes = minetest.registered_nodes

local function make_dirt_recipe(dirt_name, other_ingredient)
  
  -- minetest.log(dirt_name .. " can now be made with " .. other_ingredient)
  
  -- NOTE: I am using a shaped recipe because I don't want to overwrite some
  -- of the known shapeless recipes from ethereal, for example
  minetest.register_craft({
    output = dirt_name,
    recipe = {
      {other_ingredient},
      {"default:dirt"}
    }
  })
end

local function make_permafrost_recipe(permafrost_name, other_ingredient)
  minetest.register_craft({
    output = permafrost_name,
    recipe = {
      {other_ingredient},
      {"default:permafrost"}
    }
  })
end

-- this function attempts to figure out a reasonable recipe for dirt based on
-- the provided mod_name and other_ingredient.  this relies on some standard
-- suffixes to invoke leaves, moss, grass, etc.
local function sloppy_dirt(mod_name, dirt_name, other_ingredient)
  
  local suffixes = {
    "",
    "_1","_2","_3","_4","_5",
    "_leaves", "_moss",
    "leaves","grass"
  }
  
  -- the provided mod_name _should_ be sufficient, but sometimes we also need
  -- to use the default prefix
  local mod_choices = {
    "default",
    mod_name
  }
  
  for i=1, #mod_choices do
    for n=1, #suffixes do
      local this_ingredient = mod_choices[i] .. ":" .. other_ingredient .. suffixes[n]
      
      -- only register the craft recipe if the other ingredient really exists
      if all_nodes[this_ingredient] ~= nil then
        make_dirt_recipe(dirt_name, this_ingredient)
      end
    end
  end
end

-- look through all available registered nodes to find dirt blocks and then try
-- to figure out the other ingredient for the dirt based on the name
for key, value in pairs(all_nodes) do
  if key:find("dirt",1, true)
  and not key:find("stair",1,true)
  and not key:find("light",1,true) then
    local mod_name = string.gsub(key,":.*$","")
    
    if key:find("dirt_with",1, true) then
      local other_ingredient = key:gsub("^.*with_","")
      sloppy_dirt(mod_name, key, other_ingredient)
    end
    
    if key:find("_dirt",1,true) then
      local other_ingredient = key:gsub("^.*:(.*)_dirt$","%1")
      sloppy_dirt(mod_name, key, other_ingredient)
    end
  end
end


-- these are a couple of special kinds of dirt that don't fit the automatic
-- or sloppy process
make_dirt_recipe("default:dirt_with_rainforest_litter","default:jungleleaves")
make_dirt_recipe("default:dirt_with_rainforest_litter","default:junglesapling")
make_dirt_recipe("default:dirt_with_coniferous_litter","default:pine_needles")
make_dirt_recipe("default:dirt_with_coniferous_litter","default:pine_sapling")

sloppy_dirt("ethereal","ethereal:prairie_dirt","orange")
sloppy_dirt("ethereal","ethereal:grove_dirt","banana")
sloppy_dirt("ethereal","ethereal:grove_dirt","olive")
sloppy_dirt("ethereal","ethereal:grove_dirt","lemon")

make_permafrost_recipe("default:permafrost_with_moss","default:mossycobble")
make_permafrost_recipe("default:permafrost_with_stones","default:gravel")

-- dirt conversion recipes
minetest.register_craft({
  output = "default:dirt",
  recipe = {
    {"bucket:bucket_water"},
    {"default:dry_dirt"}

  },
  replacements = { {"bucket:bucket_water", "bucket:bucket_empty"}, }
})

minetest.register_craft({
    type = "cooking",
    output = "default:dry_dirt",
    recipe = "default:dirt",
	cooktime = 2,
})
