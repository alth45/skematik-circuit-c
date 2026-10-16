@echo off
run_tests.exe > tests_run_log3.txt 2>&1
set RC=%ERRORLEVEL%
echo RC=%RC% > tests_exit.txt
