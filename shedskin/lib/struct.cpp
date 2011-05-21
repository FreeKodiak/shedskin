#include "struct.hpp"
#include <stdio.h>

namespace __struct__ {

__GC_STRING ordering;

char buffy[32];

int get_itemsize(char order, char c) {
    switch(c) {
        case 'c': return 1;
        case 's': return 1;
        case 'p': return 1;
        case '?': return 1;
        case 'x': return 1;
    }
    if(order == '@') {
        switch(c) {
            case 'b': return sizeof(signed char);
            case 'B': return sizeof(unsigned char);
            case 'h': return sizeof(short);
            case 'H': return sizeof(unsigned short);
            case 'i': return sizeof(int);
            case 'I': return sizeof(unsigned int);
            case 'l': return sizeof(long);
            case 'L': return sizeof(unsigned long);
            case 'q': return sizeof(long long);
            case 'Q': return sizeof(unsigned long long);
            case 'f': return sizeof(float);
            case 'd': return sizeof(double);
        }
    } else {
        switch(c) {
            case 'b': return 1;
            case 'B': return 1;
            case 'h': return 2;
            case 'H': return 2;
            case 'i': return 4;
            case 'I': return 4;
            case 'l': return 4;
            case 'L': return 4;
            case 'q': return 8;
            case 'Q': return 8;
            case 'f': return 4;
            case 'd': return 8;
        }
    }
}

__ss_int unpack_one(str *s, __ss_int idx, __ss_int count, __ss_int endian) {
    unsigned int r = 0;

    for(int i=0; i<count; i++) {
        unsigned char c = s->__getitem__(i+idx)->unit[0];
        if (endian)
            r += (c << 8*(count-i-1));
        else
            r += (c << 8*i);

    }

    return r;
}

__ss_int unpack_int(char o, char c, int d, str *data, __ss_int *pos) {
    __ss_int result;
    int itemsize = get_itemsize(o, c);
    int itemsize2;
    itemsize2 = itemsize==8?4:itemsize;
    if(o == '@' and *pos%itemsize2)
        *pos += itemsize2-(*pos%itemsize2);
    result = unpack_one(data, *pos, itemsize, o=='>' or o=='!');
    *pos += itemsize;
    return result;
}

str * unpack_str(char o, char c, int d, str *data, __ss_int *pos) {
    str *result;
    int len;
    switch(c) {
        case 'c':
             result = __char_cache[(unsigned char)(data->unit[*pos])];
             break;
        case 's':
             result = new str();
             for(unsigned int i=0; i<d; i++)
                 result->unit += data->unit[*pos+i];
             break;
        case 'p':
             result = new str();
             len = data->unit[*pos];
             for(unsigned int i=0; i<len; i++)
                 result->unit += data->unit[*pos+i+1];
             break;
    }
    *pos += d;
    return result;
}

__ss_bool unpack_bool(char o, char c, int d, str *data, __ss_int *pos) {
    __ss_bool result;
    if(data->unit[*pos] == '\x01')
        result = True;
    else
        result = False;
    *pos += 1;
    return result;
}

double unpack_float(char o, char c, int d, str *data, __ss_int *pos) {
    return 3.141;
}

void unpack_pad(char o, char c, int d, str *data, __ss_int *pos) {
    *pos += d;
}

__ss_int calcsize(str *fmt) {
    __ss_int result = 0;
    str *digits = new str();
    char order = '@';
    int itemsize, itemsize2;
    for(unsigned int i=0; i<len(fmt); i++) {
        char c = fmt->unit[i];
        if(ordering.find(c) != -1) {
            order = c;
            continue;
        }
        if(::isdigit(c)) {
            digits->unit += c;
            continue;
        }
        int ndigits = 1;
        if(len(digits)) {
            ndigits = __int(digits);
            digits = new str();
        }
        itemsize = get_itemsize(order, c);
        result += ndigits * itemsize;
        switch(c) {
            case 'b': 
            case 'B': 
            case 'h': 
            case 'H': 
            case 'i':
            case 'I': 
            case 'l':
            case 'L':
            case 'q': 
            case 'Q':
                itemsize2 = itemsize==8?4:itemsize;
                if(order == '@' and result%itemsize2)
                    result += itemsize2-(result%itemsize2);
        }
    }
    return result;
}

void fillbuf(char c, __ss_int t, char order, int itemsize) {
    if(order == '@') {
        switch(c) {
            case 'b': *((signed char *)buffy) = t; break;
            case 'B': *((unsigned char *)buffy) = t; break;
            case 'h': *((short *)buffy) = t; break;
            case 'H': *((unsigned short *)buffy) = t; break;
            case 'i': *((int *)buffy) = t; break;
            case 'I': *((unsigned int *)buffy) = t; break;
            case 'l': *((long *)buffy) = t; break;
            case 'L': *((unsigned long *)buffy) = t; break;
            case 'q': *((long long *)buffy) = t; break;
            case 'Q': *((unsigned long long *)buffy) = t; break;
        }
    } else {
        if(order == '>' or order == '!') {
            for(int i=itemsize-1; i>=0; i--) {
                buffy[i] = (unsigned char)(t & 0xff);
                t >>= 8;
            }
        } else {
            for(unsigned int i=0; i<itemsize; i++) {
                buffy[i] = (unsigned char)(t & 0xff);
                t >>= 8;
            }
        }
    }
}

void fillbuf2(char c, double t, char order, int itemsize) {
    switch(c) {
        case 'f': *((float *)buffy) = t; break;
        case 'd': *((double *)buffy) = t; break;
    }
}

str *pack(int n, str *fmt, ...) {
    pyobj *arg;
    va_list args;
    va_start(args, fmt);
    str *result = new str();
    char order = '@';
    str *digits = new str();
    int pos=0;
    int itemsize, pad, itemsize2;
    int fmtlen = fmt->__len__();
    str *strarg;
    int pascal_ff = 0;
    for(unsigned int j=0; j<fmtlen; j++) {
        char c = fmt->unit[j];
        if(ordering.find(c) != -1) {
            order = c;
            continue;
        }
        if(::isdigit(c)) {
            digits->unit += c;
            continue;
        }
        int ndigits = 1;
        if(len(digits)) {
            ndigits = __int(digits);
            digits = new str();
        }
        switch(c) {
            case 'b': 
            case 'B': 
            case 'h': 
            case 'H': 
            case 'i':
            case 'I': 
            case 'l':
            case 'L':
            case 'q': 
            case 'Q':
                itemsize = get_itemsize(order, c);
                itemsize2 = itemsize==8?4:itemsize;
                if(order == '@' and pos%itemsize2) {
                    pad = itemsize2-(pos%itemsize2);
                    for(unsigned int j=0; j<pad; j++) {
                        if(pascal_ff) {
                            result->unit += '\xff';
                            pascal_ff = 0;
                        } else
                            result->unit += '\x00';
                    }
                    pos += pad;
                }
                for(unsigned int j=0; j<ndigits; j++) {
                    arg = va_arg(args, pyobj *);
                    if(arg->__class__ == cl_int_) {
                        fillbuf(c, ((int_ *)arg)->unit, order, itemsize);
                        for(unsigned int k=0; k<itemsize; k++)
                            result->unit += buffy[k];
                        pos += itemsize;
                    }
                }
                if(ndigits)
                    pascal_ff = 0;
                break;
            case 'd':
            case 'f':
                itemsize = get_itemsize(order, c);
                if(order == '@' and pos%4) {
                    pad = 4-(pos%4);
                    for(unsigned int j=0; j<pad; j++) {
                        if(pascal_ff) {
                            result->unit += '\xff';
                            pascal_ff = 0;
                        } else
                            result->unit += '\x00';
                    }
                    pos += pad;
                }
                for(unsigned int j=0; j<ndigits; j++) {
                    arg = va_arg(args, pyobj *);
                    if(arg->__class__ == cl_float_) {
                        fillbuf2(c, ((float_ *)arg)->unit, order, itemsize);
                        if(order == '>' or order == '!')
                            for(int i=itemsize-1; i>=0; i--) 
                                result->unit += buffy[i];
                        else 
                            for(int i=0; i<itemsize; i++) 
                                result->unit += buffy[i];
                        pos += itemsize;
                    }
                }
                if(ndigits)
                    pascal_ff = 0;
                break;
            case 'c': 
                for(unsigned int j=0; j<ndigits; j++) {
                    arg = va_arg(args, pyobj *);
                    strarg = ((str *)(arg));
                    int len = strarg->__len__();
                    if(len != 1)
                        throw new ValueError(new str("char format require string of length 1"));
                    if(arg->__class__ == cl_str_) {
                        result->unit += ((str *)(arg))->unit[0];
                        pos += 1;
                    }
                }
                if(ndigits)
                    pascal_ff = 0;
                break;
            case 'p': 
                arg = va_arg(args, pyobj *);
                if(ndigits) {
                    if(arg->__class__ == cl_str_) {
                        strarg = ((str *)(arg));
                        int len = strarg->__len__();
                        if(len+1 > ndigits)
                            len = ndigits-1;
                        result->unit += (unsigned char)(len);
                        for(unsigned int j=0; j<len; j++)
                            result->unit += strarg->unit[j];
                        for(unsigned int j=0; j<ndigits-len-1; j++)
                            result->unit += '\x00';
                        pos += ndigits;
                    }
                    pascal_ff = 0;
                }
                else
                    pascal_ff = 1;
                break;
            case 's':
                arg = va_arg(args, pyobj *);
                if(ndigits) {
                    if(arg->__class__ == cl_str_) {
                        strarg = ((str *)(arg));
                        int len = strarg->__len__();
                        if(len > ndigits)
                            len = ndigits;
                        for(unsigned int j=0; j<len; j++)
                            result->unit += strarg->unit[j];
                        for(unsigned int j=0; j<ndigits-len; j++) {
                            if(!len and pascal_ff) {
                                result->unit += '\xff';
                                pascal_ff = 0;
                            } else 
                                result->unit += '\x00';
                        }
                        pos += ndigits;
                    }
                    pascal_ff = 0;
                }
                break;
            case '?':
                for(unsigned int j=0; j<ndigits; j++) {
                    arg = va_arg(args, pyobj *);
                    if(arg->__nonzero__())
                        result->unit += '\x01';
                    else
                        result->unit += '\x00';
                    pos += 1;
                }
                if(ndigits)
                    pascal_ff = 0;
                break;
            case 'x':
                for(unsigned int j=0; j<ndigits; j++) {
                    if(pascal_ff) {
                        result->unit += '\xff';
                        pascal_ff = 0;
                    }
                    else
                        result->unit += '\x00';
                }
                pos += ndigits;
                break;
        }
    }
    va_end(args);
    return result;
}

void __init() {
    ordering = "@<>!=";
}

} // module namespace

