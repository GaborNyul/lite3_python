from pathlib import Path

from setuptools import Extension, setup

ROOT = Path(__file__).resolve().parent

lite3_src = list((ROOT / "tron_lib" / "src").glob("*.c"))
lib_yyjson = list((ROOT / "tron_lib" / "lib" / "yyjson").glob("*.c"))
lib_base64 = list((ROOT / "tron_lib" / "lib" / "nibble_base64").glob("*.c"))

sources = [
    "tron/_tron.c",
    *[str(path.relative_to(ROOT)) for path in lite3_src],
    *[str(path.relative_to(ROOT)) for path in lib_yyjson],
    *[str(path.relative_to(ROOT)) for path in lib_base64],
]

extension = Extension(
    name="tron._tron",
    sources=sources,
    include_dirs=[
        "tron_lib/include",
        "tron_lib/lib",
        "tron_lib/lib/yyjson",
        "tron_lib/lib/nibble_base64",
    ],
    extra_compile_args=["-std=c11"],
)

setup(
    name="tron",
    version="0.1.0",
    description="Python bindings for TRON (Lite3)",
    packages=["tron"],
    ext_modules=[extension],
)
