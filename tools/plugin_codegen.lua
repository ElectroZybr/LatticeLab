rule("lattice.plugin_codegen")
    on_load(function (target)
        local output = path.join(target:autogendir(), "plugin_register.cpp")
        target:data_set("lattice.plugin_codegen.output", output)
        target:add("files", output, {always_added = true})
    end)
    before_build(function (target)
        import("core.tool.compiler")
        import("core.base.json")
        local output = target:data("lattice.plugin_codegen.output")
        local flags = compiler.load("cxx", {target = target}):compflags({target = target})
        local flagsfile = output .. ".flags.json"
        os.mkdir(path.directory(output))
        io.writefile(flagsfile, json.encode(flags))
        os.vrunv("python3", {path.join(os.projectdir(), "tools/generate_plugin.py"),
            "--plugin", target:scriptdir(), "--output", output,
            "--flags-json", flagsfile, "--legacy"})
    end)
rule_end()
