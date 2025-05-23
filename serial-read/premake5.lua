workspace "syn_app"
    -- location of generated solution/make and build files
    location "build"

    -- architecture: 'x86' or 'x86_64'
    architecture "x86_64"
    staticruntime "on"
  
    -- language to be compiled and c++ flavor
    language "C++"
    cppdialect "C++17"

    -- set flags for the compiler
    flags { "MultiProcessorCompile" }

    -- for using OpenMP
    --buildoptions { "-fopenmp" }

    -- used for storing compiler / linker settings togehter
    configurations { "Debug", "Release" }

    -- enable symbols for Debug builds
    filter "configurations:Debug"
        defines { "DEBUG", "DEBUG_PROFILING" }
        optimize "Debug"
        symbols "On"
        buildoptions { "-fno-limit-debug-info" }
        linkoptions { "-rdynamic" }

    -- enable optimization for Release builds
    filter "configurations:Release"
        defines "NDEBUG"
        --optimize "On" --> -O2
        optimize "Speed" -- --> -O3

    -- reset filter
    filter { }


-----------------------------------------------------------------------------------------
project "serial-read"

    kind "ConsoleApp"

    targetdir ("%{wks.location}")
	objdir ("%{wks.location}/obj")

    -- show preprocessor output -- didn't work
    --buildoptions { "-E" }

    files
    {
        "src/**.c",
        "src/**.cpp",
        "src/**.h",
        "src/**.hpp",
    }

    defines
    {
        -- "_CRT_SECURE_NO_WARNINGS",
        -- "GLFW_INCLUDE_NONE",
    }

    includedirs
    {
        ".",    -- to allow orderly structured includes in files
                -- in the style #include "src/.../file.h" instead
                -- of #include "./file.h" or #include "../../file.h".
        "/usr/include/freetype2",
        "/usr/include/synapse",
    }

    libdirs
    {
    }

    links
    {
        "glfw3",
        "glad",
        "assimp",
        "freetype",
        "pthread",
        "dl",           -- dep of glfw
        "X11",          -- dep of glfw (Linux only)
        "atomic",
        -- "synapse-dev", -- instead linked under Configuration filters
        "synapse",
    }

    filter { "configurations.Debug" }
        runtime "Debug"
        -- links "synapse-dev"

    filter { "configurations.Release" }
        runtime "Release"
        -- links "synapse"

