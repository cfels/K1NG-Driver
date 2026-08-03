## K1NG-Driver

a practically finished driver for K1NG PRO (4K) mouse from redragon, ported to linux

### Installing

download the driver from [Release's](https://git.vacpro.fyi/moxiu/K1NG-Driver/releases/)

```
cd output && ./install.sh
```

everything "should" install properly

### Compiling

```
./clean.sh && ./build.sh
```

output dir is `output` and install dir is `~/.local/bin`

### TODO

- [x] add config support
- [x] changing sens
- [x] wireless mode support
- [x] wired mode support
- [x] make an ui for the driver

### License

this project is under [MIT License](https://git.vacpro.fyi/moxiu/K1NG-Driver/src/branch/dev/LICENSE)
