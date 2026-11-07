# iPhone and iPad

The iOS target builds Ratchet & Clank (NTSC-U) from the same native C/C++ as
the desktop port. It uses SDL's UIKit, audio, gamepad and touch backends and
Vulkan through MoltenVK. Gameplay uses the native engine without JIT.
The launcher reuses the desktop Svelte interface and Rust core in WKWebView,
with UIKit bridges for Files and starting the native engine. Its full catalogue
is shared; only engines listed in the signed app's `engines.json` can run.

## Build

Use a Mac with Xcode, CMake, Ninja and Python 3.10 or newer. Download
MoltenVK's official iOS release, currently 1.4.2, and use the **iOS ARM64**
slice of its dynamic XCFramework. Also obtain Python Apple Support 3.14-b14
for iOS. A macOS library cannot be used on iOS.

First build the shared launcher and its static Rust library:

```sh
cd launcher
yarn install --immutable
yarn build
rustup target add aarch64-apple-ios
cargo build --manifest-path core/Cargo.toml --release \
  --target aarch64-apple-ios --features ios-bridge
cd ..
```

```sh
python3 tools/build_ios.py --build-dir /private/path/build-ios \
  --moltenvk /private/path/MoltenVK.xcframework/ios-arm64/MoltenVK.framework \
  --python-xcframework /private/path/Python.xcframework \
  --launcher-core launcher/target/aarch64-apple-ios/release/libopenrac_launcher_core.a \
  --disc /private/path/SCUS_971.99.iso --team YOUR_TEAM
```

The result is `OpenRAC.app`. Sign all embedded frameworks and then
the app with your Apple identity and a provisioning profile for its bundle
identifier. Enable **Extended Virtual Addressing** on the App ID and include
`port/ios/OpenRAC.entitlements` when signing: game memory reserves a 4 GB
address range, with only the game's regions backed by physical pages. This
is not a request to allocate 4 GB of RAM.

Build output, signing profiles, keys and disc data stay outside Git. The
default deployment target is iOS/iPadOS 17.0. This is a deployment setting,
not a claim of performance on every supported device.

## Game data

Use the shared launcher's **Set up from disc** action, then select your
own NTSC-U image in Files. It can be on the device or in a document provider
the user has connected. OpenRAC copies it into its own Documents directory,
checks the boot executable and prepares all 21 levels locally with its
existing Python exporter. CPython is embedded for this preparation only;
the game itself remains native. No executable or Python script from the imported image is executed.
OpenRAC does not download code for disc preparation.

Keep the app open during preparation. The screen shows its progress. A
failed or interrupted import stays separate from the last working copy, and
replaced copies remain recoverable in Documents. No existing save is replaced.

The import backend also accepts the desktop launcher's prepared
`active/rac1/data`: it must contain the boot file under `iso_data/rac1` and
the exported levels under `port/ntsc`. App code, libraries, notices and the same
small catalogue thumbnails as the desktop launcher are packaged in the public
bundle. Retail discs and extracted gameplay data are rejected by the packager.

For device development, `devicectl device copy to` can transfer the same
folder to `Documents/data` in the app's data container. Wait for the copy to
finish, then launch the app; it migrates this legacy path to
`Documents/OpenRAC/active/rac1/data`. Saves and settings remain in the app's private
Library/Application Support/OpenRAC directory.

## Release checks

A successful cross-compilation proves only that the iOS executable links.
Before a release, verify installation and gameplay on physical iPhone and
iPad, touch and gamepad input, sound, suspend/resume, import errors, saves,
memory, heat and sustained frame pacing. Check the signed app and upload
separately. App Store preparation and Apple approval are separate steps.

The public app must contain no retail disc, extracted assets or private
test data. Preserve the project's licenses and credits. Store descriptions,
privacy declarations, rights documentation and screenshots must describe
the app that was actually tested.
