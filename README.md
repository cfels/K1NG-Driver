

> [!IMPORTANT]
> U may need something like this for the driver to work:

```
services.udev.extraRules = ''
    SUBSYSTEM=="hidraw", ATTRS{idVendor}=="3554", ATTRS{idProduct}=="f54d", MODE="0666"
  '';
```

## K1NG-Driver

a practically finished driver for K1NG PRO (4K) mouse from redragon on linux

### Downloading

u can download the driver from [Release's](https://git.vacpro.fyi/moxiu/K1NG-Driver/releases/)

### Compiling

```
nix-shell --run "make clean && make"
sudo ./k1ng_driver
```

### TODO

- [x] add config support
- [x] changing sens
- [x] wireless mode support
- [x] wired mode support

### License

this project is under [MIT License](https://git.vacpro.fyi/moxiu/K1NG-Driver/src/branch/dev/LICENSE)
