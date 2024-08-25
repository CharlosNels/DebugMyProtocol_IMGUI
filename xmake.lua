add_rules("mode.debug", "mode.release")
set_policy("package.requires_lock", true)
set_project("DebugMyProtocol_IMGUI")
set_version("0.0.1")
set_license("LGPL-3.0")

local gettext_lib_str = ""

if is_os("windows") then
    gettext_lib_str = "libintl"
    add_defines("SDL_MAIN_HANDLED")
else
   gettext_lib_str = "gettext"
end
add_requires("imgui 1.89", {configs = {sdl2 = true, opengl2 = true, freetype = true}})
add_requires(gettext_lib_str)
add_requires("cserialport")
add_requires("libsdl")
add_requires("opengl")
add_requires("spdlog")
add_requires("boost", {configs = {headers = true}})

target("DebugMyProtocol_IMGUI")
    set_kind("binary")
    add_packages("imgui", gettext_lib_str, "cserialport", "libsdl", "opengl", "spdlog", "boost")
    add_files("./src/*.cpp")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

