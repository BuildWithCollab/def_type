target("def_type")
    set_kind("headeronly")
    add_headerfiles("include/def_type/**.hpp")
    add_includedirs("include", { public = true })
    add_packages("fmt", { public = true })
    add_packages("nlohmann_json", { public = true })
    add_packages("unordered_dense", { public = true })
    add_packages("magic_enum", { public = true })
    add_packages("nameof", { public = true })

    if get_config("enable_pfr") then
        add_packages("boost_pfr", { public = true })
        add_defines("DEF_TYPE_HAS_PFR", { public = true })
    end

if get_config("build_tests") then
    target("tests-def_type")
        set_kind("binary")
        add_files("tests/**.cpp")
        add_deps("def_type")
        add_packages("catch2")
        set_rundir("$(projectdir)")
end
