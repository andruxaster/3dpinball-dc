# 3D Pinball - Space Cadet for Dreamcast

Port of 3D Pinball - Space Cadet to the Sega Dreamcast, based on the [decompilation project](https://github.com/k4zmu2a/SpaceCadetPinball)

![image](screenshot.png)

Built with [DreamSDK](https://github.com/dreamsdk/dreamsdk)

Libraries used:
* LibADX: https://dcemulation.org/phpBB/viewtopic.php?t=101638
  * With some modifications to compile on latest KallistiOS
  * Built automatically as part of `make`, no separate install needed

## Controls
* Paddles: DPad Left-Right / X/B / LT/RT
* Launch ball: DPad Down / A
* Nudge table: Y + DPad
* Pause: START
* New game: START + A
* GDemu reset: START + A + B + X + Y

## Known issues
* ~~After playing for a short while, the score/mission info text panel starts displaying words in new lines~~ — fixed: caused by a heap overflow in `Sound::PlaySound` (AICA channel 0..63 used as index into an 8-entry array)
