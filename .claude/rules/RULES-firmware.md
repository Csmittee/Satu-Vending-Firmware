# RULES-firmware.md — Satu 1.0
> Domain: Arduino/ESP32 firmware, library versions, NVS, UI, service mode, compile constraints
> Load this file when: Any firmware file change · compile errors · flash issues · UI/service mode · NVS
> Last updated: 2026-06-11
---

## R4 Firmware Rules (added 2026-05-31)
- R-82: (reserved — see firmware RULES.md for latest R4 additions)
- R-81: Simulator is the UI spec for service mode — match it exactly, then add R4 additions on top
- R-80: Service mode Settings has Volume slider (NVS key: vol, 0-100), ID Card Reader toggle (nvs_idc)
- R-79: Service mode Settings shows lane prices as READ-ONLY from g_slots[] — edit via dashboard only (backend = single source of truth)
- R-78: Side tabs A/B/C appear only when g_grid_rows >= 3 — slot labels A1-A7, B1-B7, C1-C7
- R-77: Grid system = runtime variables g_grid_rows + g_grid_cols from /hello config — NOT compile-time constants
- R-76: PNG for QR display via PNGdec library (bitbank2) — buffer in PSRAM via ps_malloc(200*1024) — never on stack
- R-75: config.h is in .gitignore — WiFi credentials NEVER in git — use config.h.example template in repo
- R-74: Factory reset MUST call /v1/machine/factory-reset backend first — only wipe NVS on HTTP 200 — offline reset is BLOCKED
- R-73: NVS keys: use ONLY approved keys in UI_SPEC.md NVS table — no new keys, all must be ≤15 chars
- R-72: NUM_SLOTS defined in config.h ONLY — ui.h reads it, never redefines it; same for RELAY_PUMP and RELAY_DOOR_LOCK
- R-71: idleAnimation() = LED breathing (hardware.h) · idleAnimationUI() = screen gold flash (ui.h) — two different functions, never alias or merge
- R-70: hardware.h is R2 LOCKED — never open, modify, or redeclare anything it owns (idleAnimation, mcp2_sensors, RELAY_PUMP, RELAY_DOOR_LOCK)

## Library + IDE Rules (added 2026-05-29)
- R-68: PNGdec library (bitbank2) required for R4 — install via Library Manager before flashing
- R-67: Slot grid default = 10 (5×2) · max 21 (7×3) · A/B/C tabs when R≥3
- R-66: PAYMENT_GATEWAY + SYSTEM_MODE + FAKE_OMISE_URL = plain Variables not Secrets
- R-65: Omise gateway = 3 modes: fake_omise (dev) / omise_test (real QR) / omise_live (KYC done)
- R-64: /hello body field = "firmware" NOT "firmware_version" — backend expects exact name
- R-63: Arduino sketch folder = let IDE create it via File→New, NEVER create manually in Finder
- R-62: TFT_eSPI = REMOVE if installed — incompatible with RGB panel, causes compile errors
- R-61: GFX Library for Arduino (moononournation) = 1.4.9 ONLY — 1.6.5 requires core 3.x
- R-60: ESP32 Arduino core = 2.0.17 ONLY — 3.x breaks WiFi library completely

## Firmware Rules
- R-24: hardware.h abstraction supports both single and dual ESP32 — don't hardwire single-only logic
- R-23: Heartbeat every 5min · command poll every 30s · these are NOT negotiable timing values
- R-22: IR sensor E18-D80NK = NPN normally-open · SENSOR_TRIGGERED = LOW · mount 5-8cm below shelf
- R-21: device_id + device_secret must be persisted in NVS after /hello — never lost on reboot
- R-20: Variable declarations inside switch/case blocks MUST have braces `{}` — compile error otherwise
