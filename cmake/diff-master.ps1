## Make looking at divergence from simh/master easier...

$excludes = @(
    "/*CMakeLists.txt",
    "/.gitignore",
    "/Visual Studio Projects/",
    "/build_*.bat",
    "/cmake/",
    "/PDP8/tests/diags/*.pal",
    "/PDP8/tests/diags/*.txt",
    "/appveyor.yml"
) | % { "`":!" + $_ + "`"" }

git diff --ignore-space-at-eol simh/master HEAD -- ${excludes}
