-- Configure:
--   VS 2026 (default):  xmake f --qt=C:/qt/6.10.2/msvc2022_64 -m release -p windows -a x64 -c -y
--   VS 2022 (no PFR):   xmake f --enable_pfr=n --toolchain=msvc --vs=2022 -m release -p windows -a x64 -c -y
--   WASM:                xmake f -p wasm -c -y && xmake build -a && xmake run wasm-tests
--   Linux / Mac:         xmake f -m release -c -y

add_rules("mode.release")
set_defaultmode("release")

set_languages("c++23")

includes("xmake/collab.lua")

if is_plat("windows") then
    add_cxxflags("/utf-8", { public = true })
end

add_requires("fmt")
add_requires("unordered_dense")
add_requires("magic_enum")
add_requires("nameof")
add_requires("nlohmann_json")

option("build_tests")
    set_default(true)
    set_showmenu(true)
    set_description("Build test targets")
option_end()

option("enable_pfr")
    set_default(true)
    set_showmenu(true)
    set_description("Enable PFR backend for automatic reflection")
option_end()

if get_config("enable_pfr") then
    add_requires("pfr_non_boost")
end

if get_config("build_tests") then
    add_requires("catch2")
end

includes("lib/def_type/xmake.lua")

-- 🌐 WASM test runner (node-based)
-- Usage: xmake run wasm-tests
if get_config("build_tests") then
    target("wasm-tests")
        set_kind("phony")
        set_default(false)
        add_deps("tests-def_type")
        on_run(function ()
            local testbin = path.join(os.projectdir(), "build", "wasm", "wasm32", "release", "tests-def_type.js")
            os.execv("node", {testbin})
        end)
end
