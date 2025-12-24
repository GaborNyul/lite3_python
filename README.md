# TRON (Lite3) Python Bindings

Python 3.13+ C-extension bindings for TRON (formerly Lite³), plus a high-level ergonomics layer for dict/list conversion.

[![Python 3.13+](https://img.shields.io/badge/python-3.13%2B-blue.svg)](https://www.python.org/downloads/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

This repository includes:
- `tron/` — the C-extension module (`tron/_tron.c`) and Python package.
- `tron_lib/` — upstream TRON C library.
- `tests/` — pytest suite mirroring upstream C tests.
- `examples/sample_app.py` — end-to-end usage examples.

## Requirements
- Python **3.13+**
- `uv` for environment management
- A C compiler (gcc/clang)

## Quick Start (uv)
```bash
uv python install 3.13
uv sync --python 3.13 --extra dev
uv pip install -e .
```

Run tests:
```bash
make -C tron_lib tests
uv run pytest
```

Run the sample app:
```bash
uv run python examples/sample_app.py
```

## Package Layout
- `tron._tron` — low-level C-extension bindings (thin wrappers around TRON’s context API)
- `tron.py` — ergonomics layer for dict/list conversion
- `tron.TronDocument` — wrapper class with convenience helpers

## Low-Level C-Extension API (`tron._tron`)

### `Tron` class
Create a TRON context and work directly with object/array data.

```python
from tron import Tron

tron = Tron()  # root object
tron.set_str("event", "lap_complete")
tron.set_i64("lap", 55)
tron.set_f64("time_sec", 88.427)

print(tron.to_json(pretty=True))
```

### API Reference (C-extension)

#### Initialization
| Method | Description |
| --- | --- |
| `Tron(root="object"\|"array", bufsz=0)` | Create a context (default root object) |
| `init_obj()` | Reset root as object |
| `init_arr()` | Reset root as array |

#### Object setters
| Method | Description |
| --- | --- |
| `set_null(key, ofs=0)` | Set null value |
| `set_bool(key, value, ofs=0)` | Set boolean |
| `set_i64(key, value, ofs=0)` | Set int64 |
| `set_f64(key, value, ofs=0)` | Set float64 |
| `set_bytes(key, value, ofs=0)` | Set bytes |
| `set_str(key, value, ofs=0)` | Set string |
| `set_obj(key, ofs=0) -> out_ofs` | Insert nested object |
| `set_arr(key, ofs=0) -> out_ofs` | Insert nested array |
| `delete(key, ofs=0)` | Alias for `set_null` |

#### Object getters
| Method | Description |
| --- | --- |
| `get_bool(key, ofs=0)` | Get boolean |
| `get_i64(key, ofs=0)` | Get int64 |
| `get_f64(key, ofs=0)` | Get float64 |
| `get_bytes(key, ofs=0)` | Get bytes |
| `get_str(key, ofs=0)` | Get string |
| `get_obj(key, ofs=0) -> out_ofs` | Get nested object offset |
| `get_arr(key, ofs=0) -> out_ofs` | Get nested array offset |
| `get_type(key, ofs=0)` | Return type as string |
| `get(key, ofs=0)` | Auto-typed getter |
| `exists(key, ofs=0)` | Key existence check |

#### Array appenders
| Method | Description |
| --- | --- |
| `arr_append_null(ofs=0)` | Append null |
| `arr_append_bool(value, ofs=0)` | Append boolean |
| `arr_append_i64(value, ofs=0)` | Append int64 |
| `arr_append_f64(value, ofs=0)` | Append float64 |
| `arr_append_bytes(value, ofs=0)` | Append bytes |
| `arr_append_str(value, ofs=0)` | Append string |
| `arr_append_obj(ofs=0) -> out_ofs` | Append object |
| `arr_append_arr(ofs=0) -> out_ofs` | Append array |

#### Array getters
| Method | Description |
| --- | --- |
| `arr_get_bool(index, ofs=0)` | Get boolean |
| `arr_get_i64(index, ofs=0)` | Get int64 |
| `arr_get_f64(index, ofs=0)` | Get float64 |
| `arr_get_bytes(index, ofs=0)` | Get bytes |
| `arr_get_str(index, ofs=0)` | Get string |
| `arr_get_obj(index, ofs=0) -> out_ofs` | Get object offset |
| `arr_get_arr(index, ofs=0) -> out_ofs` | Get array offset |

#### JSON + buffer helpers
| Method | Description |
| --- | --- |
| `to_json(ofs=0, pretty=False)` | Encode to JSON string |
| `to_bytes()` | Return raw buffer bytes |
| `buflen()` / `bufsz()` | Used/total buffer size |
| `save(path)` | Save raw buffer to file |

#### Constructors
| Method | Description |
| --- | --- |
| `Tron.from_bytes(data)` | Create from raw bytes |
| `Tron.from_json(json_str)` | Create from JSON string |
| `Tron.from_json_file(path)` | Create from JSON file |
| `Tron.from_file(path)` | Create from raw file |

#### Constants
| Constant | Description |
| --- | --- |
| `LITE3_NODE_SIZE` | Node size in bytes |
| `LITE3_NODE_ALIGNMENT` | Alignment requirement |
| `LITE3_ZERO_MEM_8` | Padding byte value (zeroing feature) |
| `DJB2_HASH_SEED` | Hash seed constant |

### Nested Objects & Arrays (Offsets)
TRON uses offsets to access nested objects/arrays.

```python
tron = Tron()
headers_ofs = tron.set_obj("headers")
tron.set_str("content-type", "application/json", ofs=headers_ofs)

items_ofs = tron.set_arr("items")
tron.arr_append_str("alpha", ofs=items_ofs)
tron.arr_append_i64(42, ofs=items_ofs)
```

## Ergonomics Layer (`tron.py`)

Use dicts/lists and let the helpers map them into TRON data.

### `from_obj`
```python
from tron import from_obj

payload = {
    "user": {"id": 123, "name": "Jane"},
    "roles": ["admin", "editor"],
    "active": True,
}

tron = from_obj(payload)
print(tron.to_json(pretty=True))
```

### `to_obj`
```python
from tron import to_obj

obj = to_obj(tron)
print(obj)
```

Note: JSON conversion is used internally, so `bytes` become base64 strings.

### Type Mapping
| Python Type | TRON Type |
| --- | --- |
| `None` | null |
| `bool` | bool |
| `int` | int64 |
| `float` | float64 |
| `str` | string |
| `bytes` / `bytearray` / `memoryview` | bytes |
| `dict` | object |
| `list` / `tuple` | array |

Unsupported types raise `TypeError`.

## Wrapper Class (`TronDocument`)

`TronDocument` provides an object-oriented wrapper with convenience helpers and direct access to the underlying `Tron`.

```python
from tron import TronDocument

payload = {"name": "Alice", "tags": ["x", "y"]}

# Build from Python data
 doc = TronDocument.from_obj(payload)

# Add fields later
 doc.set_value("score", 99)

print(doc.to_obj())
```

### API Reference (TronDocument)
| Method | Description |
| --- | --- |
| `from_obj(obj)` | Build from dict/list |
| `to_obj()` | Convert to Python via JSON |
| `set_value(key, value, ofs=0)` | Set value with auto type |
| `set_value_map(mapping, ofs=0)` | Insert many fields |
| `append_value(value, ofs=0)` | Append to array |
| `append_value_list(values, ofs=0)` | Append list to array |
| `tron` | Access underlying `Tron` object |

## Examples
See `examples/sample_app.py` for all workflows:
1) Create TRON object, add/remove items, save/load.
2) Build from JSON string and round-trip.
3) Nested object/array usage.
4) Ergonomics helpers (`from_obj`, `TronDocument`).

## Troubleshooting

### Build failures or missing compiler
- Ensure `gcc` or `clang` is installed and on PATH.
- Try rebuilding:
  ```bash
  uv pip install -e .
  ```

### Python version mismatch
- Make sure Python 3.13+ is used:
  ```bash
  uv python install 3.13
  uv sync --python 3.13 --extra dev
  ```

### JSON conversion errors
- JSON helpers rely on TRON’s bundled `yyjson` and base64 sources.
- If you disabled JSON compilation in the upstream library, re-enable it.

### Bytes round-trip
- `to_obj()` uses JSON conversion internally, so bytes are converted to base64 strings.
- For raw byte access, use `tron.to_bytes()` and `Tron.from_bytes()`.

### Alignment/zeroing test failures
- Ensure `LITE3_ZERO_MEM_EXTRA` is enabled in `tron_lib/include/lite3.h` (enabled by default).

## Development Notes
- TRON JSON conversion is compiled in by default (yyjson + base64).
- The bindings use the Context API so buffers auto-grow.

## Performance Notes
- TRON is designed for zero-copy reads and in-place updates; use getters/setters directly on the buffer when performance matters.
- Context API growth is geometric; pre-sizing with `Tron(bufsz=...)` avoids reallocation for large messages.
- JSON conversion (`to_json`, `from_json`) performs allocations and is slower than direct TRON access; prefer it only for interop/debugging.

## Memory Limits
- TRON enforces an internal maximum buffer size (`LITE3_BUF_SIZE_MAX` in `tron_lib/include/lite3.h`).
- Context buffers are aligned to `LITE3_NODE_ALIGNMENT`; very small buffers are clamped to `LITE3_CONTEXT_BUF_SIZE_MIN`.
- When `LITE3_ZERO_MEM_EXTRA` is enabled (default), extra padding bytes are zeroed for safety and debuggability.

## Migration Guide (Raw C → Python)

### 1) Initialize a root object
**C**
```c
unsigned char buf[1024];
size_t buflen = 0;
lite3_init_obj(buf, &buflen, sizeof(buf));
```

**Python**
```python
from tron import Tron

tron = Tron()
```

### 2) Set fields
**C**
```c
lite3_set_str(buf, &buflen, 0, sizeof(buf), "event", "lap_complete");
lite3_set_i64(buf, &buflen, 0, sizeof(buf), "lap", 55);
```

**Python**
```python
tron.set_str("event", "lap_complete")
tron.set_i64("lap", 55)
```

### 3) Nested objects
**C**
```c
size_t headers_ofs;
lite3_set_obj(buf, &buflen, 0, sizeof(buf), "headers", &headers_ofs);
lite3_set_str(buf, &buflen, headers_ofs, sizeof(buf), "content-type", "application/json");
```

**Python**
```python
headers_ofs = tron.set_obj("headers")
tron.set_str("content-type", "application/json", ofs=headers_ofs)
```

### 4) Arrays
**C**
```c
size_t tags_ofs;
lite3_set_arr(buf, &buflen, 0, sizeof(buf), "tags", &tags_ofs);
lite3_arr_append_str(buf, &buflen, tags_ofs, sizeof(buf), "alpha");
```

**Python**
```python
tags_ofs = tron.set_arr("tags")
tron.arr_append_str("alpha", ofs=tags_ofs)
```

### 5) JSON conversion
**C**
```c
char *json = lite3_json_enc(buf, buflen, 0, NULL);
printf("%s\n", json);
free(json);
```

**Python**
```python
print(tron.to_json(pretty=True))
```

### 6) High-level conversion (new in Python)
**Python only**
```python
from tron import from_obj, TronDocument

tron = from_obj({"user": {"id": 1}, "tags": ["a", "b"]})
doc = TronDocument.from_obj({"name": "Alice", "score": 99})
```

## License
- TRON (Lite3) is MIT licensed.
