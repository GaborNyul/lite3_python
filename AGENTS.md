# Repository Agent Notes

## Layout
- `tron_lib/` contains the upstream TRON (Lite3) C library.
- `tron/` contains the Python C-extension (`tron/_tron.c`) and package initializer.
- `tests/` holds pytest coverage that mirrors the original C tests.
- `examples/sample_app.py` demonstrates typical module usage.

## Build + Test (C library)
- Build + run C tests:
  - `make -C tron_lib tests`

## Build + Test (Python)
- Python 3.13+ is required. Install via uv if needed:
  - `uv python install 3.13`
- Create/update the environment:
  - `uv sync --python 3.13`
- Build/install the module in editable mode:
  - `uv pip install -e .`
- Run Python tests:
  - `uv run pytest`

## Notes
- The TRON buffer is written/read as raw bytes in `Tron.save` and `Tron.from_file`.
- JSON conversion relies on the bundled yyjson and base64 sources compiled into the extension.
- Offsets are used to address nested objects/arrays in the TRON buffer.

## Local Skills
- Local Codex skills live in `.codex/skills/`.
