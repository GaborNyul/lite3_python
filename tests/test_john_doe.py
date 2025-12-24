import pytest

from tron import Tron


def test_john_doe():
    tron = Tron()

    tron.set_i64("user_id", 12345)
    tron.set_str("username", "jdoe")
    tron.set_str("email_address", "jdoe@example.com")
    tron.set_bool("is_active", True)
    tron.set_f64("account_balance", 259.75)
    tron.set_str("signup_date_str", "2023-08-15")
    tron.set_str("last_login_date_iso", "2025-09-13T13:20:00Z")
    tron.set_i64("birth_year", 1996)
    tron.set_str("phone_number", "+14155555671")
    tron.set_str("preferred_language", "en")
    tron.set_str("time_zone", "Europe/Berlin")
    tron.set_i64("loyalty_points", 845)
    tron.set_f64("avg_session_length_minutes", 14.3)
    tron.set_bool("newsletter_subscribed", False)
    tron.set_str("ip_address", "192.168.0.42")
    tron.set_null("notes")

    assert tron.get_i64("user_id") == 12345
    assert tron.get_str("username") == "jdoe"
    assert tron.get_str("email_address") == "jdoe@example.com"
    assert tron.get_bool("is_active") is True
    assert tron.get_f64("account_balance") == pytest.approx(259.75)
    assert tron.get_str("signup_date_str") == "2023-08-15"
    assert tron.get_str("last_login_date_iso") == "2025-09-13T13:20:00Z"
    assert tron.get_i64("birth_year") == 1996
    assert tron.get_str("phone_number") == "+14155555671"
    assert tron.get_str("preferred_language") == "en"
    assert tron.get_str("time_zone") == "Europe/Berlin"
    assert tron.get_i64("loyalty_points") == 845
    assert tron.get_f64("avg_session_length_minutes") == pytest.approx(14.3)
    assert tron.get_bool("newsletter_subscribed") is False
    assert tron.get_str("ip_address") == "192.168.0.42"
    assert tron.get_type("notes") == "null"
