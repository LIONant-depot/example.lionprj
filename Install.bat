@echo off
setlocal EnableDelayedExpansion

set "ORIGINAL_DIR=%CD%"

rem Clean and recreate Cache folder
rmdir /s /q Cache
mkdir Cache
if errorlevel 1 (
    echo Failed to create Cache folder.
    exit /b 1
)
cd Cache
if errorlevel 1 (
    echo Failed to change to Cache directory.
    exit /b 1
)
mkdir Plugins
if errorlevel 1 (
    echo Failed to create Plugins folder.
    exit /b 1
)
cd Plugins
if errorlevel 1 (
    echo Failed to change to Plugins directory.
    exit /b 1
)

rem Check for git
where git >nul 2>&1
if errorlevel 1 (
    echo Git is not installed or not in PATH.
    goto :ERROR
)

rem ---------- DEPOTS ---------------
set "PLUGINS=xvirtual_folders.plugin xmaterial.plugin xtexture.plugin xgeom.plugin"
for %%P in (%PLUGINS%) do (
    echo Cloning %%P...
    git clone --depth 1 https://github.com/LIONant-depot/%%P
    if errorlevel 1 (
        echo Failed to clone %%P.
        goto :ERROR
    )
)

rem ---------- CREATE WORM HOLE FOR DEPENDENCIES ---------------
mkdir ..\dependencies
if errorlevel 1 (
    echo Failed to create dependencies folder.
    goto :ERROR
)

rem Resolve absolute path for dependencies
for %%F in ("..\dependencies") do set "dependencies_full=%%~fF"

rem Create symbolic links for plugins requiring dependencies
set "LINK_PLUGINS=xmaterial.plugin xgeom.plugin xtexture.plugin"
for %%P in (%LINK_PLUGINS%) do (
    echo Creating symbolic link for %%P\dependencies
    if exist "%%P\dependencies" (
        echo Error: %%P\dependencies already exists.
        goto :ERROR
    )
    mklink /D "%CD%\%%P\dependencies" "!dependencies_full!"
    if errorlevel 1 (
        echo Failed to create symbolic link for %%P\dependencies.
        goto :ERROR
    )
)

rem ---------- INSTALLATIONS ---------------
for %%P in (%LINK_PLUGINS%) do (
    if not exist %%P\build\CreateAndBuildProject.bat (
        echo CreateAndBuildProject.bat not found in %%P.
        goto :ERROR
    )
    echo Running CreateAndBuildProject.bat for %%P...
    cd %%P\build
    if errorlevel 1 (
        echo Failed to change to %%P\build directory.
        goto :ERROR
    )
    call CreateAndBuildProject.bat "return"
    if errorlevel 1 (
        echo Failed to execute CreateAndBuildProject.bat for %%P.
        cd /d "%ORIGINAL_DIR%\Cache\Plugins"
        goto :ERROR
    )
    cd /d "%ORIGINAL_DIR%\Cache\Plugins"
    if errorlevel 1 (
        echo Failed to return to Plugins directory.
        goto :ERROR
    )
)

rem ------ COPY THE CHECK SCRIPT ----------
cd /d "%ORIGINAL_DIR%" 2>nul
copy Cache\dependencies\check_changes.bat Cache\Plugins

rem ---------- DONE ---------------
echo Completed successfully!
cd /d "%ORIGINAL_DIR%" 2>nul
if "%1"=="" pause
exit /b 0

rem ---------- ERROR ---------------
:ERROR
echo An error occurred.
cd /d "%ORIGINAL_DIR%" 2>nul
if "%1"=="" pause
exit /b 1