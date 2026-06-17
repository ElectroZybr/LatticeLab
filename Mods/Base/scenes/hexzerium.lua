dofile("Mods/Base/API/base.lua")
dofile("Mods/Base/Generators/lattice.lua")


simulation {
    world {
        name = "hexzerium",
        size = { 100, 100, 100 },

        content = {
            
            lattice {
                structure = "hex",
                region = sphere {
                    center = center,
                    radius = 25
                },
                composition = {
                    { name = atom.Z, fraction = 1.0 },
                }
            }
        }
    }
}