# Driver Repository Naming Convention

All hardware driver repositories (individual git repos for ICs, modules, or utilities) **SHALL** follow this naming convention for clarity and easy visual grouping.

## Format

<3‑letter category prefix in uppercase>_<specific part or function name>

Example: `SNS_MAX31855`

## Prefix Table

| Prefix | Category          | Example repository name         |
|--------|-------------------|----------------------------------|
| SNS    | Sensor            | `SNS_MAX31855`, `SNS_BME280`     |
| DSP    | Display           | `DSP_SSD1306`, `DSP_ILI9341`     |
| MEM    | Memory / Storage  | `MEM_W25Q128`, `MEM_SD_CARD`     |
| AUD    | Audio             | `AUD_MAX98357`, `AUD_CS4344`     |
| COM    | Communication / I/O | `COM_MCP23017`, `COM_TCA9548A`   |
| WRL    | Wireless          | `WRL_ESP8266` (peripheral), `WRL_HM10` |
| PWR    | Power Management  | `PWR_AP2112`, `PWR_MAX17048`     |
| <ED_SYS> | Utility / Common  |     |
| LOG    | Logging           | `LOG_PERSIST`, `LOG_FLASH`       |
| SYS    | System / RTOS     | `SYS_WATCHDOG`, `SYS_TASKMON`    |
| SEC    | Security / Crypto | `SEC_ATECC608`, `SEC_SHA256`     |
| MTR    | Motor / Actuator  | `MTR_DRV8825`, `MTR_SERVO`       |
| LED    | LED / Lighting    | `LED_WS2812`, `LED_IS31FL3731`   |
| KEY    | Keypad / Button   | `KEY_MATRIX`, `KEY_CAP1203`      |
| RTC    | Real‑Time Clock   | `RTC_DS3231`, `RTC_PCF8563`      |

## Why Uppercase 3‑Letter Prefixes?

- **Quick visual scanning** – Prefix stands out in a flat list of repositories.
- **Consistent length** – Aligns nicely in terminal, GitHub, or file explorers.
- **Self‑documenting** – Anyone familiar with the table instantly knows the driver category.
- **Compatible with ESP‑IDF** – The component manager accepts underscores; uppercase works well without breaking case‑sensitive dependency names (though ESP‑IDF component names are case‑preserved, we recommend keeping the same uppercase prefix in the repository name for consistency).

## Rules

1. **Do not invent new prefixes** without updating this document.
2. **Keep the prefix exactly three uppercase letters** followed by an underscore.
3. **The remainder** (`MAX31855`) can be uppercase or mixed case as appropriate, but underscores are allowed only as part of the remainder (e.g., `SNS_BME280` is fine, `SNS_BME_280` is discouraged unless the IC name contains an underscore).
4. **Utility components** (e.g., I2C bus abstraction, ring buffers) use `UTC_` prefix.

## Examples of Valid Names

- `SNS_BME280`
- `DSP_SSD1306`
- `MEM_SD_CARD`
- `UTC_I2C_BUS`

## Examples of Invalid Names

- `Sensor_MAX31855` (wrong prefix length and case)
- `Sns_BME280` (prefix not uppercase)
- `MAX31855` (missing prefix)
- `DRV_MAX31855` (use the correct category prefix `SNS_` instead of generic `DRV_`)

## Integration with ESP‑IDF Component Manager

When writing `idf_component.yml`, the component name can match the repository name (without the `.git`). For example:

```yaml
dependencies:
  SNS_MAX31855:
    version: "1.2.0"
    git: https://github.com/your-org/SNS_MAX31855.git