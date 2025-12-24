The goal is to create a python 3.13+ compatible C/C++ extension from TRON (formerly known as Lite³) and a sample python 3.13 application that uses it.

1) Read carefully the documentation of TRON:
   * How to guides: https://lite3.io/how_to_guides.html
   * Installation: https://lite3.io/installation.html
   * Topics: https://lite3.io/topics.html
2) Clone the TRON repository into a tron_lib subfolder using ssh as git@github.com:fastserial/lite3.git
3) Create a C/C++ python 3.13+ compatible C/C++ extension from it.
4) Make sure you update all the tests that comes with the original library and run updated tests against the newly created library. If any test fails fix until all of them works.
5) Create the same tests in python 3.13 using pytest. If any test fails fix until all of them works.
6) Create a simple python 3.13 application that uses the newly created tron python C/C++ extension.
   1) Creating a new tron object, adding multiple different items, removing some of these items, saving the tron object to a native tron file and then reading it back and printing its content.
   2) Creating a new tron object from a sample python dict's json output. Saving to a native tron file and then reading it back and converting to json and printing the json.
   3) Showcase all other functionalities of the tron python C/C++ extension.
7) For the python projects use uv (from Astral) as project, dependency and venv manager.
8) Create AGENTS.md and codex skills which you see useful for this project. The codex skills must be a local skills for this repository only.
9) Add a high-level Python ergonomics layer with dict/list auto-conversion and a wrapper class. Using that user should build TRON buffers directly from Python objects, append nested structures, and convert back to Python via JSON. Add tests for this layer and showcase its capabilities in the sample application.
10) Create a comprehensive documentation in markdown format in README.md:
    1) For the new python C/C++ extension, the ergonomics layer and for the wrapper class with usage examples;
    2) Include build badges, a troubleshooting section, and API reference tables;
    39 Include performance notes, memory limits, or a migration guide from raw C usage.

