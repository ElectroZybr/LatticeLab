dofile("Mods/Base/API/base.lua")

simulation {
    world {
        name = "gas_mix",
        size = { 100, 100, 100 },

        content = {
            lattice_fill {
                structure = "bcc",
                region = sphere {
                    center = center,
                    radius = 10
                },
                margin = 0.0,
                composition = {
                    { name = atom.Z, fraction = 1.0 },
                }
            }
        }
    }
}
