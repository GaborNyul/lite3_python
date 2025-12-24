import random

from tron import DJB2_HASH_SEED, Tron

ALPHANUMS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"


def djb2_hash(key_bytes):
    h = DJB2_HASH_SEED
    for b in key_bytes:
        h = ((h << 5) + h + b) & 0xFFFFFFFF
    return h


def test_collisions():
    random.seed(52073821)

    tron = Tron()

    key_len = 2
    key_size = key_len + 1
    key_arr_size = 1024 * 1024 * key_size

    key_arr = bytearray(key_arr_size)
    for i in range(key_arr_size):
        key_arr[i] = ord(random.choice(ALPHANUMS))

    colliding_keys = []
    prev_key = key_arr[0:key_len]
    prev_hash = djb2_hash(prev_key)

    for i in range(0, key_arr_size, key_size):
        curr_key = key_arr[i : i + key_len]
        curr_hash = djb2_hash(curr_key)
        if prev_hash == curr_hash and prev_key != curr_key:
            key_a = prev_key.decode("ascii")
            key_b = curr_key.decode("ascii")
            tron.set_null(key_a)
            tron.set_null(key_b)
            colliding_keys.append(key_a)
            colliding_keys.append(key_b)
        prev_hash = curr_hash
        prev_key = curr_key

    for key in colliding_keys:
        assert tron.exists(key) is True
