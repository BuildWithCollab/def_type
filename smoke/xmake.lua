add_rules("mode.release")
set_defaultmode("release")
set_languages("c++23")

add_repositories("BuildWithCollab https://github.com/BuildWithCollab/Packages.git")
add_requires("def_type")
add_requires("catch2 3.x")

target("smoke-test")
    set_kind("binary")
    add_files("tests.cpp")
    add_packages("def_type", "catch2")
