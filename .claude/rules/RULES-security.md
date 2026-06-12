# RULES-security.md — Satu 1.0
> Domain: Auth, secrets, payment security, legal compliance, ownership model, factory reset
> Load this file when: Any auth/device-secret · factory reset · ownership · payment · admin routes
> Last updated: 2026-06-11
---

- R-45: device_events D1 table required for audit trail — log: claimed, released, suspended, nuked, factory_reset
- R-44: Four security layers: L1 machine PIN (owner) · L2 dashboard login (owner) · L3 device_secret (us) · L4 admin (us only)
- R-43: Factory reset = call backend first · ownership = AirTag-style binding · nuke = remote NVS wipe via command
- R-42: PDPA consent flow is incomplete — legal review required before any live donor data collected
- R-41: All device auth uses device_id + device_secret pair — validate both on every call
- R-40: ADMIN_SECRET + ADMIN_PATH are secrets — never in source code or logs
