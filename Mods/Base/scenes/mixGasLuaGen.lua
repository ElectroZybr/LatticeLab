dofile("Mods/Base/API/base.lua")

simulation {
    world {
        name = "gas_mix",
        size = { 100, 100, 100 },

        content = {
            load_molecules {
                path = "Mods/Base/Molecules",
            },
            random_fill {
                density = 0.01,
                region = box {
                    size = fullworld - 4,
                    center = center,
                },
                composition = {
                    { name = molecule.h2o, fraction = 0.5 },
                    { name = molecule.h2,  fraction = 0.5 },
                }
            }
        }
    }
}
