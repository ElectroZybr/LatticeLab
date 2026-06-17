dofile("Mods/Base/API/base.lua")
dofile("Mods/Base/Generators/gas.lua")

simulation {
    world {
        name = "gas_mix",
        size = { 100, 100, 100 },

        content = {
            gas {
                temperature = 300,
                composition = {
                    { name = molecule.h2o, count = 1000 },
                    { name = molecule.h2,  count = 1000 },
                }
            }
        }
    }
}
