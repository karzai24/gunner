# Xbox-style controller testing

The movement range uses an Xbox-style Enhanced Input layout for one local player. Keyboard and mouse remain available. HUD hints follow the last meaningful input device; neutral sticks and releases do not change them.

## Connect and play on this Mac

For a supported Bluetooth Xbox controller, turn it on, hold its pairing button, then choose it in macOS **System Settings → Bluetooth**. Supported models and pairing details are listed in [Apple's Xbox controller guide](https://support.apple.com/en-us/111101). This project does not install a controller driver or change macOS Bluetooth settings.

Open `Gunner.uproject`, press **Play** in `L_MotionRange`, and click inside the game viewport once to give it focus. Move a stick or press a controller button: the bottom-left hints should change to Xbox labels. If buttons navigate Unreal's editor instead, click the game view again. Shift+F1 releases the mouse back to the editor; Escape stops Play.

| Control | Action |
|---|---|
| Left stick | Move; partial tilt walks more slowly |
| Right stick | Look; up looks up |
| LT / RT | Hold aim / fire; RT alone blind-fires while attached to low cover |
| A | Tap near cover to attach; tap in open space to roll; hold while moving forward to sprint |
| X | Reload, including supported crouch/cover reload |
| Y | Switch between rifle and pistol |
| B | Melee, alternating the existing jab/cross attacks |
| L3 (left-stick click, held) | Alternate sprint input |
| R3 (right-stick click) | Toggle crouch |
| LB | Explicit dodge roll |
| RB | Swap aiming shoulder |
| D-pad up / down | Select rifle / pistol directly |
| D-pad right | Jump |

A's tap/hold traversal requires the installed local movement profile, which is enabled on the development machine. A fresh clone without that profile retains A for the portable cover/jump behavior; LB still rolls and L3 still sprints. Action guards still apply: for example, the character cannot melee while crouched, and an obstructed roll is rejected.

Both sticks have an 18% radial dead zone. Right-stick response uses a 1.5 exponent for finer small movements, with a base turn rate of 120 degrees/second; aim uses 55% of that rate. These settings live in `IMC_Xbox` and `DA_XboxInput`. Mouse look is unchanged. There is no aim assist, rumble, controller remapping menu or multiplayer device-assignment implementation in this pass.

## Content and validation

`Tools/install_controller_input.py` creates new `IMC_Xbox`, `DA_XboxInput` and `IA_CycleWeapon` assets, copies existing keyboard bindings and assigns only the input config on `BP_WardenMotion`. It refuses existing outputs, preserves the original input assets and backs up the prior character. It introduces no licensed animation dependency. Ordinary use must not rerun this authoring script.

`Tools/validate_controller_assets.py` checks saved bindings/tuning and preserved assets in a fresh process. `-GunnerControllerSmoke` with `Tools/validate_controller_editor.py` exercises the actual gamepad-key Enhanced Input path in two rendered sessions. See [TEST_MATRIX](TEST_MATRIX.md) for actual results. Automated events cannot establish Bluetooth pairing, a particular physical controller's firmware, disconnect/reconnect behavior or how the controls feel in hand.
