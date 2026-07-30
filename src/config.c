#include "../includes/include.h"
#include "config.h"

int config_save_preset(const char *path, const char *name, int dpi) {
    FILE *fp = fopen(path, "a");
    if (!fp) return -1;
    fprintf(fp, "[%s]\ndpi = %d\n\n", name, dpi);
    fclose(fp);
    return 0;
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
    if (val.type != TOML_INT64) {
        fprintf(stderr, "preset '%s' not found\n", name);
        toml_free(result);
        return -1;
    }
    *dpi = (int)val.u.int64;
    toml_free(result);
    return 0;
}
