# ✅ COMPLETE — Archived 2026-06-13 — GitHub Actions compile check workflow created
# CC_BUILD_PROMPT_github_actions_compile.md
# Satu Firmware — GitHub Actions Auto-Compile Check
# Created: 2026-06-12
# Target repo: https://github.com/Csmittee/Satu-Vending-Firmware
# Branch: feature/github-actions-compile
# ============================================================

## CC INTRO
New session. Ignore all previous context.
Repo: https://github.com/Csmittee/Satu-Vending-Firmware
Read CLAUDE.md and RULES.md first. State both read before touching anything.

---

## CONTEXT

Owner uses browser-based CC only — no local terminal CC.
Mac is Intel, 4GB RAM, Big Sur — cannot run arduino-cli locally.
Every compile error is discovered only after owner manually opens
Arduino IDE and waits 10+ minutes. Today alone: 3 compile loops,
30+ minutes lost on missing extern declarations and wrong callback type.

Goal: every PR and every push to main automatically compiles the
firmware in GitHub's cloud. CC sees red/green in under 3 minutes.
Owner sees it too. No Arduino IDE needed to catch compile errors.

---

## TASK — Create GitHub Actions workflow

### File to create:
`.github/workflows/compile-check.yml`

### What it must do:
1. Trigger on: every push to any branch, every pull request to main
2. Install arduino-cli
3. Install ESP32 core 2.0.17 ONLY — never any other version
4. Install all 6 required libraries at exact locked versions
5. Create a minimal config.h with empty credentials (compile only — no flash)
6. Compile firmware/satu_vending/ 
7. Report pass or fail on the PR/commit

### Exact board FQBN (never change this):
```
esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadSpeed=460800
```

### Exact library versions (never change these — locked in RULES.md):
```
Arduino_GFX_Library @ 1.4.9
TAMC_GT911          @ latest
PNGdec              @ 1.1.6
ArduinoJson         @ 7.x (latest 7)
Adafruit MCP23X17   @ 2.x (latest 2)
FastLED             @ 3.x (latest 3)
```

### config.h for CI (compile-only — empty credentials, correct):
```cpp
#ifndef CONFIG_H
#define CONFIG_H
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";
const char* API_BASE_URL  = "https://api.janishammer.com";
#define NUM_SLOTS      21
#define NUM_COLS        7
#define MAX_SLOTS_HW   21
#define I2C_SDA        19
#define I2C_SCL        20
#define UART_RX        44
#define UART_TX        43
#define LED_DATA_PIN    5
#define TFT_BL          2
#define MCP1_ADDR    0x20
#define MCP2_ADDR    0x21
#define MCP3_ADDR    0x22
extern const int mcp1_sensors[8];
extern const int mcp1_relays[6];
extern const int mcp2_sensors[2];
extern const int mcp2_relays[6];
#define RELAY_ON          HIGH
#define RELAY_OFF         LOW
#define SENSOR_TRIGGERED  LOW
#define SENSOR_CLEAR      HIGH
#define VEND_PULSE_MS     800
#define PAYMENT_TIMEOUT   120000
#define VEND_TIMEOUT       10000
#define DROP_TIMEOUT        5000
#define REMOVAL_TIMEOUT    30000
#define HEARTBEAT_INTERVAL 300000
#define NUM_LEDS           40
#define LED_BRIGHTNESS     128
#define ZONE_TOP_START      0
#define ZONE_TOP_END        9
#define ZONE_FLOOR1_START  10
#define ZONE_FLOOR1_END    19
#define ZONE_FLOOR2_START  20
#define ZONE_FLOOR2_END    29
#define ZONE_DOOR_START    30
#define ZONE_DOOR_END      39
#endif
```

### Complete workflow file to write:
```yaml
name: Firmware Compile Check

on:
  push:
    branches: ['**']
  pull_request:
    branches: [main]

jobs:
  compile:
    name: Compile ESP32-S3 Firmware
    runs-on: ubuntu-latest
    timeout-minutes: 15

    steps:
      - name: Checkout repo
        uses: actions/checkout@v4

      - name: Install arduino-cli
        run: |
          curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
          echo "$HOME/bin" >> $GITHUB_PATH

      - name: Configure arduino-cli
        run: |
          arduino-cli config init
          arduino-cli config set board_manager.additional_urls \
            https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

      - name: Install ESP32 core 2.0.17 (locked — never change)
        run: |
          arduino-cli core update-index
          arduino-cli core install esp32:esp32@2.0.17

      - name: Install libraries (locked versions)
        run: |
          arduino-cli lib install "Arduino_GFX_Library"@1.4.9
          arduino-cli lib install "PNGdec"@1.1.6
          arduino-cli lib install "TAMC_GT911"
          arduino-cli lib install "ArduinoJson"
          arduino-cli lib install "Adafruit MCP23X17"
          arduino-cli lib install "FastLED"

      - name: Create config.h for CI (empty credentials — compile only)
        run: |
          cat > firmware/satu_vending/config.h << 'CONFEOF'
          #ifndef CONFIG_H
          #define CONFIG_H
          const char* WIFI_SSID     = "";
          const char* WIFI_PASSWORD = "";
          const char* API_BASE_URL  = "https://api.janishammer.com";
          #define NUM_SLOTS      21
          #define NUM_COLS        7
          #define MAX_SLOTS_HW   21
          #define I2C_SDA        19
          #define I2C_SCL        20
          #define UART_RX        44
          #define UART_TX        43
          #define LED_DATA_PIN    5
          #define TFT_BL          2
          #define MCP1_ADDR    0x20
          #define MCP2_ADDR    0x21
          #define MCP3_ADDR    0x22
          extern const int mcp1_sensors[8];
          extern const int mcp1_relays[6];
          extern const int mcp2_sensors[2];
          extern const int mcp2_relays[6];
          #define RELAY_ON          HIGH
          #define RELAY_OFF         LOW
          #define SENSOR_TRIGGERED  LOW
          #define SENSOR_CLEAR      HIGH
          #define VEND_PULSE_MS     800
          #define PAYMENT_TIMEOUT   120000
          #define VEND_TIMEOUT       10000
          #define DROP_TIMEOUT        5000
          #define REMOVAL_TIMEOUT    30000
          #define HEARTBEAT_INTERVAL 300000
          #define NUM_LEDS           40
          #define LED_BRIGHTNESS     128
          #define ZONE_TOP_START      0
          #define ZONE_TOP_END        9
          #define ZONE_FLOOR1_START  10
          #define ZONE_FLOOR1_END    19
          #define ZONE_FLOOR2_START  20
          #define ZONE_FLOOR2_END    29
          #define ZONE_DOOR_START    30
          #define ZONE_DOOR_END      39
          #endif
          CONFEOF

      - name: Compile firmware
        run: |
          arduino-cli compile \
            --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadSpeed=460800 \
            --warnings default \
            firmware/satu_vending/

      - name: Compile result
        if: always()
        run: echo "Compile finished — check step above for errors"
```

---

## ALSO UPDATE RULES.md

Append at top (next R-number):
```
R-[N]: GITHUB ACTIONS COMPILE — every firmware PR triggers automatic
       compile check via .github/workflows/compile-check.yml.
       CC must NEVER open a firmware PR without waiting for the
       GitHub Actions check to go green first.
       If it goes red: fix the error, push to same branch, wait for
       green, THEN notify owner the PR is ready.
       Owner should never see a red compile on main.

R-[N+1]: CI CONFIG — GitHub Actions creates its own config.h at
          compile time from the locked template in the workflow file.
          Never store real credentials in the workflow file.
          The CI config.h is identical to the repo config.h (empty creds).
```

## ALSO UPDATE PROJECT_STATE.md
Add under infrastructure:
```
GitHub Actions compile check: ACTIVE
Triggers on: all pushes + all PRs to main
Est. compile time: 3-5 minutes (vs 10+ min local)
Board: ESP32S3 | Core: 2.0.17 locked | Libraries: locked versions
```

## ALSO UPDATE WORKFLOW_SKILL.md
Add to CC firmware loop:
```
After CC pushes firmware changes to a branch:
1. Wait for GitHub Actions to complete (3-5 min)
2. If red → fix and push again → wait for green
3. Only open PR / notify owner when Actions shows green ✅
4. State in PR body: "GitHub Actions compile: ✅ GREEN"
Owner never needs to open Arduino IDE to catch compile errors.
```

---

## QA CHECKLIST (CC must verify before opening PR)

- [ ] .github/workflows/compile-check.yml created
- [ ] ESP32 core version is exactly 2.0.17 — not latest, not 3.x
- [ ] Arduino_GFX_Library version is exactly 1.4.9
- [ ] PNGdec version is exactly 1.1.6
- [ ] PSRAM=opi in FQBN — never change this
- [ ] config.h created by CI step has empty WIFI_SSID and WIFI_PASSWORD
- [ ] config.h is NOT committed to repo — created at runtime by workflow
- [ ] hardware.h untouched — R2 LOCKED
- [ ] R-[N] and R-[N+1] added to RULES.md
- [ ] WORKFLOW_SKILL.md updated with new CC firmware loop
- [ ] PROJECT_STATE.md updated

---

## MANDATORY AT END
1. Open PR: feature/github-actions-compile
2. PR body must include:
   "After merge — every future firmware PR will show ✅ or ❌
    compile status automatically. CC waits for green before
    notifying owner. Owner never runs Arduino IDE to catch
    compile errors again."
3. Archive this prompt → docs/prompts/ stamped ✅ COMPLETE
4. Do NOT touch any firmware .h or .ino files this session

## PAYMENT MODE REMINDER
PAYMENT_MODE = fake. No backend files touched.
