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
                structure = "bcc",

                region = sphere {
                    center = center,
                    radius = 10
                }
                composition = {
                    { name = atom.Z, fraction = 1.0 },
                }
            }
        }
    }
}
