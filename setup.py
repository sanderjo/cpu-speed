from setuptools import setup, Extension

ext = Extension(
    "cpuspeed",
    sources=["cpuspeed.c"],
    extra_compile_args=["-O2"],
    libraries=["pthread"],
)

setup(
    name="cpuspeed",
    version="1.0.0",
    description="Dhrystone-style integer CPU speed benchmark",
    ext_modules=[ext],
)
