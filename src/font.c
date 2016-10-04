#include "font.h"

static int is_korean_char(const unsigned int c) {
    unsigned short ch = c;
    // hangul compatibility jamo block
    if (0x3130 <= ch && ch <= 0x318f) {
        return 1;
    }
    // hangul syllables block
    if (0xac00 <= ch && ch <= 0xd7af) {
        return 1;
    }
    // korean won sign
    if (ch == 0xffe6) {
        return 1;
    }
    return 0;
}

static int is_latin_char(const unsigned int c) {
    unsigned short ch = c;
    // basic latin block + latin-1 supplement block
    if (ch <= 0x00ff) {
        return 1;
    }
    // cyrillic block
    if (0x0400 <= ch && ch <= 0x04ff) {
        return 1;
    }
    return 0;
}

vita2d_pgf* load_system_fonts() {
    vita2d_system_pgf_config configs[] = {
        {SCE_FONT_LANGUAGE_KOREAN,  is_korean_char},
        {SCE_FONT_LANGUAGE_LATIN,   is_latin_char},
        {SCE_FONT_LANGUAGE_DEFAULT, NULL},
    };

    return vita2d_load_system_pgf(3, configs);
}
