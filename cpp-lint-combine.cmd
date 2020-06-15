@echo off 
set CLAZY_CHECKS=level0,level1,level2
cpp-lint-combine.exe -clazy-checks=%CLAZY_CHECKS% %*
