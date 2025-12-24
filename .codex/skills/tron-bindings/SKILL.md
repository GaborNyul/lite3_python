---
name: tron-bindings
description: Build, modify, or debug the TRON (Lite3) Python C-extension and its integration with the upstream C library in this repo.
---

# TRON Python Bindings

Use this skill when working on the Python C-extension (`tron/_tron.c`) or build configuration.

## Workflow
1. Confirm upstream sources in `tron_lib/` (headers in `tron_lib/include`, sources in `tron_lib/src`).
2. Update the extension build in `setup.py` when adding or removing C sources.
3. Rebuild the extension with `uv pip install -e .`.
4. Run `make -C tron_lib tests` and `uv run pytest` after changes.

## Conventions
- Prefer the context API (`lite3_context_api.h`) for Python bindings to handle buffer growth.
- Keep Python-visible APIs thin wrappers around the C API.
- Surface Lite3 constants via module attributes when tests or samples need them.
