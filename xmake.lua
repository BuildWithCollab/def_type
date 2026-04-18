-- Configure:
--   VS 2026 (default):  xmake f --qt=C:/qt/6.10.2/msvc2022_64 -m release -p windows -a x64 -c -y
--   VS 2022 (no PFR):   xmake f --enable_pfr=n --toolchain=msvc --vs=2022 -m release -p windows -a x64 -c -y
--   Linux / Mac:         xmake f -m release -c -y

add_rules("mode.release")
set_defaultmode("release")

set_languages("c++23")
set_policy("build.c++.modules", true)

add_repositories("BuildWithCollab https://github.com/BuildWithCollab/Packages.git")

if is_plat("windows") then
    add_cxxflags("/utf-8", { public = true })
end

add_requires("fmt")
add_requires("unordered_dense")
-- magic_enum v0.9.7 has a static_str→string_view implicit conversion
-- that MSVC rejects inside C++ module partitions (commit 7d87efb)
add_requires("magic_enum master")
add_requires("nameof-module-support")

-- nlohmann_json fork with C++20 module support + MSVC friend decl fix
-- (fixes nlohmann/json#4529 — :: prefix in friend declarations breaks modules)
add_requires("nlohmann_json-module-support")

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

includes("lib/type_def/xmake.lua")
