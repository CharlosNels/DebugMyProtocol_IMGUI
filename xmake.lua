add_rules("mode.debug", "mode.release")
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
add_requires("libsdl2")
add_requires("opengl")
add_requires("spdlog")
add_requires("boost")

target("DebugMyProtocol_IMGUI")
    set_kind("binary")
    add_packages("imgui", gettext_lib_str, "cserialport", "libsdl", "opengl", "spdlog", "boost")
    add_files("./src/*.cpp")



