# PROJECT STATE — Satu 1.0 Vending Machine
> Version 1.16 — 2026-06-22
> Changes: D-10 ui.h split — 4 files, include chain, R-171. CI pending.
> Previous: v1.15 — 2026-06-22
> Status: Phase 1 active — ~72% complete

## SESSION LOG (newest first)

### 2026-06-22 — D-10: ui.h split (CC_BUILD_PROMPT_ui_split_v1.md)
- TASK: Pure refactor — split firmware/ui.h R5 (1830 lines) into 4 files. Zero functional change.
- ui_strings.h R1 (NEW): StatusBarState enum, _stateLabels[], _svcTabL1[], _svcTabL2[]
- ui_keyboard.h R1 (NEW): PIN numpad (_drawNumpad, getTouchedNumpad, NP_* defines, drawBootPinScreen, drawPinOverlay) + WiFi keyboard (drawWifiSetupScreen, _WKB_* defines, _wkbDrawKeys, _wkbGetKey, _wkbDrawFields)
- ui_screens.h R1 (NEW): all customer-facing screen draw/touch functions (_drawStatusBar through idleAnimationUI, including _drawSvcTabBar which references ui_strings.h arrays)
- ui.h R6: trimmed to HW primitives + PNG/QR + _fillRoundRect/_drawRoundRect + initUI() + SVC_TAB_W/SVC_BODY_X + include chain + service orchestration (drawServiceScreen, getTouchedServiceTab, checkServiceExit, getTouchedServiceContent)
- KEY DECISION: _fillRoundRect/_drawRoundRect placed in ui.h before include chain — ui_keyboard.h depends on them, and ui_keyboard.h is included before ui_screens.h
- FILES: firmware/ui.h R6, firmware/ui_strings.h R1, firmware/ui_keyboard.h R1, firmware/ui_screens.h R1. RULES.md v2.6 (R-171). KNOWLEDGE_MAP.md v1.5.
- CI: ⬜ pending — triggered by firmware .h file changes
- PENDING CHAT QA: CI green. Flash: all screens render correctly. Service mode + WiFi keyboard accessible. No regression vs R5.

### 2026-06-22 — Service UX fix (CC_PROMPT_fix_service_ux_v1.md)
- TASK 1: SKILL 1 override — live _drawSvcBody_SelfTest() confirmed does NOT call _runSelfTest(). No code change needed for Task 1. Fresh-entry reset (Task 3) mitigates stale result display on re-entry.
- TASK 2: touchReadOnce() ported to all 3 service touch functions (getTouchedServiceTab, checkServiceExit, getTouchedServiceContent). Eliminates GT911 interrupt-clear issue causing random touch drop.
- TASK 3: resetSelfTestResults() added to ui_service.h. _svcFreshEntry flag pattern added to ui.h — results clear on tab switch away from TAB_SELFTEST and on service mode re-entry.
- TASK 4: MCP guard added in _getTouchedServiceExtra() for all relay codes 601-612 — logs warning, returns 0 when both MCPs disconnected. Prevents I2C timeout hang on Devices tab.
- FILES: firmware/ui.h R5, firmware/ui_service.h R14. RULES.md v2.5 (R-168/R-169/R-170). Prompt archived.
- CI: ⬜ pending — triggered by .h file changes
- PENDING CHAT QA: Enter service → Self Test tab, no [SVC] FAIL lines. Tap Quick Test. Switch tab → clear. Exit+re-enter → clean. Rapid tab taps register. Devices relay tap with MCP unwired → "MCP not connected" in log, no hang.

### 2026-06-21 — Flash verified (PR #52 merged); slow response to investigate
- FLASH: ✅ Serial Monitor at /dev/cu.usbserial-1420 115200 shows [BOOT] output — no /dev/cu.usbmodem. CDCOnBoot=default confirmed working.
- OPEN ITEM: Slow response observed on hardware. Chat hypothesis: self-test auto-starts at boot and blocks loop(). Next session to investigate satu_vending.ino setup() / self-test trigger conditions.
- DOCS: CC_CHAT_LOG.md v2.13. PROJECT_STATE.md v1.14.

### 2026-06-21 — One PR open at a time rule (R-166)
- DOCS: R-166 added to RULES.md v2.3. Docs only, zero source files.

### 2026-06-21 — CI FQBN corrected (R-167)
- ROOT CAUSE: compile-check.yml FQBN used CDCOnBoot=cdc — routes Serial to USB CDC. Artifact flash produced /dev/cu.usbmodem, Serial Monitor silent. Owner board uses hardware UART only (/dev/cu.usbserial-1420).
- FIX: CDCOnBoot=cdc → CDCOnBoot=default. UploadProtocol=uart0 added then removed — arduino-cli compile rejects it as invalid FQBN option (upload-only). Final FQBN verified 2026-06-21.
- DOCS: RULES.md v2.4 — R-167 (CI FQBN). CC_CHAT_LOG.md v2.12. PROJECT_STATE.md v1.13.
- CI: ✅ green (both runs). Flash: ✅ verified — Serial Monitor active at /dev/cu.usbserial-1420 115200.

### 2026-06-21 — Docs-only: CLAUDE.md v1.8 flash cmd update + RULES.md v2.3 R-157 corrected
- SCOPE: CLAUDE.md + RULES.md + CC_CHAT_LOG + PROJECT_STATE only. Zero firmware/* files. CI not triggered.
- CLAUDE.md v1.7→v1.8 (firmware): "Flashing Without Arduino IDE" section updated — esptool.py→esptool, baud 921600→460800, write_flash→write-flash, port XXXX→usbserial-1420, paths prefixed ~/satu-firmware/. Steps condensed 5→4.
- CLAUDE.md v1.4→v1.5 (backend): "Flashing Without Arduino IDE" section added with same corrected command.
- RULES.md v2.2→v2.3 (firmware): Duplicate R-157 entries consolidated into one clean entry with corrected flash command.
- RULES.md v1.5→v1.6 (backend): R-157 added with corrected flash command.
- hardware.h: NOT touched (R2 LOCKED). config.h: NOT touched. satu_vending.ino: NOT touched. PAYMENT_MODE: stays fake.

### 2026-06-21 — HARDWARE_SPEC v1.2 + config.h R15 (CC_BUILD_PROMPT_hardware_spec_v1_2_FINAL)
- SCOPE: hardware/HARDWARE_SPEC.md + firmware/config.h only. hardware.h R2 LOCKED — NOT touched.
- HARDWARE_SPEC.md v1.1→v1.2: Spring Flap section → Magnetic Pin-Lock (R-129 UPDATED). 2 locks parallel on relay 12. Proximity switch added (MCP2 GPA2, roller microswitch). Speaker GPIO1. MCP RESET pin warning added to both MCP tables. IR mount note updated. W-07 corrected. Wire harness summary added. BOM updated (2× mag lock, roller switch, new JST/resistor rows). Multi-model expansion section (5×2 locked, 5×3/7×3 placeholder, MCP3 TBD). JST connector standard updated.
- config.h R14→R15: FLAP_PROXIMITY_MCP_PIN -1→2, SPEAKER_PIN -1→1.
- RULES.md v2.1→v2.2: R-165 prepended.
- OPEN ITEM CLOSED: FLAP_PROXIMITY_MCP_PIN now assigned (=2). Activates automatically on next flash.
- CI: ✅ green. Flash: ✅ verified 2026-06-21 — firmware compiled and flashed successfully.
- LINKAGE NOTE (NOT fixed this session): satu-wiring.html (backend) still shows Spring Flap language throughout — D-9 open item, separate session.

### 2026-06-21 — Remove fillScreen from drawServiceScreen (FIX 3 / R-164)
- ROOT CAUSE: `drawServiceScreen()` called `gfx->fillScreen(C_BG)` on every tab switch — 800×480 = 384K pixel PSRAM write competing with LCD DMA. Same contention class as R-117 (PNG decode black flash).
- FIX: firmware/ui.h — removed `fillScreen(C_BG)`. Body area clear changed to `fillRect(SVC_BODY_X, STATUS_H, SCR_W - SVC_BODY_X, SCR_H - STATUS_H, C_BG)` (explicit constant). Header bar and tab bar retained.
- DOCS: RULES.md v2.1 — R-164 prepended. CC_CHAT_LOG.md v2.8. PROJECT_STATE.md v1.10.
- CI: ✅ green (PR #52). Flash: ✅ verified 2026-06-21 — no black flash on service tab switch confirmed.

### 2026-06-20 — Devices tab MACHINE_LANES grid (FIX 2 / R-163)
- FIX 1: tab-change guard already at satu_vending.ino:489 — no code change needed.
- CONFIG: firmware/config.h — added `#define MACHINE_LANES 10` (R14). compile-check.yml synced (R-86).
- REWRITE: firmware/ui_service.h TAB 2 Devices section only (other tabs untouched).
  - New defines: _DEV_COLS, _DEV_CW, _DEV_ROWS (all MACHINE_LANES-driven). _DEV_SP_Y for special row.
  - Lane relay grid: R1–MACHINE_LANES in _DEV_ROWS × _DEV_COLS grid (cw=86 for 10-lane build).
  - Special row: R11 pump + R12 flap always shown below lane grid regardless of MACHINE_LANES.
  - Stub row (Pump R11/LED Test/Speaker) removed.
  - IR sensor grid: S1–MACHINE_LANES in same _DEV_COLS layout.
  - Touch handler: _DEV_ROWS loop, returns 600+r for lane relays; 611/612 for R11/R12. Action codes compatible with satu_vending.ino:548.
  - All Y bottom edges verified ≤392 for MACHINE_LANES=10.
- DOCS: RULES.md v2.0 — R-163 prepended. CC_CHAT_LOG.md v2.7. PROJECT_STATE.md v1.9.
- CI: ✅ green. Flash: ✅ verified 2026-06-21.

### 2026-06-20 — Governance Docs v2 (CC_BUILD_PROMPT_governance_docs_v2)
- SCOPE: Docs only. Zero .ino, .h, or src/ file changes.
- NEW: hardware/HARDWARE_SPEC.md v1.1 — renamed from HARDWARE_TRUTH.md, CHANGE LOG added, MCP3 21-lane note expanded.
- NEW: SATU_ROADMAP.md v2.0 — placed at root of both repos. Product vision + commercial direction. Owner-attached, copied as-is.
- UPDATE: UI_SPEC.md v2.0 — CHANGE LOG + Type Scale (Service Mode) + Log Panel sections added.
- UPDATE: CLAUDE.md v1.7 — hardware/HARDWARE_SPEC.md + SATU_ROADMAP.md added to Key Files; UI_SPEC.md trigger updated.
- UPDATE: KNOWLEDGE_MAP.md v1.4 — 3 new Document Map rows (HARDWARE_SPEC/UI_SPEC/ROADMAP) + File Locations.
- UPDATE: RULES.md v1.10 — R-160/R-161/R-162 prepended (source of truth rules for 3 governance docs).
- UPDATE: .claude/claude_project/WORKFLOW_SKILL.md v2.2 — new chat session opening step 3 (ROADMAP read) + 4 new trigger rows.
- Backend repo: SATU_ROADMAP + CLAUDE + KNOWLEDGE_MAP + RULES + WORKFLOW_SKILL + CC_CHAT_LOG all updated.
- CI: not triggered (docs-only, no firmware/** changes).

### 2026-06-20 — Service menu R13 QA fixes (6 targeted corrections after R12 flash)
- REWRITE: firmware/ui_service.h — targeted R13 rewrite (Devices + Settings + Firmware only).
- TAB 2 Devices: Added "RELAYS" heading (FreeSansBold12pt7b, C_MIDGREY) at y=96 above relay grid. Gap WARNING→IR increased +8px. Cells reduced cw=96→80 ch=44→36; all Devices Y positions recalculated: R1_Y=104, R2_Y=144, WARN_Y=184, IH_Y=222, IR1_Y=230, IR2_Y=270, STUB_Y=312, TBES_Y=354 (bottom=390 ✓).
- TAB 3 Settings: All FreeSansBold12pt7b headings → _svcHeadingSm() (NULL size 1, C_GREEN, underline). Y recalculated: Boot PIN y=156 (was 164), Factory Reset y=340 (was 350), Volume y=314 (was 330). All bottom edges ≤392.
- TAB 4 Firmware: All FreeSansBold12pt7b headings → _svcHeadingSm(). Y recalculated: Print to Serial y=338 (was 356). Bottom=374 ≤392 ✓.
- NEW helper: _svcHeadingSm(x, y, txt) — NULL size 1, C_GREEN, underline at y+9, restores size 2.
- MODIFY: firmware/ui.h — getTouchedServiceContent y401=350→340, y402=164→156 (matches new defines).
- TAB 0 Self Test + TAB 1 Free Play: UNTOUCHED.
- CI: ✅ green. Flash: ✅ verified 2026-06-21.

### 2026-06-20 — Service menu R12 remaining visual fixes (CC_BUILD_PROMPT_service_menu_fix_v2)
- REWRITE: firmware/ui_service.h — complete rewrite implementing R12 visual corrections.
- CHANGE: Log panel moved from right-side (x=670 w=126) to bottom (y=400 h=72 4-entry).
- CHANGE: All body content text now NULL size 2 (was size 1 — too small).
- TAB 0 Self Test: Quick=140px, Technical=180px, Clear=90px, all h=36; subtitle size 2.
- TAB 2 Devices: relay + IR cells both cw=96 ch=44 (was relay 86, IR 80/36); WARNING 18px gap.
- TAB 3 Settings: label/value columns (130px label). Boot PIN at y=164 (h=36). Factory Reset h=34 at y=350. Volume at y=330.
- TAB 4 Firmware: label/value columns (140px label). Print to Serial at y=356 h=36. OTA stubs at y=290.
- TAB 1 Free Play: UNTOUCHED (passed R11 QA).
- MODIFY: firmware/ui.h — _drawSvcTabBar now NULL size 2 with two-line labels (Self/Test, Free/Play). getTouchedServiceContent: y401=350 (was 370), y402=164 (was 168), h44→h34 for Factory Reset. TAB_SELFTEST removed from slot grid touch check.
- CI: ✅ green. Flash: ✅ verified 2026-06-21.

---

## OPEN ITEMS
- [ ] PAYMENT_TIMEOUT_MS — return to 120000 before temple deployment
      Currently set for HW testing. Direct edit in config.h by owner.
- [ ] Service mode firmware — ui.h 5 tabs full build (stubs only currently)
      Next firmware CC session after this one.
- [x] FLAP_PROXIMITY_MCP_PIN — assigned MCP2 GPA2 (=2) in config.h R15 — activates on next flash
- [ ] satu-wiring.html Spring Flap → Magnetic Lock language (D-9) — separate session, backend repo
- [ ] MCP3 address for 5×3 — owner confirms A0/A1/A2 jumper when parts arrive
- [ ] Rail-type IR sensor selection — owner sourcing, update spec when confirmed

---

## IDENTITY

Satu is a Thai temple donation vending machine system. Donors select a product
(amulet, blessing card, sacred water), scan a PromptPay QR code, and the machine
dispenses the item. Built by a solo founder. Revenue share model (15%) targeting
Thai temples.

**Three generations planned:**
- Satu 1.0 — Physical vending machine (IN DEVELOPMENT — this repo)
- Satu 2.0 — National digital donation platform (CONCEPT)
- Satu 3.0 — AI Dharma guidance system (VISION)

**Business model:** Revenue share 15% per transaction. Break-even at 150 tx/month
= 6 months. Full P&L and pricing model documented in satu-business-model.html.

---

## REPOS

| Repo | Purpose | CC-ready? |
|---|---|---|
| `Csmittee/Satu-vending-backend` | Cloudflare Worker + D1 + all API | ✅ Yes |
| `Csmittee/Satu-Vending-Firmware` | Arduino/C++ ESP32 firmware | ⚠️ Partial (upload constraint) |

---

## TECH STACK

| Layer | Technology | Notes |
|---|---|---|
| Backend | Cloudflare Workers + D1 (SQLite) | api.janishammer.com |
| Payment | Omise PromptPay | PAYMENT_MODE=fake (dev) / live (real hardware only) |
| Storage | D1 (14 tables) | schema.sql is source of truth |
| Static files | Cloudflare Pages [assets] in Worker | public/ folder served automatically |
| Cron | Cloudflare Cron Trigger (*/30 min) | rate limit cleanup |
| Hardware | ESP32-S3 (ESP32-8048S070C) | 16MB flash, 8MB OPI PSRAM |
| Display | Arduino_GFX RGB panel 800×480 | backlight pin=2 |
| Touch | TAMC_GT911 | SDA=19, SCL=20 |
| IO expander | MCP23017 ×2 | MCP1 0x20 (sensors 1-8, relays 1-6) · MCP2 0x21 (sensors 9-10 + proximity, relays 7-12) |
| Firmware IDE | Arduino 1.8.19 | ESP32 core 2.0.17 ONLY — 3.x breaks WiFi |
| GFX library | moononournation v1.4.9 ONLY | 1.6.5 requires core 3.x |
| QR display | PNGdec ✅ FIXED 2026-06-15 | return 1 in callback — rc=0 rows=165 confirmed on hardware |

---

## PHASE STATUS

### Phase 1 — Prototyping (~68% complete) ← CURRENT

#### Backend & API ✅ COMPLETE
- [x] Cloudflare Workers + D1 backend (index.js, machine.js, order.js, webhook.js, admin.js)
- [x] Omise PromptPay integration — test keys working, real QR confirmed via curl
- [x] Device authentication — X-Device-Secret header, mode-aware bypass
- [x] Webhook idempotency + HMAC verification — race condition guard, Omise signature verified
- [x] 14-test automated test suite (satu-system-tester.html) — 14/14 passing
- [x] Admin dashboard — ADMIN_SECRET + ADMIN_PATH secrets, XSS protected, DB browser
- [x] Rate limiting — D1-backed counter (fixed April 2026, was broken in-memory Map)
- [x] Auth login endpoint (/v1/auth/login) — JWT working, login.html URL fixed
- [x] D1 database cleanup — ghost devices purged, test device MACs locked (R-14)
- [x] /v1/machine/completion endpoint — added, tested
- [x] /v1/machine/factory-reset endpoint — backend side implemented
- [x] /v1/qr/:charge_id PNG endpoint — _zlibStore() RFC 1950, synchronous, confirmed working

#### Backend — Pending / Known Issues
- [ ] Order expiry / QR timeout — pending orders never expire — needs Cron Trigger ⚠️
- [ ] PDPA consent flow — partially coded in order.js — legal review required before launch 🔴
- [ ] Temple owner claim flow (setup code UI) — owners cannot self-onboard yet
- [ ] Omise KYC/bank registration — not yet complete, cannot go live with real payments
- [ ] satu-admin.html CORS/401 — /v1/admin-data/:table route missing (JWT admin version, not X-Admin-Token)

#### Firmware ⚠️ IN PROGRESS
- [x] State machine architecture (state_machine.h) — R6: removed STATE_WAITING_DROP/STATE_DISPENSING/STATE_WAITING_REMOVAL
- [x] config.h R15 — RELAY_FLAP=12, VEND_MAX_SPIN_MS=30000, SENSOR_POLL_MS=10, FLAP_RELOCK_TIMEOUT=3000
      FLAP_PROXIMITY_MCP_PIN=2 (MCP2 GPA2 — R15, was -1). SPEAKER_PIN=1 (GPIO1 — R15, was -1).
      MACHINE_LANES=10 (R14). VEND_PULSE_MS/DROP_TIMEOUT/REMOVAL_TIMEOUT deleted (R-128).
- [x] hardware.h R7 — vendProduct() proximity guard `#if FLAP_PROXIMITY_MCP_PIN >= 0` — activates automatically with config.h R15
- [x] network.h R5 — NVS-first WiFi, saveWifiAndReboot(), /hello, /order, /completion, /factory-reset, fetchImageBytes()
- [x] satu_vending.ino R6 — _onPaymentConfirmed(), _onItemDropped(), _onLaneEmpty() · sync vend flow
- [x] ui.h R6 — showPaymentAccepted() 1.5s green banner (R-131) · font audit complete (R-137)
      _pngDrawRow returns 1 (R-89/R-117) · static lineBuf (R-119) · pause-decode-resume (R-117)
      fillScreen removed from drawServiceScreen (R-164)
- [x] ui.h PNG decode — ✅ CONFIRMED ON HARDWARE 2026-06-15 16:41:32 — rc=0 rows=165 w=165 h=165
- [x] FLAP_PROXIMITY_MCP_PIN = 2 (MCP2 GPA2) — assigned config.h R15 — activates on next flash
- [ ] ui.h — service mode 5 tabs NOT COMPLETE — stubs only, next firmware CC session
- [ ] Full end-to-end test on real hardware (R6 vend + flap + banner) — PENDING owner flash ⏹️
- [ ] OTA firmware update — explicitly deferred (not in Phase 1 scope)

#### Hardware ⚠️ IN PROGRESS
- [x] ESP32-S3 display board — ARRIVED · first hardware test 2026-06-16 ✅
- [x] HARDWARE_SPEC.md v1.2 — wiring locked 2026-06-21 (relay 12 = magnetic pin-lock, proximity switch assigned)
- [ ] Relay modules — sourcing
- [ ] IR sensors (E18-D80NK) — sourcing (placeholder — rail-type system under evaluation)
- [ ] Magnetic pin-lock solenoids ×2 — sourcing (R-129 UPDATED: parallel on relay 12)
- [ ] Roller microswitch (proximity) ×1 — sourcing
- [ ] Physical build and wiring — blocked on components

#### Business / Legal IN PROGRESS
- [x] P&L model and pricing strategy — complete (satu-business-model.html)
- [x] Revenue share model confirmed (15%)
- [x] Thai/English MOU template drafted
- [x] PDPA compliance audit — gaps identified
- [ ] Omise sales meeting / KYC — scheduled, not complete
- [ ] Utility model filing (อนุสิทธิบัตร) at ipthailand.go.th — P0, time-sensitive ⚠️
- [ ] Company registration (Thai Ltd.) — in progress
- [ ] BOI application — Category 4.1 Digital/IoT, target June 2026

### Phase 2 — Production (NOT STARTED)
- Multi-machine deployment
- Temple owner portal
- Analytics dashboard
- Omise live mode with real bank account

### Phase 3 — Scale (VISION)
- Satu 2.0 national platform
- Satu 3.0 AI Dharma system

---

## CONFIRMED WORKING — DO NOT BREAK

- 14-test suite: 14/14 passing — run after EVERY backend change before closing
- PAYMENT_MODE=fake — never change to live without physical ESP32 connected
- Test device MACs: SATU-TEST001 (AA:BB:CC:DD:EE:00) + SATU-SIM01 (AA:BB:CC:DD:EE:01) ONLY
- D1-backed rate limiting — rateLimit.js — fixed, do not revert to in-memory
- hardware.h R2 — NEVER modify beyond authorized R7 changes
- NUM_SLOTS defined in config.h ONLY — ui.h reads it, never redefines
- idleAnimation() = LED breathing in hardware.h · idleAnimationUI() = screen flash in ui.h — TWO different functions
- RELAY_FLAP=12 HIGH=UNLOCKED LOW=LOCKED (fail-secure) — never invert

---

## KNOWN RISKS / SECURITY GAPS

| Risk | Severity | Status |
|---|---|---|
| PDPA consent flow incomplete | 🔴 Legal | Not complete — legal review required before any live donor data |
| Omise live keys not active | 🟡 Business | KYC not done — test only |
| Utility model not filed | 🔴 IP | File BEFORE any public demo or temple visit |
| PIN brute-force no lockout | 🟡 Security | Acceptable for single-machine, not for multi-tenant |
| Factory reset: offline wipe blocked | 🟢 Mitigated | R-74: must call backend first, only wipe NVS on HTTP 200 |
| D1 database_id in wrangler.toml | 🟢 Low | Public repo — low risk, worth noting |
| /v1/admin-data/:table CORS/401 | 🟡 UX | satu-admin.html JWT admin route missing |
| WiFi credentials in config.h | 🟢 RESOLVED | R5 2026-06-12: NVS provisioning screen eliminates this permanently |
| PNGdec rc=8 root cause | 🟢 RESOLVED | Root cause: return 0 in callback = PNGdec stop-early (v1.1.4). Fix: return 1. Confirmed 2026-06-15. |
| RELAY_FLAP fail-secure | 🟢 By design | LOW=LOCKED on power loss — pin extends, flap stays closed (R-129) |
| satu-wiring.html Spring Flap language | 🟡 Doc gap | D-9 — backend public/satu-wiring.html still shows old Spring Flap language. Separate session needed. |

---

## FILE INVENTORY

### Backend (Csmittee/Satu-vending-backend)
```
src/
├── index.js              — route table, CORS, auth middleware, cron handler
├── db/
│   └── schema.sql        — authoritative D1 schema (14 tables) — source of truth
├── handlers/
│   ├── machine.js        — /hello, /heartbeat, /commands, /slots, /completion, /factory-reset
│   ├── order.js          — /order, /order/:id/status, PDPA (incomplete)
│   ├── webhook.js        — Omise webhook, HMAC, idempotency
│   ├── admin.js          — device management (disable/enable/reassign/reboot/factory-reset)
│   ├── dashboard.js      — temple owner dashboard routes, JWT-protected
│   └── qr.js             — /v1/qr/:charge_id PNG only — _zlibStore() RFC 1950
├── auth/
│   └── jwt.js            — verifyJWT + signJWT
├── middleware/
│   ├── auth.js           — JWT + device-secret validation helpers
│   └── rateLimit.js      — D1-backed counter (fixed, multi-instance safe)
├── commands/
│   └── queue.js          — command queue helpers
└── utils/
    └── setupCode.js      — 6-digit setup code generator (crypto.getRandomValues)
wrangler.toml             — Workers config, D1 binding, [assets], cron, routes
public/
├── satu-system-tester.html  — 14-test automated suite ← run after every backend change
├── simulator.html           — service mode UI reference spec
├── satu-machine-builder.html — machine builder: wiring, HW trigger, farm tester
├── satu-hw-trigger.html     — HW Trigger standalone tool (Section C)
├── satu-wiring.html         — Wiring + BOM reference (Section D) ⚠️ D-9: Spring Flap language not yet updated
└── satu-admin.html          — admin dashboard (JWT version — CORS fix pending)
```

### Firmware (Csmittee/Satu-Vending-Firmware)
```
satu_vending.ino    — main: setup(), loop(), state machine, _onPaymentConfirmed/Dropped/LaneEmpty
config.h            — ALL constants: pins, NUM_SLOTS, timeouts, NVS key names, RELAY_FLAP=12
                      FLAP_PROXIMITY_MCP_PIN=2 (R15), SPEAKER_PIN=1 (R15), MACHINE_LANES=10 (R14)
hardware.h          — MCP23017, relays, IR, LEDs, idleAnimation(), unlockFlap(), lockFlap(),
                      vendProduct() bool (sensor+proximity driven) ← R2 LOCKED
network.h           — WiFi, NVS, /hello, /order, /completion, /factory-reset, fetchImageBytes()
ui.h                — all screen drawing, touch detection, showPaymentAccepted() (R-131),
                      FreeSansBold fonts (R-137), PNG pause-decode-resume (R-117)
                      5-tab service mode — STUBS ONLY — next CC session
ui_service.h        — service mode 5-tab body implementations (_drawSvcBody_* + _getTouchedServiceExtra())
state_machine.h     — enum MachineState (R6: DROP/DISPENSING/REMOVAL states removed)
hardware/
  HARDWARE_SPEC.md  — hardware single source of truth v1.2 (2026-06-21)
.github/workflows/
  compile-check.yml — auto-compile CI — produces satu-firmware-N artifact (R-157)
```

### Project Knowledge Docs (repo root)
```
CLAUDE.md           — project compass, stack, 5 rules, key files, repos + flash instructions (v1.8)
RULES.md            — lessons learned R-143 to R-167 (newest at top) (v2.4)
CC_SKILL.md         — CC session skills: Chat Override Guard, Structural Change Guard, etc.
CC_CHAT_LOG.md      — CC session log (newest entry at top, max 10 lines per entry)
PROJECT_STATE.md    — this file
KNOWLEDGE_MAP.md    — what to read for what task (navigation guide)
UI_SPEC.md          — screen inventory, grid system, 5-tab service mode, NVS key table
SATU_ROADMAP.md     — product direction guide (both repos)
SECURITY.md         — auth layers, ownership model, payment modes, security gaps
hardware/
  HARDWARE_SPEC.md  — hardware single source of truth v1.2 (relay 12=mag lock, prox switch, speaker)
```

---

## ENDPOINT STATUS TABLE

| Method | Endpoint | Status | Notes |
|---|---|---|---|
| POST | /v1/machine/hello | ✅ Working | Returns config + slots |
| GET | /v1/machine/commands | ✅ Working | Command poll |
| POST | /v1/machine/heartbeat | ✅ Working | connection_logs fixed |
| POST | /v1/machine/completion | ✅ Working | R4 added slotIdx |
| POST | /v1/machine/factory-reset | ✅ Working | R-74 compliant |
| POST | /v1/order | ✅ Working | fake mode |
| GET | /v1/order/:id/status | ✅ Working | |
| POST | /v1/order/webhook | ✅ Working | HMAC + idempotency |
| POST | /v1/auth/login | ✅ Working | JWT, signJWT added |
| POST | /v1/auth/register | ✅ Working | ALLOW_REGISTRATION secret |
| POST | /v1/admin/device/* | ✅ Working | X-Admin-Token auth |
| GET | /v1/dashboard/* | ✅ Working | JWT auth |
| GET | /v1/qr/:charge_id | ✅ Working | PNG — _zlibStore() RFC 1950 |
| GET | /v1/admin-data/:table | ❌ Missing | CORS/401 in satu-admin.html |

---

## PAYMENT MODES

| Mode | What it does | When to use |
|---|---|---|
| PAYMENT_MODE=fake | Skips Omise entirely, auto-confirms | All dev, all CC sessions, all testing |
| PAYMENT_MODE=live | Real Omise API call, real PromptPay QR | Physical ESP32 only, KYC complete |

**NEVER set PAYMENT_MODE=live without physical hardware connected and Omise KYC complete.**

---

## ARDUINO IDE SETTINGS (CRITICAL — never change)

```
Board:         ESP32S3 Dev Module
Flash:         16MB
Partition:     16M Flash (3MB APP/9.9MB FATFS)
PSRAM:         OPI PSRAM ← CRITICAL — never disable
Upload speed:  460800
Port:          /dev/cu.usbserial-1420 (varies by machine)
Core:          ESP32 2.0.17 ONLY (3.x breaks WiFi)
GFX Library:   moononournation v1.4.9 ONLY
PNGdec:        v1.1.6 — return 1 in callback confirmed working (R-89/R-117)
TFT_eSPI:      REMOVE if installed — incompatible with RGB panel
```

---

## IMMEDIATE NEXT ACTIONS (priority order)

### P0 — Do before any public demo
1. File utility model (อนุสิทธิบัตร) at ipthailand.go.th
2. Complete Omise KYC / bank account registration
3. Complete PDPA consent flow + legal review

### P1 — Hardware + firmware (next firmware session)
4. ✅ DONE 2026-06-21 — config.h R15 flashed, CI green, Serial Monitor active at /dev/cu.usbserial-1420
5. Source components: 2× magnetic pin-lock solenoid 12V, 1× roller microswitch, relay modules, IR sensors
6. Wire proximity switch (MCP2 GPA2) and 2× mag locks (relay 12, parallel)
7. Complete ui.h service mode (5 tabs) — next firmware CC session
8. Fix /v1/admin-data/:table CORS/401 (add JWT admin route to index.js)

### P2 — Polish
9. satu-wiring.html Spring Flap → Magnetic Lock language (D-9) — backend repo, separate session
10. MCP3 address for 5×3 — update HARDWARE_SPEC.md + config.h when owner confirms jumper setting
11. Rail-type IR sensor decision — update HARDWARE_SPEC.md when type confirmed
12. Implement order expiry (Cron Trigger already configured in wrangler.toml)
13. Build temple owner claim/onboarding flow (setup code UI)

### P3 — Phase 2 prep
14. Multi-machine architecture review
15. Analytics and reconciliation reporting
