---@diagnostic disable: undefined-global

dofile("Mods/Base/API/base.lua")
dofile("Mods/Base/Generators/gas.lua")
dofile("Mods/Base/Generators/lattice.lua")


simulation {
    world {
        name = "gas_mix",
        size = { 100, 100, 100 },

        content = {
            lattice {
                structure = "hex",
                size = fullworld - 10,
                pos = center,
                composition = {
                    { name = atom.Z, fraction = 0.5 },
                    { name = atom.C, fraction = 0.5 },
                }
            }
        }
    }
}
