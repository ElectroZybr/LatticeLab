local gas = dofile("Mods/Base/Generators/gas.lua")
local loaded_count = scene:load_molecules("Mods/Base/Molecules")
assert(loaded_count > 0, "no molecules were loaded from Mods/Base/Molecules")

gas.build(scene, { name = "h2o", count = 1000 })

return gas
