local config = require "config"

workspace "gloo_test"
  location "./project"
  configurations { "Debug" }
project "gloo_test"
  kind "SharedLib"
  language "C++"
  location "./project"
  targetdir "./bin"
  cppdialect "C++17"

  files {
    "src/**.cpp",
    "src/**.h",
  }

  include "../premake5.lua"

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

  postbuildcommands {
    '{COPY} "%{cfg.buildtarget.abspath}" "'..config.garrysmod..'/garrysmod/lua/bin/'..config.libname..'"*',
  }