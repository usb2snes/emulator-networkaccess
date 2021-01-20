# emulator-networkaccess
is a stream protocol to communicate with emulators.

Its goal is to integrate emulators into the already existing ecosystem used
for randomizers, trackers and other tools as well as expose a uniform interface
to other emulator-specific functionality.

SNES application writers can continue to use the usb2snes websocket protocol,
for which QUsb2Snes will detect your emulator as connected console.

## Implementations
* bnses-plus fork: [https://github.com/black-sliver/bsnes-plus]
* Qt client library: [https://github.com/black-sliver/EmuNWAccess-qt]
* Qt client test tool: [https://github.com/black-sliver/EmuNWAccessTest]
