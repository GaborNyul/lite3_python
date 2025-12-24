---
name: tron-testing
description: Run and maintain the TRON C tests and the mirrored pytest suite for the Python bindings.
---

# TRON Testing

Use this skill when updating or troubleshooting tests.

## C tests
- Run `make -C tron_lib tests`.
- If a test fails, inspect the corresponding file in `tron_lib/tests/`.

## Python tests
- Run `uv run pytest`.
- The pytest suite mirrors `tron_lib/tests/`:
  - `tests/test_john_doe.py`
  - `tests/test_collisions.py`
  - `tests/test_alignment_zeroing.py`

## Notes
- The alignment test relies on `Tron.debug_fill` and exposes `LITE3_NODE_SIZE` and `LITE3_ZERO_MEM_8`.
