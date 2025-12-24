import json

from tron import Tron, TronDocument, from_obj


def example_basic():
    tron = Tron()

    tron.set_str("event", "lap_complete")
    tron.set_i64("lap", 55)
    tron.set_f64("time_sec", 88.427)
    tron.set_bool("fastest_lap", True)
    tron.set_bytes("raw_payload", b"\x01\x02\x03")

    tron.delete("fastest_lap")

    tron.save("example_basic.tron")
    reloaded = Tron.from_file("example_basic.tron")
    print("Example 1 (basic) as JSON:")
    print(reloaded.to_json(pretty=True))


def example_from_json_dict():
    payload = {
        "user": {
            "id": 12345,
            "name": "Jane Doe",
        },
        "roles": ["admin", "editor"],
        "active": True,
        "quota": 12.5,
    }

    json_str = json.dumps(payload)
    tron = Tron.from_json(json_str)
    tron.save("example_from_json.tron")
    reloaded = Tron.from_file("example_from_json.tron")

    print("\nExample 2 (JSON round-trip) as JSON:")
    print(reloaded.to_json(pretty=True))


def example_features():
    tron = Tron()

    headers_ofs = tron.set_obj("headers")
    tron.set_str("content-type", "application/json", ofs=headers_ofs)
    tron.set_str("x-request-id", "req_9f8e2a", ofs=headers_ofs)

    tags_ofs = tron.set_arr("tags")
    tron.arr_append_str("alpha", ofs=tags_ofs)
    tron.arr_append_str("beta", ofs=tags_ofs)
    tron.arr_append_bool(True, ofs=tags_ofs)

    print("\nExample 3 (features) exists:", tron.exists("headers"))
    print("headers type:", tron.get_type("headers"))
    print("tags[0]:", tron.arr_get_str(0, ofs=tags_ofs))
    print("raw buflen:", tron.buflen())
    print("as JSON:")
    print(tron.to_json(pretty=True))


def example_python_ergonomics():
    payload = {
        "request": {
            "method": "POST",
            "duration_ms": 47,
            "headers": {
                "content-type": "application/json",
                "x-request-id": "req_9f8e2a",
            },
        },
        "flags": [True, False, None, 5, 7.5, "alpha"],
    }

    tron = from_obj(payload)
    print("\nExample 4 (from_obj) as JSON:")
    print(tron.to_json(pretty=True))

    doc = TronDocument.from_obj(payload)
    print("\nExample 5 (TronDocument to_obj):")
    print(doc.to_obj())


def main():
    example_basic()
    example_from_json_dict()
    example_features()
    example_python_ergonomics()


if __name__ == "__main__":
    main()
