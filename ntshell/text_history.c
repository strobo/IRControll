/**
 * @file text_history.c
 * @author Shinichiro Nakamura
 * @brief
 * NT-Shell用テキストヒストリモジュールの実装。
 */

/*
 * ===============================================================
 *  Natural Tiny Shell (NT-Shell)
 *  Version 0.0.4
 * ===============================================================
 * Copyright (c) 2010-2011 Shinichiro Nakamura
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * ===============================================================
 */

#include "text_history.h"

void text_history_init(text_history_t *p)
{
    p->rp = 0;
    p->wp = 0;
    int i;
    for (i = 0; i < sizeof(p->history); i++) {
        p->history[i] = 0;
    }
}

int text_history_write(text_history_t *p, unsigned char *buf)
{
    if (buf[0] == '\0') {
        return 0;
    }
    unsigned char *sp = p->history + (TEXTHISTORY_MAXLEN * p->wp);
    while (*buf) {
        *sp = *buf;
        sp++;
        buf++;
    }
    *sp = '\0';
    p->wp = (p->wp + 1) % TEXTHISTORY_DEPTH;
    p->rp = p->wp;
    return 1;
}

int text_history_read(text_history_t *p, unsigned char *buf, int siz)
{
    unsigned char *sp = p->history + (TEXTHISTORY_MAXLEN * p->rp);
    int n = 0;
    while (*sp) {
        *buf = *sp;
        buf++;
        sp++;
        n++;
        if (siz - 1 <= n) {
            break;
        }
    }
    *buf = '\0';
    return n;
}

int text_history_read_point_next(text_history_t *p)
{
    int n = (p->rp + 1) % TEXTHISTORY_DEPTH;
    if (n != p->wp) {
        p->rp = n;
        return 1;
    }
    return 0;
}

int text_history_read_point_prev(text_history_t *p)
{
    int n = (p->rp == 0) ? (TEXTHISTORY_DEPTH - 1) : (p->rp - 1);
    if (n != p->wp) {
        unsigned char *sp = p->history + (TEXTHISTORY_MAXLEN * n);
        if (*sp != '\0') {
            p->rp = n;
            return 1;
        }
    }
    return 0;
}

