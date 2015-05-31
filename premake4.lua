simavr_library_path = path.getdirectory(_SCRIPT) .. "/simavr/simavr/obj-x86_64-linux-gnu"

solution "TeensyLCDSimulator"
    configurations { "Debug", "Release" }
    language "C"

    configuration "gmake"
        buildoptions { "-std=c99", "-fPIC", "-O3", "-g" }
        includedirs { "/usr/include/SDL2" }
    
    configuration "Debug"
        defines "DEBUG"
        flags "Symbols"
    
    configuration "Release"
        defines "NDEBUG"
        flags "Optimize"
    
    --include "simavr"
    include "libteensylcd"
    include "teensylcd_run"


