#ifndef CONFIG_H
#define CONFIG_H

int config_save_preset(const char *path, const char *name, int dpi);
int config_load_preset(const char *path, const char *name, int *dpi);

#endif
