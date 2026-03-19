import os
import sys

Import("env")

def prune_libs(source, target, env):
    """
    Script to prune heavy library files that aren't excluded by standard defines.
    This ensures portability across systems without manually editing .pio folder.
    """
    # Use the project's library dependencies directory
    lib_deps_dir = env.get("PROJECT_LIBDEPS_DIR")
    if not lib_deps_dir:
        return

    # Use the specific environment subfolder
    env_name = env.get("PIOENV")
    esp_libs_dir = os.path.join(lib_deps_dir, env_name)
    
    if not os.path.exists(esp_libs_dir):
        return

    print(f"--- [PRUNE] Checking libraries in {esp_libs_dir} ---")

    # 1. Prune LovyanGFX heavy fonts
    lgfx_efont_dir = os.path.join(esp_libs_dir, "LovyanGFX", "src", "lgfx", "Fonts", "efont")
    if os.path.exists(lgfx_efont_dir):
        fonts_to_prune = ["lgfx_efont_cn.c", "lgfx_efont_ja.c", "lgfx_efont_kr.c", "lgfx_efont_tw.c"]
        for font in fonts_to_prune:
            font_path = os.path.join(lgfx_efont_dir, font)
            if os.path.exists(font_path) and os.path.getsize(font_path) > 100:
                print(f"--- [PRUNE] Truncating: {font} ---")
                with open(font_path, "w") as f:
                    f.write("// Pruned by build script to save flash\n")

    # 2. LVGL unused widgets pruning removed
    # We have 4MB flash now, so we can afford these small files.
    # Pruning them was causing linker errors with internal dependencies.
    pass

# Register the pruning function to run before the binary is linked
env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", prune_libs)

# Also run once immediately to ensure the folder is clean before compilation starts
prune_libs(None, None, env)
