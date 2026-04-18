target("collab.core")
    set_kind("static")
    add_files("src/**.cpp")
    add_files("src/**.cppm", { public = true })
    add_packages("fmt", { public = true })
    add_packages("nlohmann_json-module-support", { public = true })
    add_packages("unordered_dense", { public = true })
    add_packages("magic_enum", { public = true })
    add_packages("nameof-module-support", { public = true })

    if get_config("enable_pfr") then
        add_packages("pfr_non_boost", { public = true })
        add_defines("COLLAB_FIELD_HAS_PFR", { public = true })
    end

if get_config("build_tests") then
    target("tests-collab.core")
        set_kind("binary")
        add_files("tests/**.cpp")
        add_deps("collab.core")
        add_packages("catch2")
end
