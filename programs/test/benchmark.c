/*
 *  Benchmark demonstration program
 *
 *  Copyright (C) 2006-2013, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "polarssl/config.h"
#include "polarssl/timing.h"

#include "polarssl/md4.h"
#include "polarssl/md5.h"
#include "polarssl/sha1.h"
#include "polarssl/sha256.h"
#include "polarssl/sha512.h"
#include "polarssl/arc4.h"
#include "polarssl/des.h"
#include "polarssl/aes.h"
#include "polarssl/blowfish.h"
#include "polarssl/camellia.h"
#include "polarssl/gcm.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/rsa.h"
#include "polarssl/dhm.h"
#include "polarssl/havege.h"

#define BUFSIZE         1024
#define HEADER_FORMAT   "  %-16s :  "
#define TITLE_LEN       17

#if !defined(POLARSSL_TIMING_C)
int main( int argc, char *argv[] )
{
    ((void) argc);
    ((void) argv);

    printf("POLARSSL_TIMING_C not defined.\n");
    return( 0 );
}
#else

static int myrand( void *rng_state, unsigned char *output, size_t len )
{
    size_t use_len;
    int rnd;

    if( rng_state != NULL )
        rng_state  = NULL;

    while( len > 0 )
    {
        use_len = len;
        if( use_len > sizeof(int) )
            use_len = sizeof(int);

        rnd = rand();
        memcpy( output, &rnd, use_len );
        output += use_len;
        len -= use_len;
    }

    return( 0 );
}

#define TIME_AND_TSC( TITLE, CODE )                                     \
do {                                                                    \
    unsigned long i, j, tsc;                                            \
                                                                        \
    printf( HEADER_FORMAT, TITLE );                                     \
    fflush( stdout );                                                   \
                                                                        \
    set_alarm( 1 );                                                     \
    for( i = 1; ! alarmed; i++ )                                        \
    {                                                                   \
        CODE;                                                           \
    }                                                                   \
                                                                        \
    tsc = hardclock();                                                  \
    for( j = 0; j < 1024; j++ )                                         \
    {                                                                   \
        CODE;                                                           \
    }                                                                   \
                                                                        \
    printf( "%9lu Kb/s,  %9lu cycles/byte\n", i * BUFSIZE / 1024,       \
                    ( hardclock() - tsc ) / ( j * BUFSIZE ) );          \
} while( 0 )

#define TIME_PUBLIC( TITLE, TYPE, CODE )                                \
do {                                                                    \
    unsigned long i;                                                    \
    int ret;                                                            \
                                                                        \
    printf( HEADER_FORMAT, TITLE );                                     \
    fflush( stdout );                                                   \
    set_alarm( 3 );                                                     \
                                                                        \
    ret = 0;                                                            \
    for( i = 1; ! alarmed && ! ret ; i++ )                              \
    {                                                                   \
        CODE;                                                           \
    }                                                                   \
                                                                        \
    if( ret != 0 )                                                      \
        printf( "FAILED\n" );                                           \
    else                                                                \
        printf( "%9lu " TYPE "/s\n", i / 3 );                                  \
} while( 0 )

unsigned char buf[BUFSIZE];

int main( int argc, char *argv[] )
{
    int keysize;
    unsigned char tmp[64];
    char title[TITLE_LEN];
    ((void) argc);
    ((void) argv);

    memset( buf, 0xAA, sizeof( buf ) );

    printf( "\n" );

#if defined(POLARSSL_MD4_C)
    TIME_AND_TSC( "MD4", md4( buf, BUFSIZE, tmp ) );
#endif

#if defined(POLARSSL_MD5_C)
    TIME_AND_TSC( "MD5", md5( buf, BUFSIZE, tmp ) );
#endif

#if defined(POLARSSL_SHA1_C)
    TIME_AND_TSC( "SHA-1", sha1( buf, BUFSIZE, tmp ) );
#endif

#if defined(POLARSSL_SHA256_C)
    TIME_AND_TSC( "SHA-256", sha256( buf, BUFSIZE, tmp, 0 ) );
#endif

#if defined(POLARSSL_SHA512_C)
    TIME_AND_TSC( "SHA-512", sha512( buf, BUFSIZE, tmp, 0 ) );
#endif

#if defined(POLARSSL_ARC4_C)
    {
        arc4_context arc4;
        arc4_setup( &arc4, tmp, 32 );
        TIME_AND_TSC( "ARC4", arc4_crypt( &arc4, BUFSIZE, buf, buf ) );
    }
#endif

#if defined(POLARSSL_DES_C) && defined(POLARSSL_CIPHER_MODE_CBC)
    {
        des3_context des3;
        des3_set3key_enc( &des3, tmp );
        TIME_AND_TSC( "3DES",
                des3_crypt_cbc( &des3, DES_ENCRYPT, BUFSIZE, tmp, buf, buf ) );
    }

    {
        des_context des;
        des_setkey_enc( &des, tmp );
        TIME_AND_TSC( "DES",
                des_crypt_cbc( &des, DES_ENCRYPT, BUFSIZE, tmp, buf, buf ) );
    }
#endif

#if defined(POLARSSL_AES_C)
#if defined(POLARSSL_CIPHER_MODE_CBC)
    {
        aes_context aes;
        for( keysize = 128; keysize <= 256; keysize += 64 )
        {
            snprintf( title, sizeof( title ), "AES-CBC-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            aes_setkey_enc( &aes, tmp, keysize );

            TIME_AND_TSC( title,
                    aes_crypt_cbc( &aes, AES_ENCRYPT, BUFSIZE, tmp, buf, buf ) );
        }
    }
#endif
#if defined(POLARSSL_GCM_C)
    {
        gcm_context gcm;
        for( keysize = 128; keysize <= 256; keysize += 64 )
        {
            snprintf( title, sizeof( title ), "AES-GCM-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            gcm_init( &gcm, POLARSSL_CIPHER_ID_AES, tmp, keysize );

            TIME_AND_TSC( title,
                    gcm_crypt_and_tag( &gcm, GCM_ENCRYPT, BUFSIZE, tmp,
                        12, NULL, 0, buf, buf, 16, tmp ) );
        }
    }
#endif
#endif

#if defined(POLARSSL_CAMELLIA_C) && defined(POLARSSL_CIPHER_MODE_CBC)
    {
        camellia_context camellia;
        for( keysize = 128; keysize <= 256; keysize += 64 )
        {
            snprintf( title, sizeof( title ), "CAMELLIA-CBC-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            camellia_setkey_enc( &camellia, tmp, keysize );

            TIME_AND_TSC( title,
                    camellia_crypt_cbc( &camellia, CAMELLIA_ENCRYPT,
                        BUFSIZE, tmp, buf, buf ) );
        }
    }
#endif

#if defined(POLARSSL_BLOWFISH_C) && defined(POLARSSL_CIPHER_MODE_CBC)
    {
        blowfish_context blowfish;
        for( keysize = 128; keysize <= 256; keysize += 64 )
        {
            snprintf( title, sizeof( title ), "BLOWFISH-CBC-%d", keysize );

            memset( buf, 0, sizeof( buf ) );
            memset( tmp, 0, sizeof( tmp ) );
            blowfish_setkey( &blowfish, tmp, keysize );

            TIME_AND_TSC( title,
                    blowfish_crypt_cbc( &blowfish, BLOWFISH_ENCRYPT, BUFSIZE,
                        tmp, buf, buf ) );
        }
    }
#endif

#if defined(POLARSSL_HAVEGE_C)
    {
        havege_state hs;
        havege_init( &hs );
        TIME_AND_TSC( "HAVEGE", havege_random( &hs, buf, BUFSIZE ) );
    }
#endif

#if defined(POLARSSL_CTR_DRBG_C)
    {
        ctr_drbg_context    ctr_drbg;

        if( ctr_drbg_init( &ctr_drbg, myrand, NULL, NULL, 0 ) != 0 )
            exit(1);
        TIME_AND_TSC( "CTR_DRBG (NOPR)",
                if( ctr_drbg_random( &ctr_drbg, buf, BUFSIZE ) != 0 )
                exit(1) );

        if( ctr_drbg_init( &ctr_drbg, myrand, NULL, NULL, 0 ) != 0 )
            exit(1);
        ctr_drbg_set_prediction_resistance( &ctr_drbg, CTR_DRBG_PR_ON );
        TIME_AND_TSC( "CTR_DRBG (PR)",
                if( ctr_drbg_random( &ctr_drbg, buf, BUFSIZE ) != 0 )
                exit(1) );
    }
#endif

#if defined(POLARSSL_RSA_C) && defined(POLARSSL_GENPRIME)
    {
        rsa_context rsa;
        for( keysize = 1024; keysize <= 4096; keysize *= 2 )
        {
            snprintf( title, sizeof( title ), "RSA-%d", keysize );

            rsa_init( &rsa, RSA_PKCS_V15, 0 );
            rsa_gen_key( &rsa, myrand, NULL, keysize, 65537 );

            TIME_PUBLIC( title, " public",
                    buf[0] = 0;
                    ret = rsa_public( &rsa, buf, buf ) );

            TIME_PUBLIC( title, "private",
                    buf[0] = 0;
                    ret = rsa_private( &rsa, myrand, NULL, buf, buf ) );

            rsa_free( &rsa );
        }
    }
#endif

#if defined(POLARSSL_DHM_C) && defined(POLARSSL_BIGNUM_C)
    {
#define DHM_SIZES 3
        int sizes[DHM_SIZES] = { 1024, 2048, 3072 };
        const char *dhm_P[DHM_SIZES] = {
            POLARSSL_DHM_RFC5114_MODP_1024_P,
            POLARSSL_DHM_RFC3526_MODP_2048_P,
            POLARSSL_DHM_RFC3526_MODP_3072_P,
        };
        const char *dhm_G[DHM_SIZES] = {
            POLARSSL_DHM_RFC5114_MODP_1024_G,
            POLARSSL_DHM_RFC3526_MODP_2048_G,
            POLARSSL_DHM_RFC3526_MODP_3072_G,
        };

        dhm_context dhm;
        size_t olen;
        for( keysize = 0; keysize < DHM_SIZES; keysize++ )
        {
            memset( &dhm, 0, sizeof( dhm_context ) );

            mpi_read_string( &dhm.P, 16, dhm_P[keysize] );
            mpi_read_string( &dhm.G, 16, dhm_G[keysize] );
            dhm.len = mpi_size( &dhm.P );
            dhm_make_public( &dhm, dhm.len, buf, dhm.len, myrand, NULL );
            mpi_copy( &dhm.GY, &dhm.GX );

            snprintf( title, sizeof( title ), "DHM-%d", sizes[keysize] );
            TIME_PUBLIC( title, "handshake",
                    olen = sizeof( buf );
                    ret |= dhm_make_public( &dhm, dhm.len, buf, dhm.len,
                                            myrand, NULL );
                    ret |= dhm_calc_secret( &dhm, buf, &olen, myrand, NULL ) );

            snprintf( title, sizeof( title ), "DHM-%d-fixed", sizes[keysize] );
            TIME_PUBLIC( title, "handshake",
                    olen = sizeof( buf );
                    ret |= dhm_calc_secret( &dhm, buf, &olen, myrand, NULL ) );

            dhm_free( &dhm );
        }
    }
#endif

    printf( "\n" );

#if defined(_WIN32)
    printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( 0 );
}

#endif /* POLARSSL_TIMING_C */
