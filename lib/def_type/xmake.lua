target("def_type")
    set_kind("headeronly")
    add_headerfiles("include/(**.hpp)")
    add_includedirs("include", { public = true })
    add_packages("fmt", { public = true })
    add_packages("nlohmann_json", { public = true })
    add_packages("unordered_dense", { public = true })
    add_packages("magic_enum", { public = true })
    add_packages("nameof", { public = true })

    if get_config("enable_pfr") then
        add_packages("pfr_non_boost", { public = true })
        add_defines("DEF_TYPE_HAS_PFR", { public = true })
    end

if get_config("build_tests") then
    target("tests-def_type")
        set_kind("binary")
        add_files("tests/**.cpp")
        add_deps("def_type")
        add_packages("catch2")
        set_rundir("$(projectdir)")

        on_load(function (target)
            if target:is_plat("wasm") then
                target:add("cxflags", "-fwasm-exceptions")
                target:add("ldflags", "-s EXIT_RUNTIME=1", "-fwasm-exceptions")
            end
        end)
end
