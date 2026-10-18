## K1NG-Driver

a practically finished driver for K1NG PRO (4K) mouse from redragon, ported to linux

> [!WARNING]
> soo something broke and setting ur sens no longer works for now

### Installing

download the driver from [Release's](https://github.com/cfels/K1NG-Driver/releases/)

everything works maybe

### Compiling

```
git clone https://github.com/cfels/K1NG-Driver.git
cd K1NG-Driver
./clean.sh && ./build.sh
```

output dir is `output` and install dir is `~/.local/bin`

### TODO

- [x] add config support
- [x] changing sens
- [x] wireless mode support
- [x] wired mode support
- [ ] find battery precentage
- [ ] delete bits of old ui code
- [ ] make the driver work (again)

### License

this project is under [MIT License](https://github.com/cfels/K1NG-Driver/blob/dev/LICENSE)
