project "teensylcd_run"
    kind "ConsoleApp"
    includedirs { 
        "../libteensylcd",
        "../simavr/simavr/sim"
    }
    links "teensylcd"
    libdirs {
        simavr_library_path
    }
    links "simavr"
    links "SDL2"
    links "elf"
    defines {
        "_BSD_SOURCE"
    }
    files {
        "teensylcd_run.c"
    }

