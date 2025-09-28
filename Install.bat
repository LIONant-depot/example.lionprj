@echo off
set "ORIGINAL_DIR=%CD%"

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
git clone --depth 1 https://github.com/LIONant-depot/xvirtual_folders.plugin
if errorlevel 1 (
    echo Failed to clone xvirtual_folders.plugin.
    goto :ERROR
)

git clone --depth 1 https://github.com/LIONant-depot/xmaterial.plugin
if errorlevel 1 (
    echo Failed to clone xmaterial.plugin.
    goto :ERROR
)

git clone --depth 1 https://github.com/LIONant-depot/xtexture.plugin
if errorlevel 1 (
    echo Failed to clone xtexture.plugin.
    goto :ERROR
)

git clone --depth 1 https://github.com/LIONant-depot/xgeom.plugin
if errorlevel 1 (
    echo Failed to clone xgeom.plugin.
    goto :ERROR
)

rem ---------- INSTALLATIONS ---------------
if not exist xmaterial.plugin\build\CreateAndBuildProject.bat (
    echo CreateAndBuildProject.bat not found in xmaterial.plugin.
    goto :ERROR
)
cd xmaterial.plugin\build
if errorlevel 1 (
    echo Failed to change to xmaterial.plugin\build directory.
    goto :ERROR
)
call CreateAndBuildProject.bat "return"
if errorlevel 1 (
    echo Failed to execute CreateAndBuildProject.bat for xmaterial.plugin.
    cd /d "%ORIGINAL_DIR%\Cache\Plugins"
    goto :ERROR
)
cd /d "%ORIGINAL_DIR%\Cache\Plugins"
if errorlevel 1 (
    echo Failed to return to Plugins directory.
    goto :ERROR
)

if not exist xtexture.plugin\build\CreateAndBuildProject.bat (
    echo CreateAndBuildProject.bat not found in xtexture.plugin.
    goto :ERROR
)
cd xtexture.plugin\build
if errorlevel 1 (
    echo Failed to change to xtexture.plugin\build directory.
    goto :ERROR
)
call CreateAndBuildProject.bat "return"
if errorlevel 1 (
    echo Failed to execute CreateAndBuildProject.bat for xtexture.plugin.
    cd /d "%ORIGINAL_DIR%\Cache\Plugins"
    goto :ERROR
)
cd /d "%ORIGINAL_DIR%\Cache\Plugins"
if errorlevel 1 (
    echo Failed to return to Plugins directory.
    goto :ERROR
)

if not exist xgeom.plugin\build\CreateAndBuildProject.bat (
    echo CreateAndBuildProject.bat not found in xgeom.plugin.
    goto :ERROR
)
cd xgeom.plugin\build
if errorlevel 1 (
    echo Failed to change to xgeom.plugin\build directory.
    goto :ERROR
)
call CreateAndBuildProject.bat "return"
if errorlevel 1 (
    echo Failed to execute CreateAndBuildProject.bat for xgeom.plugin.
    cd /d "%ORIGINAL_DIR%\Cache\Plugins"
    goto :ERROR
)
cd /d "%ORIGINAL_DIR%\Cache\Plugins"
if errorlevel 1 (
    echo Failed to return to Plugins directory.
    goto :ERROR
)

echo Completed successfully!
if "%1"=="" pause
exit /b 0

:ERROR
echo An error occurred.
cd /d "%ORIGINAL_DIR%" 2>nul
if "%1"=="" pause
exit /b 1