-- collab.lua — shared helper for BuildWithCollab projects
--
-- Usage in xmake.lua:
--   includes("xmake/collab.lua")
--
--   -- Top level (like add_requires)
--   add_collab_requires("collab.core", "collab.process")
--
--   -- Inside a target (like add_packages)
--   target("myapp")
--       add_collab_packages("collab.core", "collab.process")
--
-- All names use dots (the target/module name). The helpers convert to
-- dashes internally for folder lookups and xmake package names.
--
-- Functions:
--   use_collab_local("collab.core")  — mark a package as local (alternative to env var)
--
-- Environment variables:
--   BUILDWITHCOLLAB_ROOT   — path to the folder containing all collab repos
--                              (e.g. C:\Code\mrowr\BuildWithCollab)
--   BUILDWITHCOLLAB_LOCAL  — comma-delimited list of package names to pull
--                              from local folders instead of the xmake registry
--                              (e.g. "collab-core,collab-process")
--                              (env var uses dashes — folder names)

-- BuildWithCollab package registry
add_repositories("BuildWithCollab https://github.com/BuildWithCollab/Packages")

-- dots → dashes (collab.core → collab-core)
local function _to_dash(name)
    return name:gsub("%.", "-")
end

-- Packages marked local via use_collab_local() — merged with BUILDWITHCOLLAB_LOCAL
-- Stored as dash names internally (folder/package names)
local _explicit_locals = {}

-- Takes dotted names: use_collab_local("collab.core")
function use_collab_local(...)
    for _, name in ipairs({...}) do
        _explicit_locals[_to_dash(name)] = true
    end
end

-- Returns a set of dash names + the root path
local function _get_local_set()
    local root = os.getenv("BUILDWITHCOLLAB_ROOT")
    local local_csv = os.getenv("BUILDWITHCOLLAB_LOCAL")

    if not root or not os.isdir(root) then return {}, nil end

    -- Start with explicitly marked locals
    local set = {}
    for name, _ in pairs(_explicit_locals) do
        set[name] = true
    end

    -- Merge in env var entries (already dashes)
    if local_csv and #local_csv > 0 then
        for name in local_csv:gmatch("([^,]+)") do
            local trimmed = name:match("^%s*(.-)%s*$")
            if #trimmed > 0 then
                set[trimmed] = true
            end
        end
    end

    return set, root
end

-- Top level — like add_requires, but checks local first
-- Takes dotted names: add_collab_requires("collab.core")
function add_collab_requires(...)
    local names = {...}
    local local_set, root = _get_local_set()

    local not_found = {}

    for _, name in ipairs(names) do
        local dash = _to_dash(name)
        if local_set[dash] then
            local xmake_file = path.join(root, dash, "xmake.lua")
            if os.isfile(xmake_file) then
                includes(xmake_file)
            else
                table.insert(not_found, dash)
            end
        else
            add_requires(dash)
        end
    end

    if #not_found > 0 then
        raise("BUILDWITHCOLLAB_LOCAL lists packages not found in "
            .. root .. ":\n  " .. table.concat(not_found, ", ")
            .. "\n\nClone them or remove from BUILDWITHCOLLAB_LOCAL.")
    end
end

-- Inside a target — like add_packages, but uses add_deps for local
-- Takes dotted names + optional opts table:
--   add_collab_packages("collab.core")
--   add_collab_packages("collab.core", { public = true })
function add_collab_packages(...)
    local args = {...}
    local opts = {}

    -- If the last arg is a table, it's the options
    if type(args[#args]) == "table" then
        opts = table.remove(args)
    end

    local local_set, root = _get_local_set()

    for _, name in ipairs(args) do
        local dash = _to_dash(name)
        if local_set[dash] and root and os.isfile(path.join(root, dash, "xmake.lua")) then
            add_deps(name, opts)         -- dotted target name
        else
            add_packages(dash, opts)     -- dashed package name
        end
    end
end
