#include <gost_3411_2012_calc.h>

//#define DEBUG_MODE

#ifdef DEBUG_MODE
static void
GOSTHashPrintDebug(uint8_t *state)
{
    int i;
    for (i = 0; i < 64; i++)
        printf("%02x", state[i]);
    printf("\n");
}
#endif

void
GOSTHashInit(TGOSTHashContext *CTX, uint16_t hash_size)
{
    memset(CTX, 0x00, sizeof(TGOSTHashContext));
    if (hash_size == 256)
        memset(CTX->h, 0x01, BLOCK_SIZE);
    else
        memset(CTX->h, 0x00, BLOCK_SIZE);
    CTX->hash_size = hash_size;
    CTX->v_512[1] = 0x02;
}

static void
GOSTHashX(const uint8_t *a, const uint8_t *b, uint8_t *c)
{
    int i;
    for (i = 0; i < 64; i++)
        c[i] = a[i]^b[i];
}

static void
GOSTHashAdd512(const uint8_t *a, const uint8_t *b, uint8_t *c)
{
    int i;
    int internal = 0;
    for (i = 0; i < 64; i++)
    {
        internal = a[i] + b[i] + (internal >> 8);
        c[i] = internal & 0xff;
    }
}

static void
GOSTHashLPS(uint8_t *state)
{
    uint64_t internal[8];
    internal[0] = T[0][(state[0])] ^ T[1][(state[8])] ^ T[2][(state[16])] ^ T[3][(state[24])] ^
            T[4][(state[32])] ^ T[5][(state[40])] ^ T[6][(state[48])] ^ T[7][(state[56])];
    internal[1] = T[0][(state[1])] ^ T[1][(state[9])] ^ T[2][(state[17])] ^ T[3][(state[25])] ^
            T[4][(state[33])] ^ T[5][(state[41])] ^ T[6][(state[49])] ^ T[7][(state[57])];
    internal[2] = T[0][(state[2])] ^ T[1][(state[10])] ^ T[2][(state[18])] ^ T[3][(state[26])] ^
            T[4][(state[34])] ^ T[5][(state[42])] ^ T[6][(state[50])] ^ T[7][(state[58])];
    internal[3] = T[0][(state[3])] ^ T[1][(state[11])] ^ T[2][(state[19])] ^ T[3][(state[27])] ^
            T[4][(state[35])] ^ T[5][(state[43])] ^ T[6][(state[51])] ^ T[7][(state[59])];
    internal[4] = T[0][(state[4])] ^ T[1][(state[12])] ^ T[2][(state[20])] ^ T[3][(state[28])] ^
            T[4][(state[36])] ^ T[5][(state[44])] ^ T[6][(state[52])] ^ T[7][(state[60])];
    internal[5] = T[0][(state[5])] ^ T[1][(state[13])] ^ T[2][(state[21])] ^ T[3][(state[29])] ^
            T[4][(state[37])] ^ T[5][(state[45])] ^ T[6][(state[53])] ^ T[7][(state[61])];
    internal[6] = T[0][(state[6])] ^ T[1][(state[14])] ^ T[2][(state[22])] ^ T[3][(state[30])] ^
            T[4][(state[38])] ^ T[5][(state[46])] ^ T[6][(state[54])] ^ T[7][(state[62])];
    internal[7] = T[0][(state[7])] ^ T[1][(state[15])] ^ T[2][(state[23])] ^ T[3][(state[31])] ^
            T[4][(state[39])] ^ T[5][(state[47])] ^ T[6][(state[55])] ^ T[7][(state[63])];
    memcpy(state, internal, 64);
}

static void
GOSTHashKeyShedule(uint8_t *K, int i)
{
    GOSTHashX(K, C[i], K);
    GOSTHashLPS(K);
}

static void
GOSTHashE(uint8_t *K, const uint8_t *m, uint8_t *state)
{
    int i;
    memcpy(K, K, BLOCK_SIZE);
    GOSTHashX(m, K, state);
    for(i = 0; i < 12; i++)
    {
        GOSTHashLPS(state);
        GOSTHashKeyShedule(K, i);
        GOSTHashX(state, K, state);
    }
}

static void
GOSTHashG( uint8_t *h, uint8_t *N, const uint8_t *m)
{
    vect K, internal;
    GOSTHashX(N, h, K);

    GOSTHashLPS(K);

    GOSTHashE(K, m, internal);

    GOSTHashX(internal, h, internal);
    GOSTHashX(internal, m, h);
}

static void
GOSTHashPadding(TGOSTHashContext *CTX)
{
    vect internal;

    if (CTX->buf_size < BLOCK_SIZE)
    {
        memset(internal, 0x00, BLOCK_SIZE);
        memcpy(internal, CTX->buffer, CTX->buf_size);
        internal[CTX->buf_size] = 0x01;
        memcpy(CTX->buffer, internal, BLOCK_SIZE);
    }
}

static void
GOSTHashStage_2(TGOSTHashContext *CTX, const uint8_t *data)
{
    GOSTHashG(CTX->h, CTX->N, data);
    GOSTHashAdd512(CTX->N, CTX->v_512, CTX->N);
    GOSTHashAdd512(CTX->Sigma, data, CTX->Sigma);

#ifdef DEBUG_MODE
    printf("Stage 2\n");
    printf("G:\n");
    GOSTHashPrintDebug(CTX->h);
    printf("N:\n");
    GOSTHashPrintDebug(CTX->N);
    printf("Sigma:\n");
    GOSTHashPrintDebug(CTX->Sigma);
#endif
}

static void
GOSTHashStage_3(TGOSTHashContext *CTX)
{
    vect internal;
    memset(internal, 0x00, BLOCK_SIZE);
    internal[1] = ((CTX->buf_size * 8) >> 8) & 0xff;
    internal[0] = (CTX->buf_size * 8) & 0xff;

    GOSTHashPadding(CTX);

    GOSTHashG(CTX->h, CTX->N, (const uint8_t*)&(CTX->buffer));

    GOSTHashAdd512(CTX->N, internal, CTX->N);
    GOSTHashAdd512(CTX->Sigma, CTX->buffer, CTX->Sigma);

    GOSTHashG(CTX->h, CTX->v_0, (const uint8_t*)&(CTX->N));
    GOSTHashG(CTX->h, CTX->v_0, (const uint8_t*)&(CTX->Sigma));

#ifdef DEBUG_MODE
    printf("Stage 3\n");
    printf("G:\n");
    GOSTHashPrintDebug(CTX->h);
    printf("N:\n");
    GOSTHashPrintDebug(CTX->N);
    printf("Sigma:\n");
    GOSTHashPrintDebug(CTX->Sigma);
#endif

    memcpy(CTX->hash, CTX->h, BLOCK_SIZE);
}

void
GOSTHashUpdate(TGOSTHashContext *CTX, const uint8_t *data, size_t len)
{
    size_t chk_size;

    while((len > 63) && (CTX->buf_size) == 0)
    {
        GOSTHashStage_2(CTX, data);
        data += 64;
        len -= 64;
    }
    while (len)
    {
        chk_size = 64 - CTX->buf_size;
        if (chk_size > len)
            chk_size = len;
        memcpy(&CTX->buffer[CTX->buf_size], data, chk_size);
        CTX->buf_size += chk_size;
        len -= chk_size;
        data += chk_size;
        if (CTX->buf_size == 64)
        {
            GOSTHashStage_2(CTX, CTX->buffer);
            CTX->buf_size = 0;
        }
    }
}
void
GOSTHashFinal(TGOSTHashContext *CTX)
{
    GOSTHashStage_3(CTX);
    CTX->buf_size = 0;
}
