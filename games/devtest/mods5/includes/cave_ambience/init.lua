minetest.register_on_joinplayer(function (player)
    local meta = player:get_meta()
    meta:set_int("mood", 0)
end)

local function handle_player_moods()
    for _, player in ipairs(minetest.get_connected_players()) do
        local meta = player:get_meta()
        local name = player:get_player_name()
        local pos = vector.offset(player:get_pos(), 0, 1, 0)
        local mood = meta:get_int("mood")
        local maximum_node_light = minetest.get_node_light(pos, 0.5)

        if maximum_node_light and maximum_node_light < 15 then
            -- Player is underground
            local light = minetest.get_node_light(pos)

            if light and light <= 6 then
                mood = mood + (7 - light)
            else
                mood = mood - 2
            end
        else
            mood = mood - 2
        end

        if mood > 100 then
            minetest.sound_play({to_player = name, name = "cave"})
            mood = 0
        
        elseif mood < 0 then
            mood = 0
        end

        meta:set_int("mood", mood)
    end

    minetest.after(1, handle_player_moods)
end

minetest.after(1, handle_player_moods)
