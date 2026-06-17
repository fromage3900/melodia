"""Smoke tests for generators."""


def test_naming():
    from core.naming import enforce_sm_prefix, sanitize_name
    assert enforce_sm_prefix("test") == "SM_test"
    assert enforce_sm_prefix("SM_test") == "SM_test"
    print("PASS: SM_ prefix")


def test_sanitize():
    from core.naming import sanitize_name
    assert sanitize_name("hello world!") == "hello_world_"
    print("PASS: sanitize name")


def test_batch_parse():
    from batch.layout_parser import parse_layout
    data = {"type": "elements", "elements": [{"generator": "stem_column"}]}
    result = parse_layout(data)
    assert len(result["elements"]) == 1
    print("PASS: batch parse elements")


if __name__ == "__main__":
    test_naming()
    test_sanitize()
    test_batch_parse()
    print("All tests passed.")
