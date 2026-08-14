# Quartz LCD

**Quartz LCD** is a native Pebble watchface that recreates the visual language of a vintage digital quartz display. Its clock is composed from pre-rendered, seven-segment-style bitmap digits rather than a conventional text clock. Alongside the time, the face presents the date, live local temperature, locality name, battery state, Bluetooth status, and a compact French weekday indicator.

The source package identifies the project as version **1.5.0**. It targets Pebble SDK 3 and combines a C application running on the watch with a PebbleKit JS companion component running through the paired phone application. This split is the intended Pebble architecture for native watchfaces with phone-assisted functionality.[1]

> **Design principle:** the displayed digits are image assets. The application selects and draws the correct large or small bitmap for each numeral, allowing a consistent LCD/segment appearance across the interface.

## Features

Quartz LCD provides a complete digital dashboard while keeping the watch-side rendering deterministic and lightweight.

| Area | Behaviour |
|---|---|
| **Clock** | Displays a zero-padded 24-hour time in `HH:MM` using large segment-style digits. The colon can blink each second or remain continuously visible. |
| **Seconds** | Optionally displays `SS` using smaller segment-style digits in the lower-right area of the time display. |
| **Date and weekdays** | Shows the date as `DD-MM` and a Monday-first weekday strip labelled `L M M J V S D`. A bitmap cursor marks the current day. |
| **LCD appearance** | Supports six active-segment palettes: gray, green, blue, orange, yellow, and white. It can also draw the inactive segments behind each digit for a fuller LCD effect. |
| **Colour customization** | Lets the user set independent background, date/accent, and time/weather text colours. |
| **Weather** | Retrieves the current temperature from the phone’s location, resolves a locality name, and displays the result in either Celsius or Fahrenheit. |
| **Connectivity state** | Shows a Bluetooth-connected or disconnected bitmap and a battery gauge rounded to the nearest ten-percent step. |
| **Persistence** | Stores display settings on the watch so customisation survives application restarts. |

## What is shown on the watch

The layout is rendered by a single custom canvas layer. Time, date, weather, battery, and connectivity are redrawn whenever the clock ticks, a system status changes, or a configuration/weather message arrives.

| Screen area | Content | Implementation detail |
|---|---|---|
| Top left | Date | `DD-MM`, rendered as system text. |
| Top right | Phone connection | `bt_on` or `bt_off` bitmap. |
| Centre | Main time | Four large bitmap digits and a colon. |
| Lower right of time | Seconds | Two smaller bitmap digits when enabled. |
| Middle lower band | Locality and temperature | Uppercased locality, current temperature, degree symbol, and `C` or `F`. |
| Bottom band | Battery | `BAT` label and one of eleven charge-level bitmap indicators. |
| Bottom row | Weekdays | French initials, with a cursor under today’s initial. |

## Configuration

The settings page is defined in `src/pkjs/config.js` and is generated with **Clay**. Clay allows a Pebble configuration UI to be declared in JavaScript/JSON and delivers the selected values to the native application through AppMessage.[2]

| Setting | Choices or value | Effect on the face |
|---|---|---|
| **Background colour** | Colour picker | Changes the canvas fill colour. |
| **Date colour** | Colour picker | Changes the date, weekday labels, divider, and battery label. |
| **Time/weather text colour** | Colour picker | Changes the temperature and related text. |
| **Temperature unit** | Celsius or Fahrenheit | Keeps incoming temperature in Celsius and converts it locally for Fahrenheit display. |
| **Active segment colour** | Gray, green, blue, orange, yellow, or white | Reloads the corresponding large and small digit bitmap sets. |
| **Show inactive segments** | On/off | Draws the unlit-segment bitmap before each digit when enabled. |
| **Show seconds** | On/off | Shows or hides the two small seconds digits. |
| **Blink colon** | On/off | Toggles the colon once per second when enabled; otherwise the colon stays visible. |

The watch stores the colours, toggles, selected segment palette, and temperature unit under a persistent settings key. Weather data is kept in memory only and is refreshed by the companion component.

## Weather and location flow

The watch itself does not make HTTP requests. Instead, `src/pkjs/app.js` requests the paired phone’s location, uses it to determine a city or locality, and then fetches the current air temperature. PebbleKit JS is specifically designed to extend watch applications with a phone-managed JavaScript component and bidirectional watch/phone messaging.[1]

```text
Phone location permission
        |
        v
Browser geolocation (latitude, longitude)
        |
        +--> BigDataCloud reverse geocoding --> locality name
        |
        +--> Open-Meteo forecast endpoint --> current temperature_2m
                                              |
                                              v
                               Pebble AppMessage: LOCATION + TEMPERATURE
                                              |
                                              v
                                    Native C canvas redraw
```

The JavaScript companion runs a weather update when it becomes ready and schedules subsequent attempts every **30 minutes**. It asks for non-high-accuracy positioning, accepts a cached location up to ten minutes old, and uses a 15-second location timeout. The locality is uppercased and trimmed to 18 characters before being sent to the watch.

The reverse-geocoding endpoint used by this project accepts latitude/longitude and returns structured locality data, including city and locality fields.[3] The weather request asks Open-Meteo for `current=temperature_2m`; Open-Meteo defines this current variable as air temperature at two metres above ground.[4]

### Error behaviour

If location permission is unavailable, the face uses the fallback label `LOCALISATION`. If the weather request fails before a valid temperature has reached the watch, the temperature area shows `--`. If a later update fails, the last valid temperature remains displayed because the C application does not clear it.

## Privacy and network notes

Location-based weather requires the user to grant location permission to the companion environment and requires network access on the paired phone. According to the source code, latitude and longitude are sent over HTTPS to BigDataCloud for reverse geocoding and to Open-Meteo for weather retrieval. The locality string and rounded temperature are then sent to the watch; the watch-side code does not persist coordinates, locality, or weather values.

This README describes the observed technical data flow, not a legal privacy policy. Before distributing a build, maintainers should review the current privacy terms and service conditions of the two external providers.[3] [4]

## Project structure

```text
.
├── package.json                 # Pebble app metadata, capabilities, message keys, assets, targets
├── wscript                      # Pebble SDK build and JavaScript bundle definition
├── jshintrc                     # JavaScript linting configuration
├── resources/
│   └── images/                  # 138 bitmap assets: digit sets, status icons, cursor, and glyphs
└── src/
    ├── c/
    │   └── main.c               # Native watchface: drawing, settings, sensors, AppMessage receiver
    └── pkjs/
        ├── app.js               # Clay bootstrap, phone geolocation, API calls, weather messaging
        └── config.js            # Declarative settings-page schema
```

### Native watch application: `src/c/main.c`

The C component owns the window, custom canvas layer, bitmap lifecycles, settings persistence, and device services. Its `canvas_update()` function clears the background and draws every visible component in a fixed coordinate layout. The code subscribes to the tick timer, battery service, connection service, and inbound AppMessage events.

Digit rendering is data-driven. The `large_ids` and `small_ids` arrays map each colour palette and numeral to its resource ID. When the palette changes, the program destroys the current digit bitmaps and loads the new set. The digit layers have two sizes: large digits for hours and minutes, and small digits for seconds.

### Phone companion: `src/pkjs/app.js`

The JavaScript component initializes Clay, retrieves location through `navigator.geolocation`, performs two GET requests, and sends weather results back to the watch. It handles an incoming `REQUEST_WEATHER` message, although the current C code does not send this key; scheduled updates are therefore the active refresh mechanism in this source snapshot.

### Configuration schema: `src/pkjs/config.js`

The configuration schema groups the controls into **Colours** and **LCD display** sections. It uses Clay colour pickers, radio groups, and toggles, then sends matching message keys that are declared in `package.json` and decoded in the C inbox handler.

### Build manifest: `package.json` and `wscript`

`package.json` declares the native watchface, the `location` and `configurable` capabilities, all AppMessage keys, SDK version 3, bitmap resources, and supported platform identifiers. Pebble projects use `package.json` for this metadata, including target platforms, message keys, and bundled media.[5]

`wscript` compiles all C files under `src/c/` for each declared target, copies the manifest to the JavaScript bundle area, and bundles the files under `src/pkjs/` with `app.js` as the entry point. A worker binary is supported by the build script only when a `worker_src/` directory exists; none is present in this project.

## AppMessage contract

The following keys constitute the communication boundary between the configuration/phone component and the native face.

| Message key | Direction | Payload | Purpose |
|---|---|---|---|
| `BackgroundColor` | Phone → watch | Integer colour | Stores the canvas background colour. |
| `DateColor` | Phone → watch | Integer colour | Stores the date/accent colour. |
| `TimeColor` | Phone → watch | Integer colour | Stores the time/weather text colour. |
| `ShowSeconds` | Phone → watch | Integer boolean | Enables or hides the small seconds digits. |
| `BlinkColon` | Phone → watch | Integer boolean | Enables the per-second colon toggle. |
| `ShowInactiveSegments` | Phone → watch | Integer boolean | Draws or hides unlit digit segments. |
| `SegmentColor` | Phone → watch | String | Selects the digit asset palette. |
| `TemperatureUnit` | Phone → watch | `"C"` or `"F"` | Selects the display unit. |
| `LOCATION` | Phone → watch | String | Supplies the locality label. |
| `TEMPERATURE` | Phone → watch | Integer | Supplies the rounded Celsius temperature. |
| `REQUEST_WEATHER` | Watch → phone | Presence flag | Supported by `app.js` but not emitted by the current C source. |

## Requirements and supported targets

To build the project, use a Pebble SDK 3-compatible development environment and a working Node/Pebble package setup. The project relies on `@rebble/clay` version `^1.0.0` for its settings UI. The Pebble C API is used for the native watchface and PebbleKit JS for the phone-assisted behaviour.[1] [2]

| Requirement | Why it is needed |
|---|---|
| **Pebble SDK 3-compatible toolchain** | Compiles the native C source and packages the watchface. |
| **Node/Pebble package workflow** | Resolves `@rebble/clay` before JavaScript bundling. |
| **A supported Pebble platform** | The manifest declares `aplite`, `basalt`, `diorite`, `emery`, and `flint`. |
| **Paired phone with location permission and data access** | Required only for the automatic locality and temperature feature. |
| **Rebble/Pebble companion environment** | Runs the PebbleKit JS companion and opens the Clay configuration page. |

The project does **not** declare `chalk`; round Pebble targets are therefore not part of this build configuration. The screen positions are hard-coded around a 144-pixel-wide, 168-pixel-high rectangular design. This works naturally on the 144×168 family, but the current source does not scale or recenter its layout for larger rectangular displays such as `emery`; maintainers may want to add geometry-aware layout before distributing an Emery-focused build.

## Build and install

1. Clone or extract the repository, then enter its root directory.
2. Install the declared JavaScript dependency using the package workflow provided by your Pebble SDK environment. Clay’s documentation lists `pebble package install @rebble/clay` as the relevant package-install command for local SDK projects.[2]
3. Build the native watchface for the targets declared in `package.json`.
4. Install the generated `.pbw` package on a compatible paired watch using your Pebble/Rebble development workflow.

A typical local sequence is:

```bash
git clone <repository-url> quartz-lcd
cd quartz-lcd
pebble package install
pebble build
```

If your toolchain does not automatically resolve dependencies from `package.json`, install the declared package explicitly before running the build:

```bash
pebble package install @rebble/clay
pebble build
```

> The legacy Pebble toolchain ecosystem can vary by operating system and companion setup. Use a maintained SDK 3-compatible workflow, and consult the Rebble developer documentation when adapting the build process to a modern system.[1] [5]

## Customisation workflow

After installing the watchface, open its configuration page from the Pebble/Rebble companion application. Select the desired colours, segment palette, temperature unit, and LCD toggles, then save. The companion sends the values to the watch, which persists them and redraws the face. Changing the segment palette reloads both large and small digit assets immediately.

## Development notes and known limitations

| Topic | Current behaviour | Suggested maintenance action |
|---|---|---|
| **Version naming** | The package version is `1.5.0`, while the visible `displayName` still says `Quartz LCD 1.4`; the Clay page says `Quartz LCD 1.5`. | Align the visible name and configuration heading before release. |
| **Inactive-segment default** | C runtime defaults enable inactive segments, while the Clay form declares its toggle default as disabled. | Choose a single first-run default and use it in both places. |
| **Weather refresh** | Updates occur at startup and every 30 minutes; a request message is handled in JS but is not sent by C. | Add a watch-side trigger if manual or event-driven refresh is desired. |
| **Fixed layout** | Drawing coordinates are static rather than computed from screen bounds. | Add platform-aware or bounds-aware layout for larger displays. |
| **Language** | The weekday initials, fallback locality, and configuration labels are French. | Localize strings if the project is intended for a broader audience. |
| **Weather resilience** | The last valid temperature remains visible after a later failed fetch. | Optionally show freshness or clear stale data after a defined timeout. |

## Contributing

Contributions are welcome. For a focused change, update the appropriate layer: native drawing and watch services belong in `src/c/main.c`; phone/network behaviour belongs in `src/pkjs/app.js`; and settings controls belong in `src/pkjs/config.js`. When introducing a new configuration option, keep the Clay `messageKey`, `package.json` message key list, and the native AppMessage inbox handler synchronized.

Please test visual changes on every target platform you intend to support, especially changes to image sizes or fixed coordinates. Avoid adding credentials: the current weather integration uses public HTTPS endpoints and contains no API key.

## License

No license file was included in this source archive. Until the repository owner adds an explicit license, reuse and redistribution rights are not granted by this README. Add a `LICENSE` file before publishing or accepting external contributions.

## References

[1]: https://developer.rebble.io/docs/ "Rebble Developer Documentation — Pebble C API and PebbleKit JS"
[2]: https://developer.rebble.io/blog/2016/06/24/introducing-clay/ "Rebble — Introducing Clay"
[3]: https://www.bigdatacloud.com/free-api/free-reverse-geocode-to-city-api "BigDataCloud — Free Client Side Reverse Geocoding to City API"
[4]: https://open-meteo.com/en/docs "Open-Meteo — Weather Forecast API documentation"
[5]: https://developer.rebble.io/guides/tools-and-resources/app-metadata/ "Rebble Developer Documentation — App Metadata"
