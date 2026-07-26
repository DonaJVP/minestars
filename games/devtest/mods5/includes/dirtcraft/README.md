# dirtcraft

[![ContentDB](https://content.minetest.net/packages/dayer4b/dirtcraft/shields/title/)](https://content.minetest.net/packages/dayer4b/dirtcraft/)
[![ContentDB](https://content.minetest.net/packages/dayer4b/dirtcraft/shields/downloads/)](https://content.minetest.net/packages/dayer4b/dirtcraft/)

This minetest mod provides recipes for crafting dirt by automatic means.  This
was specifically written with the intention to support the `ethereal` mod.

In order to avoid possible conflicts with the recipes produced, they are shaped
with the "other_ingredient" being placed above good ole "default:dirt".

```lua
minetest.register_craft({
   output = dirt_name,
   recipe = {
      {other_ingredient},
      {"default:dirt"}
   }
})
```
