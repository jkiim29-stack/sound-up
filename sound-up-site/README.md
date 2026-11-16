# Sound Up — Five-Level Sound Meter

This is a standalone GitHub Pages replacement for the camera-oriented starter page. It provides:

- Device microphone mode using the browser's MediaDevices and Web Audio APIs.
- A microphone picker for built-in and external inputs exposed by the operating system.
- Five sound activity levels: Quiet, Low Average, Average, High Average, and Loud.
- Arduino Nano mode using Web Serial at 115200 baud.
- Backward-compatible parsing for newline-terminated named numeric pairs such as `sound 42` and `sound 42 rms 4.7`.
- Local-only microphone processing; audio is not recorded or uploaded by this page.

## Publish on GitHub Pages

1. Open the repository that publishes `https://jkiim29-stack.github.io/sound-up/`.
2. Back up or rename the current `index.html`.
3. Replace the repository's root `index.html` with this file.
4. Commit the change to the branch/folder configured in **Settings → Pages**.
5. Open the GitHub Pages URL over HTTPS and refresh.

This package is a front-end-only website; no server or API key is required.

## Microphone mode

- Open the site over HTTPS.
- Choose **Device microphone**.
- Click **Start microphone** and grant browser permission.
- After permission, use the input selector to choose the built-in mic, USB mic, headset mic, or another audio input listed by the operating system.
- USB-A, USB-C, Lightning adapters, and Bluetooth are connection methods, not separate browser APIs. The microphone must be paired/connected and exposed by the OS as an audio input.

The browser's input processing and the microphone's own gain controls can affect readings. The meter uses dBFS (digital level), not calibrated dB SPL.

## Arduino Nano mode

- Use a desktop Chromium browser such as Chrome or Edge that supports Web Serial.
- Connect the Nano by USB.
- Close Arduino IDE Serial Monitor / Serial Plotter so they do not occupy the serial port.
- Choose **Arduino Nano → Connect Arduino**, select the Nano's serial port, and connect.
- The page opens the port at 115200 baud.
- It reads newline-separated text containing a `sound` value, for example:

  ```text
  sound 42
  sound 57
  sound 31
  ```

  It also accepts an additional pair, for example `sound 42 rms 4.7`.
- Set **Arduino `sound` full-scale value** to match the scale produced by the firmware. The default is 100. If the firmware's signal range is 0–200, set it to 200.

The browser categorizes the incoming numeric signal into five proportional bands. It does not infer calibrated dB SPL from an arbitrary raw serial value.

## Browser limitations

- `getUserMedia()` requires a secure context (HTTPS or localhost) and explicit permission.
- Microphone device labels and non-default inputs may only appear after permission is granted.
- Web Serial is not supported in every browser. Current desktop Chrome and Edge are the intended Arduino connection path. Mobile Safari and Firefox may not provide Web Serial; microphone mode can still work there.
- On mobile devices, external microphone support depends on the OS, adapter, browser, and whether the input is exposed to the browser.

## Important measurement note

Device microphone mode presents relative dBFS values. Arduino mode presents the serial signal number. Neither is automatically sound pressure level in dB SPL. Actual SPL measurement requires a suitable microphone, calibration against a reference, and appropriate acoustic measurement processing.
