local suffixes = {
  linux   = "_linux",
  macosx  = "_macosx",
  windows = "_win32",
}

defines "GMMODULE"

includedirs {
  "include",
  "vendor/gmod-module-base/include",
}

targetprefix "gm_"
targetsuffix(suffixes[os.target()])
targetextension ".dll"