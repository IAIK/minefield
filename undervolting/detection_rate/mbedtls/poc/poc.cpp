#include "poc.h"

#include "mbedtls/bignum.h"
#include "mbedtls/error.h"
#include "mbedtls/rsa.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define BUF_SIZE 40000

char buffer[BUF_SIZE];

#define CHECK(_x)                                                  \
    if ( (_x) != 0 ) {                                             \
        report_printf("error in crypo init line: %u\n", __LINE__); \
        return -1;                                                 \
    }

static void report_printf(char const *fmt, ...) {
    char    message_buffer[1000];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(message_buffer, 999, fmt, argptr);
    va_end(argptr);
    message_buffer[999] = 0;
    ocall_puts(message_buffer);
}

static mbedtls_rsa_context rsa_ctx;
static mbedtls_mpi         message;
static mbedtls_mpi         cipher;
static mbedtls_mpi         plain;
static mbedtls_mpi         tmp;
static mbedtls_mpi         oracle_D;
static mbedtls_mpi         oracle_reverse_D;

int crypto_encrypt_raw(mbedtls_rsa_context *ctx, mbedtls_mpi const *message, mbedtls_mpi *cipher) {
    return mbedtls_mpi_exp_mod(cipher, message, &ctx->E, &ctx->N, &ctx->RN);
}

int crypto_decrypt_raw(mbedtls_rsa_context *ctx, mbedtls_mpi const *cipher, mbedtls_mpi *plain) {
    return mbedtls_mpi_exp_mod(plain, cipher, &ctx->D, &ctx->N, &ctx->RN);
}

static int crypto_init_context(mbedtls_rsa_context *p_rsa_ctx, const char *p, const char *q, const char *e,
                               const char *d) {

    mbedtls_rsa_init(p_rsa_ctx, MBEDTLS_RSA_PKCS_V15, 0);

    mbedtls_mpi P, Q, E, D, D_check;
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&D_check);

    mbedtls_mpi_init(&oracle_D);
    mbedtls_mpi_init(&oracle_reverse_D);

    CHECK(mbedtls_mpi_read_string(&P, 16, p));
    CHECK(mbedtls_mpi_read_string(&Q, 16, q));
    CHECK(mbedtls_mpi_read_string(&E, 16, e));
    CHECK(mbedtls_mpi_read_string(&D_check, 16, d));

    CHECK(mbedtls_rsa_import(p_rsa_ctx, NULL, &P, &Q, &D_check, &E));
    CHECK(mbedtls_rsa_complete(p_rsa_ctx));

    CHECK(mbedtls_rsa_check_pubkey(p_rsa_ctx));
    CHECK(mbedtls_rsa_check_privkey(p_rsa_ctx));

    CHECK(mbedtls_rsa_check_pub_priv(p_rsa_ctx, p_rsa_ctx));

    CHECK(mbedtls_rsa_export(p_rsa_ctx, NULL, &P, &Q, &D, &E));

    CHECK(mbedtls_rsa_check_pub_priv(p_rsa_ctx, p_rsa_ctx));

    CHECK(mbedtls_mpi_cmp_mpi(&D, &D_check));

    mbedtls_mpi_init(&message);
    mbedtls_mpi_init(&cipher);
    mbedtls_mpi_init(&plain);
    mbedtls_mpi_init(&tmp);

    char buffer[8000] = "ffaaffaaffaaffaa";

    CHECK(mbedtls_mpi_read_string(&message, 16, buffer));

    CHECK(crypto_encrypt_raw(p_rsa_ctx, &message, &cipher));
    CHECK(crypto_decrypt_raw(p_rsa_ctx, &cipher, &plain));

    CHECK(mbedtls_mpi_cmp_mpi(&message, &plain));

    return mbedtls_mpi_bitlen(&D);
}

extern "C" {

void crypto_init() {
    // https://www.mobilefish.com/services/rsa_key_generation/rsa_key_generation.php
    const char *p =
        "E745D5499F3E714E187DEBBD6D716B346679ACF650A6A1615F68A0E275B6705CFB45A59A8316E99B94D4590145C435AB3DE2A18E8843EF"
        "E7AB8FC920453A7C417F21DB0337C49ECB99F7CDDEBB028AF9E42B876C6E765419051D394B57B58CD7D667DA0E2C78D9C570A6A32BA92D"
        "248D59ABFAC7214EE1276B1A72C3D0FE9AA66574B491EC8E565C34FA88DD27AC6B8058654879B329A01E4312E113107BBA76EF6E07DCFC"
        "5E383158C00E7F6D7900C5DE483E9171DFB0B4F191C8020D8278FCF07FD8EDDD111E35D12CB53EA6B50384FD42166E21C6221257EA1464"
        "6C8C615D0C394AE94A681F1C94D45A3A895EBF86A0D5F4C19B16D3F201E8576D74B8CA81";

    const char *q =
        "CB01E4172F48EFBCAF0CF4CE021280F9D95A156E5140089CC5362A312834A4F335890A64D1F9218005154E0827CE010029166E7A567616"
        "1BAC404CD574F1C8A97AD30D4BCC2ECBD24419DA6BE8DB6C11F1DA8B4751A162FDE1AED2559F9BFE196E0B643A3A2F0C7E49E6E931C155"
        "42B230CDF9816E5B7E4F0A51A4A9ECA2C35AAD2794E757A165A0874FD149A413FBD4E0F5554E50D49D5A25410411E3C76B90EEFF7845A0"
        "DDE5282CB671D1C96CA5A961FBF7B2CF940CF4D51005B8D77049C66FC10B109EF138CBA7A99C7115411A773C12290BA635FF7C4604341D"
        "747DE3E43EEFF4DDE02CA887D544F8EA851361C2258ACDCBC0CD4706D17C2346F9072A39";

    const char *e = "10001";

    const char *d =
        "58f55661c343e1fe5e9eb490b90fc050eeb46f10a750d3d67ae77fc680763166dc6a6f240e34b5bd6644c8bfd931388ea9b9e3565b3d27"
        "d189e99b8beefe8a45fd7c0b79080d0ea9135a233db4ae35d6984709300c512413ee56ee84ec98fd92315477796af68a0d936a6e86a7fa"
        "cd677a84bd0b05554bc21f710e1c6d5b4fd2461b6f2964eedc1aa8f1f87061b0362d13ae93f6800ffc5bc76a2fac593beb668c44bd9819"
        "410bd2094099fb15adf88f603b8c6df68823748a476b810a4bbac232403e1c5a134a73d6d4ffe74b7142b936135cdb22f1a4f8834bfb46"
        "2b1210f3d2034d4fe1fb4047067edb01539a3be3ba73ed14db16d10726c697b5a5a179e1ac4446f4ab71bf718828e96e914add156046f3"
        "6a39bbe53247ef48c4421b7e7fa0d8f30765f06b8b1b203714d262a67510cc2d38d2a09476a54645ebc5fb7108081fb02b8fe221ae1401"
        "f5d46fd5f2cb412d1661d228b9ccc94a3226036baea1aac8103684ecbabf1b7be4704b3d1afd4ef69b24fffa3349fc95bbf9fa4256934a"
        "e07679c43aa59fa6aece36b67b354f2ed89854ca8ba593a0a4784ec003e395d8ba7c7243331ef98f890358e9ff42f5180f037989cd61f2"
        "8da61a1b6bed19bebe58cdf30af750402a3a87e3489e04bfbc5aa699076b7fa666b3da8b1af6b47e4c084eaa8210f250869632de1e0b39"
        "49b33fa0fced081cceb6b89e8fa25d5c01";

    crypto_init_context(&rsa_ctx, p, q, e, d);
}

void crypto_clear() {
    mbedtls_mpi_read_string(&tmp, 16, "0");
}

void crypto_run() {
    crypto_decrypt_raw(&rsa_ctx, &cipher, &tmp);
}

int crypto_is_faulted() {
    return mbedtls_mpi_cmp_mpi(&message, &tmp) != 0;
}
}

extern "C" {
volatile const uint64_t __fault_mul_factor = FACTOR;
volatile const uint64_t __fault_mul_init   = INIT;
volatile uint64_t       __fault_pending    = 0;
}

static uint64_t target_faulted          = 0;
static uint64_t target_and_trap_faulted = 0;
static uint64_t trap_faulted            = 0;

static uint64_t effective_faults = 0;
static uint64_t overall_faults   = 0;

static void __attribute__((noinline)) reset_pending_flag() {
    // put this inside a function as at the begining of the function the traps are checked.
    // reset the fault pending flag
    __fault_pending = 0;
}

static void __attribute__((noinline)) analyse(bool faulted) {

    bool b_result_faulted = faulted;
    bool b_trap_faulted   = __fault_pending != 0;

    // used for recall/fscore
    target_faulted += b_result_faulted;
    target_and_trap_faulted += b_result_faulted && b_trap_faulted;
    trap_faulted += b_trap_faulted;

    // used for mitigation rate
    effective_faults += b_result_faulted && !b_trap_faulted;
    overall_faults += b_result_faulted || b_trap_faulted;
}

extern "C" void ecall_init() {
    crypto_init();
}

extern "C" void ecall_experiment() {
    crypto_clear();

    // do this manually setup minefield here. as we want to only build the poc with minefield and not the complete
    // binary
    asm volatile("mov __fault_mul_init(%rip), %r12");
    asm volatile("mov __fault_mul_init(%rip), %r13");
    reset_pending_flag();

    // undervolted execution
    ocall_undervolt_begin();

    crypto_run();

    ocall_undervolt_end();

    analyse(crypto_is_faulted());
}

extern "C" void ecall_print_results(uint64_t current_iteration) {
    double fscore          = target_and_trap_faulted / (double)target_faulted;
    double mitigation_rate = 1.0 - (effective_faults / (double)overall_faults);

    report_printf("%5lu: fscore: %5.6f = %8llu / %8llu (%8llu) mitigation_rate: %5.6f = 1.0 - (%8llu / %8llu) \r",
                  current_iteration, fscore, target_and_trap_faulted, target_faulted, trap_faulted, mitigation_rate,
                  effective_faults, overall_faults);
}