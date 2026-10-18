#include "../includes/include.h"
#include "config.h"

int config_save_preset(const char *path, const char *name, int dpi) {
    FILE *fp = fopen(path, "a");
    if (!fp) return -1;
    int written = fprintf(fp, "[%s]\ndpi = %d\n\n", name, dpi);
    int closed = fclose(fp);
    return written < 0 || closed != 0 ? -1 : 0;
}

int config_load_preset(const char *path, const char *name, int *dpi) {
    toml_result_t result = toml_parse_file_ex(path);
    if (!result.ok) {
        fprintf(stderr, "config error: %s\n", result.errmsg);
        return -1;
    }
    char key[256];
    snprintf(key, sizeof(key), "%s.dpi", name);
    toml_datum_t val = toml_seek(result.toptab, key);
    if (val.type != TOML_INT64 || val.u.int64 < 50 || val.u.int64 > 26000 || val.u.int64 % 50 != 0) {
        fprintf(stderr, "preset '%s' has no valid DPI (50-26000 in steps of 50)\n", name);
        toml_free(result);
        return -1;
    }
    *dpi = (int)val.u.int64;
    toml_free(result);
    return 0;
}
