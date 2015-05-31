project "teensylcd"
  kind "StaticLib"
  --kind "SharedLib"
  includedirs {
      "../simavr/simavr/sim"
  }
  libdirs {
      simavr_library_path
  }
  links {
      "simavr"
  }
  files {
      "teensylcd.h",
      "teensylcd.c",
      "pcd8544.h",
      "pcd8544.c",
      "timer.c"
  }

