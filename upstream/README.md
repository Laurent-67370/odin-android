# Pull requests to Odin

The Android port goes into Odin (Tovarichtch/Odin) in small pull requests,
from branches of the fork Laurent-67370/Odin. Each must leave the programs
of the other systems as they are.

`branches.txt` lists the branches the workflow `upstream-check.yml` builds
for Android (arm64, the NDK's CMake toolchain), to check that each one
builds on its own:

| Branch | Pull request |
|---|---|
| `android-build` | 1. Building for Android: CMake, folders, folder picker, fonts, the whole screen |
