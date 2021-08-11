# emulator-networkaccess
is a stream protocol to communicate with emulators.

Its goal is to integrate emulators into the already existing ecosystem used
for randomizers, trackers and other tools as well as expose a uniform interface
to other emulator-specific functionality.

## About the repository

The repository contains the specification documents for the protocol but also an implementation exemple

### Generic poll server

This is base written in C implemting the protocol, using `poll` as a base for async socket, 
to use it just follow what is written in the sources files.

### Dummy Emulator

This is fake snes emulator using the Generic Poll Server code and taking a SNES rom + snes9x savestate to
simulate an emulator.

## Snes and Usb2snes

If you already wrote application using the usb2snes websocket protocol, QUsb2Snes offer a 
support for this protocol. If your application is a desktop one it can be benifical to use
this protocol when avalaible since it avoid the need for QUsb2Snes to be started.

## Implementations
* bnses-plus fork: [https://github.com/black-sliver/bsnes-plus]
* Qt client library: [https://github.com/black-sliver/EmuNWAccess-qt]
* Qt client test tool: [https://github.com/black-sliver/EmuNWAccessTest]
* Snes9x fork : [https://github.com/Skarsnik/snes9x]
