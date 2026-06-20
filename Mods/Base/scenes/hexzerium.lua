dofile("Mods/Base/API/base.lua")

simulation {
    world {
        name = "hexzerium",
        size = { 100, 100, 100 },

        content = {
            lattice_fill {
                structure = "hex",
                region = sphere {
                    center = center,
                    radius = 25
                },
                margin = 0.0,
                composition = {
                    { name = atom.Z, fraction = 1.0 },
                }
            }
        }
    }
}
