workspace "gloo_test"
  location "./project"
  configurations { "Debug" }
project "gloo_test"
  kind "SharedLib"
  language "C++"
  location "./project"
  targetdir "./bin"

  files {
    "src/**.cpp",
    "src/**.h",
  }

  include "../premake5.lua"

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"