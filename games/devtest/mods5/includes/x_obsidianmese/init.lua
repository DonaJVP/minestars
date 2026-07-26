--[[
    X Obsidianmese. Adds obsidian and mese tools and items.
    Copyright (C) 2024 SaKeL

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
--]]

minetest = minetest.global_exists('minetest') and minetest --[[@as Minetest]]
ItemStack = minetest.global_exists('ItemStack') and ItemStack --[[@as ItemStack]]
vector = minetest.global_exists('vector') and vector --[[@as Vector]]
default = minetest.global_exists('default') and default --[[@as MtgDefault]]
creative = minetest.global_exists('creative') and creative --[[@as MtgCreative]]
farming = minetest.global_exists('farming') and farming --[[@as MtgFarming]]

local mod_start_time = minetest.get_us_time()
local path = minetest.get_modpath('x_obsidianmese')

dofile(path .. '/api.lua')
dofile(path .. '/tools.lua')
dofile(path .. '/nodes.lua')

if x_obsidianmese.settings.x_obsidianmese_chest then
    dofile(path .. '/obsidianmese_chest.lua')
end

dofile(path .. '/crafting.lua')

if x_obsidianmese.mod.ethereal then
    dofile(path .. '/mods/ethereal/init.lua')
end

local mod_end_time = (minetest.get_us_time() - mod_start_time) / 1000000

print('[Mod] x_obsidianmese loaded.. [' .. mod_end_time .. 's]')
