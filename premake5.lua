local suffixes = {
  linux   = "_linux",
  macosx  = "_macosx",
  windows = "_win32",
}

defines "GMMODULE"
cppdialect "C++17"

includedirs {
  "include",
  "vendor/gmod-module-base/include",
}

targetprefix "gm_"
targetsuffix(suffixes[os.target()])
targetextension ".dll"