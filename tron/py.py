import json
from typing import Any

from ._tron import Tron


class TronDocument:
    """High-level wrapper around Tron with dict/list convenience methods."""

    def __init__(self, *args, **kwargs):
        self._tron = Tron(*args, **kwargs)

    @property
    def tron(self) -> Tron:
        return self._tron

    def __getattr__(self, name):
        return getattr(self._tron, name)

    @classmethod
    def from_obj(cls, obj: Any) -> "TronDocument":
        doc = cls(root="object")
        if isinstance(obj, dict):
            doc._tron.init_obj()
            doc.set_value_map(obj)
            return doc
        if isinstance(obj, (list, tuple)):
            doc._tron.init_arr()
            doc.append_value_list(obj)
            return doc
        raise TypeError("root value must be dict or list")

    def to_obj(self) -> Any:
        return json.loads(self._tron.to_json())

    def set_value(self, key: str, value: Any, *, ofs: int = 0) -> None:
        if not isinstance(key, str):
            raise TypeError("object keys must be strings")
        _set_value(self._tron, key, value, ofs)

    def set_value_map(self, mapping: dict, *, ofs: int = 0) -> None:
        for key, value in mapping.items():
            self.set_value(key, value, ofs=ofs)

    def append_value(self, value: Any, *, ofs: int = 0) -> None:
        _append_value(self._tron, value, ofs)

    def append_value_list(self, values: list, *, ofs: int = 0) -> None:
        for value in values:
            self.append_value(value, ofs=ofs)


def from_obj(obj: Any) -> Tron:
    """Create a Tron instance from a dict or list without manual field setup."""
    if isinstance(obj, dict):
        tron = Tron(root="object")
        tron.init_obj()
        for key, value in obj.items():
            _set_value(tron, key, value, 0)
        return tron
    if isinstance(obj, (list, tuple)):
        tron = Tron(root="array")
        tron.init_arr()
        for value in obj:
            _append_value(tron, value, 0)
        return tron
    raise TypeError("root value must be dict or list")


def to_obj(tron: Tron) -> Any:
    """Convert a Tron buffer to Python objects via JSON."""
    return json.loads(tron.to_json())


def _set_value(tron: Tron, key: str, value: Any, ofs: int) -> None:
    if value is None:
        tron.set_null(key, ofs=ofs)
        return
    if isinstance(value, bool):
        tron.set_bool(key, value, ofs=ofs)
        return
    if isinstance(value, int):
        tron.set_i64(key, value, ofs=ofs)
        return
    if isinstance(value, float):
        tron.set_f64(key, value, ofs=ofs)
        return
    if isinstance(value, str):
        tron.set_str(key, value, ofs=ofs)
        return
    if isinstance(value, (bytes, bytearray, memoryview)):
        tron.set_bytes(key, bytes(value), ofs=ofs)
        return
    if isinstance(value, dict):
        child_ofs = tron.set_obj(key, ofs=ofs)
        for child_key, child_value in value.items():
            _set_value(tron, child_key, child_value, child_ofs)
        return
    if isinstance(value, (list, tuple)):
        child_ofs = tron.set_arr(key, ofs=ofs)
        for child_value in value:
            _append_value(tron, child_value, child_ofs)
        return
    raise TypeError(f"unsupported value type: {type(value)!r}")


def _append_value(tron: Tron, value: Any, ofs: int) -> None:
    if value is None:
        tron.arr_append_null(ofs=ofs)
        return
    if isinstance(value, bool):
        tron.arr_append_bool(value, ofs=ofs)
        return
    if isinstance(value, int):
        tron.arr_append_i64(value, ofs=ofs)
        return
    if isinstance(value, float):
        tron.arr_append_f64(value, ofs=ofs)
        return
    if isinstance(value, str):
        tron.arr_append_str(value, ofs=ofs)
        return
    if isinstance(value, (bytes, bytearray, memoryview)):
        tron.arr_append_bytes(bytes(value), ofs=ofs)
        return
    if isinstance(value, dict):
        child_ofs = tron.arr_append_obj(ofs=ofs)
        for child_key, child_value in value.items():
            _set_value(tron, child_key, child_value, child_ofs)
        return
    if isinstance(value, (list, tuple)):
        child_ofs = tron.arr_append_arr(ofs=ofs)
        for child_value in value:
            _append_value(tron, child_value, child_ofs)
        return
    raise TypeError(f"unsupported value type: {type(value)!r}")
