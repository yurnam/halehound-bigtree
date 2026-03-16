Import("env")

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
