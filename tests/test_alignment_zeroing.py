from tron import LITE3_NODE_SIZE, LITE3_ZERO_MEM_8, Tron


def test_alignment_zeroing():
    tron = Tron(bufsz=1024)

    tron.debug_fill(0xEE)
    tron.init_obj()

    tron.set_obj("a")
    buf = tron.to_bytes()
    assert buf[LITE3_NODE_SIZE] == LITE3_ZERO_MEM_8

    tron.debug_fill(0xEE)
    tron.init_obj()

    tron.set_str("key1", "val1")
    test_buflen = tron.buflen()

    tron.set_obj("key1")
    buf = tron.to_bytes()
    assert buf[test_buflen] == LITE3_ZERO_MEM_8
    assert buf[test_buflen + 1] == LITE3_ZERO_MEM_8
