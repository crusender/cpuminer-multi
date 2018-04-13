#include "miner.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "sha3/sph_jh.h"
#include "sha3/sph_keccak.h"
#include "sha3/sph_skein.h"
#include "sha3/sph_echo.h"
#include "sha3/sph_fugue.h"
#include "sha3/sph_bmw.h"
#include "sha3/gost_streebog.h"

void sonoa_hash(void *state, const void *input)
{

    uint8_t _ALIGN(128) hash[64];

    sph_echo512_context     ctx_echo;
    sph_skein512_context        ctx_skein;
    sph_fugue512_context        ctx_fugue;
    sph_bmw512_context          ctx_bmw;
    sph_jh512_context           ctx_jh;
    sph_keccak512_context       ctx_keccak;
    sph_gost512_context         ctx_gost;


        sph_echo512_init(&ctx_echo);
        sph_echo512(&ctx_echo, input, 80);
        sph_echo512_close(&ctx_echo, (void*) hash);

        sph_skein512_init(&ctx_skein);
        sph_skein512(&ctx_skein, (const void*) hash, 64);
        sph_skein512_close(&ctx_skein, (void*) hash);

        sph_fugue512_init(&ctx_fugue);
        sph_fugue512(&ctx_fugue, (const void*) hash, 64);
        sph_fugue512_close(&ctx_fugue, (void*) hash);

        sph_gost512_init(&ctx_gost);
        sph_gost512(&ctx_gost, (const void*) hash, 64);
        sph_gost512_close(&ctx_gost, (void*) hash);

        sph_bmw512_init(&ctx_bmw);
        sph_bmw512(&ctx_bmw, (const void*) hash, 64);
        sph_bmw512_close(&ctx_bmw, (void*) hash);

        sph_jh512_init(&ctx_jh);
        sph_jh512(&ctx_jh, (const void*) hash, 64);
        sph_jh512_close(&ctx_jh, (void*) hash);

        sph_keccak512_init(&ctx_keccak);
        sph_keccak512(&ctx_keccak, (const void*) hash, 64);
        sph_keccak512_close(&ctx_keccak, (void*) hash);


        memcpy(state, hash, 32);
}

int scanhash_sonoa(int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done)
{
        uint32_t _ALIGN(128) hash32[8];
        uint32_t _ALIGN(128) endiandata[20];
        uint32_t *pdata = work->data;
        uint32_t *ptarget = work->target;
        const uint32_t first_nonce = pdata[19];
        const uint32_t Htarg = ptarget[7];
        uint32_t n = pdata[19] - 1;

        uint64_t htmax[] = {
                0,
                0xF,
                0xFF,
                0xFFF,
                0xFFFF,
                0x10000000
        };
        uint32_t masks[] = {
                0xFFFFFFFF,
                0xFFFFFFF0,
                0xFFFFFF00,
                0xFFFFF000,
                0xFFFF0000,
                0
        };

        // we need bigendian data...
        for (int i=0; i < 19; i++) {
                be32enc(&endiandata[i], pdata[i]);
        }

#ifdef DEBUG_ALGO
        printf("[%d] Htarg=%X\n", thr_id, Htarg);
#endif
        for (int m=0; m < 6; m++) {
                if (Htarg <= htmax[m]) {
                        uint32_t mask = masks[m];
                        do {
                                pdata[19] = ++n;
                                be32enc(&endiandata[19], n);
                                sonoa_hash(hash32, endiandata);
#ifndef DEBUG_ALGO
                                if ((!(hash32[7] & mask)) && fulltest(hash32, ptarget)) {
                                        work_set_target_ratio(work, hash32);
                                        *hashes_done = n - first_nonce + 1;
                                        return 1;
                                }
#else
                                if (!(n % 0x1000) && !thr_id) printf(".");
                                if (!(hash32[7] & mask)) {
                                        printf("[%d]",thr_id);
                                        if (fulltest(hash32, ptarget)) {
                                                work_set_target_ratio(work, hash32);
                                                *hashes_done = n - first_nonce + 1;
                                                return 1;
                                        }
                                }
#endif
                        } while (n < max_nonce && !work_restart[thr_id].restart);
                        // see blake.c if else to understand the loop on htmax => mask
                        break;
                }
        }

        *hashes_done = n - first_nonce + 1;
        pdata[19] = n;
        return 0;
}