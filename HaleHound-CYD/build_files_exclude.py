Import("env")
import os
import re

# Exclude specific GFX library source files that do not compile for this target.
# Arduino_ESP32LCD8.cpp — requires 8-bit parallel LCD hardware not present on PandaTouch.
# Arduino_ESP32QSPI.cpp — requires QSPI bus, conflicts with OPI PSRAM on ESP32-S3.
# https://docs.platformio.org/en/latest/scripting/middlewares.html

custom_build_files_exclude = env.GetProjectOption("custom_build_files_exclude")
print(" ** Custom skip build targets ** ", custom_build_files_exclude)

def skip_from_build(env, node):
    """Return None to exclude this node from the build graph entirely."""
    print("Skipping", node.get_path())
    return None

for pattern in custom_build_files_exclude.split():
    env.AddBuildMiddleware(skip_from_build, pattern)

# ── Patch Arduino_ESP32RGBPanel.h ──────────────────────────────────────────
# GFX 1.6.1 introduced a bug in the header guard that controls the definition
# of the internal esp_rgb_panel_t struct copy.  The condition was changed from
# (ESP_ARDUINO_VERSION_MAJOR < 3) to (ESP_ARDUINO_VERSION_MAJOR > 5), which
# means the struct is never defined for any current Arduino ESP32 version
# (2.x: 2>5=false; 3.x: 3>5=false).  The .cpp still uses the old (< 3) guard,
# so when ESP_ARDUINO_VERSION_MAJOR == 2 the code tries to use the struct that
# the header no longer defines → compile error.
# Restoring the original < 3 condition makes the struct available for 2.x and
# keeps 3.x unaffected (3 < 3 = false → struct omitted, else-branch used).
#
# NOTE: The .h and .cpp use slightly different trailing content on that line:
#   .h  → #if ... >5)          (no trailing comment)
#   .cpp → #if ... >5)  //Modify
# Previous version used exact string matching which failed because of this
# difference. We now use a regex that matches >5 regardless of trailing text.
def patch_rgb_panel_header():
    libdeps_dir = os.path.join(
        env.subst("$PROJECT_LIBDEPS_DIR"),
        env.subst("$PIOENV"),
    )
    target = os.path.join(
        libdeps_dir,
        "GFX Library for Arduino",
        "src", "databus", "Arduino_ESP32RGBPanel.h",
    )
    if not os.path.isfile(target):
        print(" ** patch_rgb_panel_header: file not found, skipping **")
        return

    with open(target, "r") as f:
        content = f.read()

    sentinel = "// [HH-PATCHED]"
    if sentinel in content:
        print(" ** Arduino_ESP32RGBPanel.h already patched **")
        return

    # Match the broken #if line regardless of:
    #   - whitespace around >5
    #   - trailing //Modify comment or lack thereof
    #   - Windows (\r\n) vs Unix (\n) line endings
    # [^\r\n]* is safe: it matches any non-newline characters (stopping at the
    # end of the current line), so the pattern can never span across lines.
    BROKEN = re.compile(
        r'#if\s*\(!defined\(ESP_ARDUINO_VERSION_MAJOR\)\)\s*\|\|\s*'
        r'\(ESP_ARDUINO_VERSION_MAJOR\s*>\s*5\)[^\r\n]*\r?\n',
        re.MULTILINE,
    )
    FIXED = (
        sentinel + ": restored Arduino 2.x struct guard (GFX 1.6.1 header bug)\n"
        "#if (!defined(ESP_ARDUINO_VERSION_MAJOR)) || (ESP_ARDUINO_VERSION_MAJOR < 3)\n"
    )

    new_content, count = BROKEN.subn(FIXED, content)
    if count:
        with open(target, "w") as f:
            f.write(new_content)
        print(" ** Patched Arduino_ESP32RGBPanel.h: restored < 3 guard **")
    else:
        print(" ** Arduino_ESP32RGBPanel.h: broken guard pattern not found;"
              " struct guard may already be correct **")

patch_rgb_panel_header()
