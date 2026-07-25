add_rules("mode.debug", "mode.release")
add_requires("opengl", "glu")

target("qwt")
    add_rules("qt.shared")
    add_frameworks(
        "QtWidgets", "QtGui", "QtCore", "QtSvg", "QtTest", 
        "QtOpenGL", "QtPrintSupport", "QtConcurrent"
    )
    add_files("src/**.cpp")
    add_files("src/**.c")
    add_files("src/**.h")
    add_headerfiles("(classincludes/**)", {prefixdir = "qwt"})
    add_headerfiles("src/(**.h)", {prefixdir = "qwt"})
    add_includedirs("src")
    add_includedirs(os.dirs("src/**"))
    add_defines(
        "QWTCORE_MAKEDLL", "QWTCORE_DLL",
        "QWT_MAKEDLL", "QWT_DLL", 
        "QWT3D_MAKEDLL", "QWT3D_DLL", 
        "GL2PSDLL_EXPORTS", "GL2PSDLL"
    )
    if is_plat("windows") and is_mode("debug") then
        set_suffixname("D")
    end
    add_packages("opengl", "glu")
target_end()
