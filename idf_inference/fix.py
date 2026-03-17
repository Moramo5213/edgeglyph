import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent
COMPONENTS = ROOT / "components"

print("Project:", ROOT)

driver_map = {
    "gpio": "esp_driver_gpio",
    "spi": "esp_driver_spi",
    "ledc": "esp_driver_ledc",
    "i2c": "esp_driver_i2c",
}

def patch_cmake(path, drivers):
    text = path.read_text()

    deps = " ".join(drivers + ["freertos"])

    new = f"""idf_component_register(
    SRCS "{path.parent.name}.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES {deps}
)
"""

    path.write_text(new)
    print("patched:", path)

for comp in COMPONENTS.iterdir():

    cmake = comp / "CMakeLists.txt"
    if not cmake.exists():
        continue

    name = comp.name.lower()

    if name == "gc9a01":
        patch_cmake(cmake, [
            driver_map["gpio"],
            driver_map["spi"],
            driver_map["ledc"]
        ])

    elif name == "touch":
        patch_cmake(cmake, [
            driver_map["gpio"],
            driver_map["i2c"]
        ])

    else:
        print("skipping component:", name)

# remove broken build cache
build = ROOT / "build"
if build.exists():
    print("removing stale build directory")
    shutil.rmtree(build)

print("\nDone.")
print("Now run:")
print("idf.py build")