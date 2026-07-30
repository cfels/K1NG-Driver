> [!NOTE]  
> work in progress, but works!

> [!IMPORTANT]
> U may need something like this for the driver to work:

```
services.udev.extraRules = ''
    SUBSYSTEM=="hidraw", ATTRS{idVendor}=="3554", ATTRS{idProduct}=="f54d", MODE="0666"
  '';
```

## K1NG-Driver

a driver for K1NG PRO (4K) mouse from redragon

### Downloading

u can download the driver from [Release's](https://git.vacpro.fyi/moxiu/K1NG-Driver/releases/)

### Compiling

```
nix-shell --run "make clean && make"
sudo ./k1ng_driver
```

### TODO

- [ ] add config support
- [x] changing sens
- [x] wireless mode support
- [x] wired mode support
- [ ] make a gui for the driver

### License

this project is under [MIT License](https://github.com/cfels/K1NG-Driver/blob/dev/LICENSE)
