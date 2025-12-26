import json
import time
from dataclasses import dataclass
from pathlib import Path

from tron import Tron, TronDocument, from_obj


@dataclass
class TimingResult:
    name: str
    read_s: float
    parse_s: float
    tron_s: float
    save_s: float
    total_s: float
    output_path: Path


def _read_json_text(path: Path) -> tuple[str, float]:
    start = time.perf_counter()
    text = path.read_text(encoding="utf-8")
    end = time.perf_counter()
    return text, end - start


def _parse_json(text: str) -> tuple[object, float]:
    start = time.perf_counter()
    obj = json.loads(text)
    end = time.perf_counter()
    return obj, end - start


def _build_tron(builder, *args, **kwargs) -> tuple[object, float]:
    start = time.perf_counter()
    tron_obj = builder(*args, **kwargs)
    end = time.perf_counter()
    return tron_obj, end - start


def _save_tron(tron_obj: object, path: Path) -> float:
    start = time.perf_counter()
    tron_obj.save(str(path))
    end = time.perf_counter()
    return end - start


def _run_variant_from_obj(json_path: Path, output_path: Path, name: str, builder) -> TimingResult:
    text, read_s = _read_json_text(json_path)
    obj, parse_s = _parse_json(text)
    tron_obj, tron_s = _build_tron(builder, obj)
    save_s = _save_tron(tron_obj, output_path)
    total_s = read_s + parse_s + tron_s + save_s
    return TimingResult(name, read_s, parse_s, tron_s, save_s, total_s, output_path)


def _run_variant_from_json(json_path: Path, output_path: Path, name: str, builder) -> TimingResult:
    text, read_s = _read_json_text(json_path)
    tron_obj, tron_s = _build_tron(builder, text)
    save_s = _save_tron(tron_obj, output_path)
    total_s = read_s + tron_s + save_s
    return TimingResult(name, read_s, 0.0, tron_s, save_s, total_s, output_path)


def _format_seconds(value: float) -> str:
    return f"{value:.6f}"


def _format_table(
    rows: list[tuple[str, ...]],
    *,
    header_left: bool = True,
    separator: str = "  ",
    left_align_cols: set[int] | None = None,
) -> list[str]:
    if left_align_cols is None:
        left_align_cols = set()
    widths = [0] * len(rows[0])
    for row in rows:
        for idx, cell in enumerate(row):
            widths[idx] = max(widths[idx], len(cell))
    formatted_rows = []
    for row_idx, row in enumerate(rows):
        formatted_cells = []
        for idx, cell in enumerate(row):
            if idx in left_align_cols:
                formatted_cells.append(cell.ljust(widths[idx]))
            elif header_left and row_idx == 0:
                formatted_cells.append(cell.ljust(widths[idx]))
            else:
                formatted_cells.append(cell.rjust(widths[idx]))
        formatted_rows.append(separator.join(formatted_cells))
    return formatted_rows


def main() -> None:
    base_dir = Path(__file__).resolve().parent
    json_path = base_dir / "large.json"
    out_dir = base_dir / "out"
    out_dir.mkdir(exist_ok=True)

    results = [
        _run_variant_from_obj(
            json_path,
            out_dir / "large_from_obj.tron",
            "from_obj (Tron)",
            from_obj,
        ),
        _run_variant_from_obj(
            json_path,
            out_dir / "large_document_from_obj.tron",
            "from_obj (TronDocument)",
            TronDocument.from_obj,
        ),
        _run_variant_from_json(
            json_path,
            out_dir / "large_from_json.tron",
            "from_json (Tron)",
            Tron.from_json,
        ),
    ]

    header = (
        "variant",
        "read_s",
        "json_parse_s",
        "tron_build_s",
        "save_s",
        "total_s",
        "output",
    )
    rows = [header]
    for result in results:
        rows.append((
            result.name,
            _format_seconds(result.read_s),
            _format_seconds(result.parse_s),
            _format_seconds(result.tron_s),
            _format_seconds(result.save_s),
            _format_seconds(result.total_s),
            str(result.output_path),
        ))
    for line in _format_table(rows, header_left=True, separator="  ", left_align_cols={0, 6}):
        print(line)


if __name__ == "__main__":
    main()
