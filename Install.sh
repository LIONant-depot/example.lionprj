#!/usr/bin/env bash
# Install.sh: the Linux counterpart of Install.bat (same name, same job): clone the plugins into Cache/Plugins and give each one its "dependencies" link.
#
#   bash Install.sh            clone what is missing (an existing plugin is left alone)
#   bash Install.sh --update   also move every plugin to the newest commit of main (what the CI does)
#
# The list of plugins is the PLUGINS line of Install.bat (one list, read from there). The Windows batch file also builds the compiler of each of those plugins; on Linux the build of xLION
# does that (the xlion_compilers target), so this script only clones and links. The plugins after it (HEADER_ONLY) are the ones Windows gets from the root CMakeLists.txt of xLION.
# Temporary by design: when the editor enables the plugins from the project settings, this and Install.bat go away together.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GIT_BASE="${GIT_BASE:-https://github.com/LIONant-depot}"
UPDATE=0; [ "${1:-}" = "--update" ] && UPDATE=1

PLUGINS="$(tr -d '\r' < "$HERE/Install.bat" | sed -n 's/^set "PLUGINS=\(.*\)"$/\1/p')"
[ -n "$PLUGINS" ] || { echo "Install.sh: there is no PLUGINS line in Install.bat"; exit 1; }
HEADER_ONLY="xscene.plugin xlevel.plugin xscript_module.plugin xgame.plugin xPhysicsMaterial.plugin xShareComponent.plugin xprefab.plugin"

PLUGINS_DIR="$HERE/Cache/Plugins"
mkdir -p "$PLUGINS_DIR" "$HERE/Cache/dependencies"
command -v git > /dev/null || { echo "Git is not installed or not in PATH."; exit 1; }

for p in $PLUGINS $HEADER_ONLY; do
  d="$PLUGINS_DIR/$p"
  if [ -d "$d/.git" ] && git -C "$d" rev-parse -q --verify HEAD > /dev/null; then
    if [ "$UPDATE" = 1 ]; then
      git -C "$d" fetch -q --depth 1 origin main && git -C "$d" reset -q --hard FETCH_HEAD || { echo "Failed to update $p."; exit 1; }
      echo "Updated $p."
    else
      echo "Already exist skipping the clone $p."
    fi
  else
    echo "Cloning $p..."
    rm -rf "$d"                                              # a half finished clone is redone
    git clone -q --depth 1 "$GIT_BASE/$p.git" "$d" || { echo "Failed to clone $p."; exit 1; }
  fi
  # every plugin except xvirtual_folders reaches the shared dependencies through its own "dependencies" link
  if [ "$p" != "xvirtual_folders.plugin" ] && [ ! -e "$d/dependencies" ]; then
    echo "Creating symbolic link for $p/dependencies"
    ln -s ../../dependencies "$d/dependencies" || { echo "Failed to create symbolic link for $p/dependencies."; exit 1; }
  fi
done
echo "Completed successfully!"
