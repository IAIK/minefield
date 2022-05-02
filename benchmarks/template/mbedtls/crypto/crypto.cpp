#include "mbedtls/bignum.h"
#include "mbedtls/error.h"
#include "mbedtls/rsa.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define BUF_SIZE 40000

char buffer[BUF_SIZE];

unsigned char cipher_raw[4096] = "Some Cipher Text.";
unsigned char tmp_raw[4096];

#define CHECK(_x)                                                   \
    if ( (_x) != 0 ) {                                              \
        enclave_printf("error in crypo init line: %u\n", __LINE__); \
        return -1;                                                  \
    }

void ocall_give_hint(int running);

extern void enclave_puts(char const *str);

void enclave_printf(char const *fmt, ...) {
    char    message_buffer[1000];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(message_buffer, 999, fmt, argptr);
    va_end(argptr);
    message_buffer[999] = 0;
    enclave_puts(message_buffer);
}

typedef enum {
    Key_256o                                                         = 0,
    Key_256o_256z                                                    = 1,
    Key_128o_128z_128o_128z                                          = 2,
    Key_64o_64z_64o_64z_64o_64z_64o_64z                              = 3,
    Key_64o_64z_64o_192z_64o_64z                                     = 4,
    Key_32o_32z_32o_32z_32o_32z_32o_32z_128z_64o_64z                 = 5,
    Key_32o_32z_32o_32z_16o_16z_16o_16z_16o_16z_16o_16z_128z_64o_64z = 6,
    Key_32oz_32oz_16oz_16oz_8oz_8oz_8oz_8oz_128z_64oz                = 7,
    Key_2x32oz_2x16oz_4x8oz_peaks_64oz                               = 8,
    Key_2x32oz_2x16oz_4x8oz_4x5h_64oz                                = 9,
    Key_2x32oz_2x16oz_4x8oz_2x0fh_64oz                               = 10,
    Key_2x32oz_2x16oz_4x8oz_4x33h_64oz                               = 11,
    Key1_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz                          = 12,
    Key2_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz                          = 13,
    Key3_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz                          = 14,
    Key0_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz                          = 15,
    Keym1_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz                         = 16,
    Keym1_2x32oz_2x16oz_192z_64oz                                    = 17,
    Key_rising_mulitple                                              = 18,
    Key_rising                                                       = 19,
    Key_short                                                        = 20,
    Key_even_shorter                                                 = 21,
    Key_bit_longer                                                   = 22,
    Key_bit_bit_longer                                               = 23,
    Key_only_leading_zeros                                           = 24,
    Key_short2                                                       = 25,
    Key_real_512                                                     = 26,
    Key_real_2048                                                    = 27,
    Key_short_2048                                                   = 28,
    Key_short_2048_inv                                               = 29
} key_index_t;

typedef struct __attribute__((packed)) {
    const char *p_str;
    const char *q_str;
    const char *e_str;
    const char *d_str;
} rsa_data_t;

static void get_key(key_index_t key, rsa_data_t *data);

static mbedtls_rsa_context rsa_ctx;
static mbedtls_mpi         message;
static mbedtls_mpi         cipher;
static mbedtls_mpi         plain;
static mbedtls_mpi         tmp;
static mbedtls_mpi         oracle_D;
static mbedtls_mpi         oracle_reverse_D;

void print_mpi(mbedtls_mpi *mpi) {
    char   buffer[4000];
    size_t out_len = 0;
    mbedtls_mpi_write_string(mpi, 16, buffer, 4000, &out_len);
    buffer[out_len] = 0;
    enclave_puts(buffer);
}

int crypto_encrypt_raw(mbedtls_rsa_context *ctx, mbedtls_mpi const *message, mbedtls_mpi *cipher) {
    return mbedtls_mpi_exp_mod(cipher, message, &ctx->E, &ctx->N, &ctx->RN);
}

int crypto_decrypt_raw(mbedtls_rsa_context *ctx, mbedtls_mpi const *D, mbedtls_mpi const *cipher, mbedtls_mpi *plain) {
    return mbedtls_mpi_exp_mod(plain, cipher, D, &ctx->N, &ctx->RN);
}

int crypto_encrypt(mbedtls_rsa_context *ctx, unsigned char *message, unsigned char *cipher) {
    return mbedtls_rsa_public(ctx, message, cipher);
}

int crypto_decrypt(mbedtls_rsa_context *ctx, unsigned char *cipher, unsigned char *plain) {
    return mbedtls_rsa_private(ctx, NULL, NULL, cipher, plain);
}

int crypto_decrypt_no_check(mbedtls_rsa_context *ctx, unsigned char *cipher, unsigned char *plain) {
    return mbedtls_rsa_private_no_check(ctx, NULL, NULL, cipher, plain);
}

static int crypto_init_context(mbedtls_rsa_context *p_rsa_ctx, const char *p, const char *q, const char *e,
                               const char *d) {
    // CHECK(mbedtls_rsa_self_test(1));

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

    /*enclave_puts("P: ");
    print_mpi(&P);
    enclave_puts("Q: ");
    print_mpi(&Q);
    enclave_puts("E: ");
    print_mpi(&E);
    enclave_puts("D: ");
    print_mpi(&D);
    enclave_puts("D_check: ");
    print_mpi(&D_check); */

    CHECK(mbedtls_mpi_cmp_mpi(&D, &D_check));

    mbedtls_mpi_init(&message);
    mbedtls_mpi_init(&cipher);
    mbedtls_mpi_init(&plain);
    mbedtls_mpi_init(&tmp);

    char buffer[8000] = "ffaaffaaffaaffaa";

    CHECK(mbedtls_mpi_read_string(&message, 16, buffer));

    CHECK(crypto_encrypt_raw(p_rsa_ctx, &message, &cipher));
    CHECK(crypto_decrypt_raw(p_rsa_ctx, &p_rsa_ctx->D, &cipher, &plain));

    // CHECK(mbedtls_mpi_write_binary_le(&cipher, cipher_raw, sizeof(cipher_raw)));
    /*enclave_puts("MESSAGE: ");
    print_mpi(&message);
    enclave_puts("CIPHER:  ");
    print_mpi(&cipher);
    enclave_puts("PLAIN:   ");
    print_mpi(&plain);*/
    CHECK(mbedtls_mpi_cmp_mpi(&message, &plain));
    // mbedtls_mpi_grow(&D, E.n);
    return mbedtls_mpi_bitlen(&D);
}

int enclave_init(int key_index) {
    rsa_data_t rsa_key;
    get_key((key_index_t)key_index, &rsa_key);
    return crypto_init_context(&rsa_ctx, rsa_key.p_str, rsa_key.q_str, rsa_key.e_str, rsa_key.d_str);
}

void enclave_encrypt() {
    crypto_encrypt_raw(&rsa_ctx, &message, &cipher);
}

void enclave_decrypt() {
    crypto_decrypt_raw(&rsa_ctx, &rsa_ctx.D, &cipher, &plain);
}

int enclave_oracle() {
    crypto_decrypt_raw(&rsa_ctx, &oracle_D, &cipher, &tmp);
    return mbedtls_mpi_cmp_mpi(&message, &tmp) == 0;
}

int enclave_test() {
    crypto_decrypt_raw(&rsa_ctx, &rsa_ctx.D, &cipher, &tmp);
    return mbedtls_mpi_cmp_mpi(&message, &tmp) == 0;
}

static void parse_reverse_key() {
    mbedtls_mpi_shrink(&oracle_reverse_D, 0);
    size_t bitlen = mbedtls_mpi_bitlen(&oracle_reverse_D);
    size_t rem    = bitlen % 64;

    bitlen += rem != 0 ? (64 - rem) : 0;
    mbedtls_mpi_lset(&oracle_D, 0);

    for ( size_t i = 0; i < bitlen; ++i ) {
        int bit = mbedtls_mpi_get_bit(&oracle_reverse_D, i);
        mbedtls_mpi_set_bit(&oracle_D, bitlen - i - 1, bit);
    }
}

void enclave_oracle_set_bit(int index, int value) {
    mbedtls_mpi_set_bit(&oracle_reverse_D, index, (unsigned char)value);
    parse_reverse_key();
}

int enclave_oracle_get_bit(int index) {
    return mbedtls_mpi_get_bit(&oracle_reverse_D, index);
}

int enclave_oracle_write_key(char *buffer, size_t N) {
    size_t output_len = 0;
    return mbedtls_mpi_write_string(&oracle_D, 2, buffer, N, &output_len);
}

int enclave_victim_write_key(char *buffer, size_t N) {
    size_t output_len = 0;
    return mbedtls_mpi_write_string(&rsa_ctx.D, 2, buffer, N, &output_len);
}

void enclave_oracle_clear_key() {
    mbedtls_mpi_lset(&oracle_reverse_D, 0);
    parse_reverse_key();
}

void enclave_get_encrypt_adr(void **adr) {
    *adr = (void *)mbedtls_mpi_exp_mod;
}

void enclave_get_decrypt_adr(void **adr) {
    *adr = (void *)mbedtls_mpi_exp_mod;
}

static void get_key(key_index_t key, rsa_data_t *data) {
    // https://www.mobilefish.com/services/rsa_key_generation/rsa_key_generation.php

    const char *p_str_m2 = "cf154d09df730ff558a963e2d2a5325d";
    const char *q_str_m2 = "887b440072ee09b3506d27ef6c4ead8d";

    const char *p_str_m1 = "dbf46d1853f9eec30cce7e414705ea5234ed98cc4c5919bdacc4decc6352cd91";
    const char *q_str_m1 = "9769a631dd7b5d9d7b0247f1c41014b09b9ba50d7c445b674090ab95381525fd";

    const char *p_str_0 = "facc9a5727e7a4e4ee0e8f0c5263865de9a3d67d480a91c1f6afea3fa2982c5989bb9ed2de382ac2ced5e71129cb"
                          "24a2d6e5f42ee8ebcd301ba53d543f2b8597";
    const char *q_str_0 = "f9ece5b2756a0fc5f59a6a735df7d60f379770ed5e6dfa0c04b1cdc98ec36eae42aaa7e9763fb1335d878c02bc9e"
                          "0a3c9363c73d47524781251a461bdf53355f";

    const char *p_str_1 = "e980638b97e6349c4bbf1bbdd9370b5ce418a9fa41f614ee8d829429a164ad8d7509a26b6285fb33caaf4763c101"
                          "204b13b9d651f2a51bd2ac0eb92277adbd96e04e80fe2f2c76e43fadb32d622c1c44d8c6809556827c306c684d11"
                          "51f0cac97110346f709a0f0456f828593f80bdd680b2cd3e52c03088df6589afa236a40f";
    const char *q_str_1 = "e47bb9d94cf88e1cbec5dc1cda240659820069b1666f3a12b1b43d77df52e4ff8855b79c50ffac650cf374e6144a"
                          "85563a08137a744ffff7344ebfc7eb04b9059a8d6d3263093a1b3a3ddacbf0082fab7d45c169ac036d76496f1be7"
                          "9a15ccc8a82aec5881e728718112c6f47af505e0b802ab0d92802657614acfca45e93151";

    const char *p_str_2 =
        "E745D5499F3E714E187DEBBD6D716B346679ACF650A6A1615F68A0E275B6705CFB45A59A8316E99B94D4590145C435AB3DE2A18E8843EF"
        "E7AB8FC920453A7C417F21DB0337C49ECB99F7CDDEBB028AF9E42B876C6E765419051D394B57B58CD7D667DA0E2C78D9C570A6A32BA92D"
        "248D59ABFAC7214EE1276B1A72C3D0FE9AA66574B491EC8E565C34FA88DD27AC6B8058654879B329A01E4312E113107BBA76EF6E07DCFC"
        "5E383158C00E7F6D7900C5DE483E9171DFB0B4F191C8020D8278FCF07FD8EDDD111E35D12CB53EA6B50384FD42166E21C6221257EA1464"
        "6C8C615D0C394AE94A681F1C94D45A3A895EBF86A0D5F4C19B16D3F201E8576D74B8CA81";
    const char *q_str_2 =
        "CB01E4172F48EFBCAF0CF4CE021280F9D95A156E5140089CC5362A312834A4F335890A64D1F9218005154E0827CE010029166E7A567616"
        "1BAC404CD574F1C8A97AD30D4BCC2ECBD24419DA6BE8DB6C11F1DA8B4751A162FDE1AED2559F9BFE196E0B643A3A2F0C7E49E6E931C155"
        "42B230CDF9816E5B7E4F0A51A4A9ECA2C35AAD2794E757A165A0874FD149A413FBD4E0F5554E50D49D5A25410411E3C76B90EEFF7845A0"
        "DDE5282CB671D1C96CA5A961FBF7B2CF940CF4D51005B8D77049C66FC10B109EF138CBA7A99C7115411A773C12290BA635FF7C4604341D"
        "747DE3E43EEFF4DDE02CA887D544F8EA851361C2258ACDCBC0CD4706D17C2346F9072A39";

    const char *p_str_3 =
        "ff6e8a4b012442ed83589eda5d1ef53135c5d337aff04acd45a9b04f61c66c6034f6ab351bee229e46c178a3df6dd51432a84ce19896ae"
        "dd3f5e2ca0249800598fac8eaa9ab148e072c083512d58665f00b2409948ee76874ca327713b27cacad358746d09bc4d7b7cd1cda92933"
        "082ae548c32d3639b483dcdbf41802bee2d61657432013919b906268398b9b42b5808a78adc1b261448e3c38a63ff689150ce0582cf3e9"
        "c8834b30bdd7ccff8c4c8905fa4ab75b8e4598c6ecd7bc75ecc95544f5482ebdcc79ee8950ba88dd0611e2005c995ceadef4b1cacb6d3f"
        "b73694fb2cdd29e9a9a3d732f391bad098cbb298274bf4122ae8c3f4b330346a67c86693a58f7c4e32548026e53af67d06a9162df40a3f"
        "1ef30213a63347119a0f772db506a90339fd845fe3be5560c4fe76f2f38131117434241a676b2b017ba22e30a9d733e2d5b741b8769618"
        "3a958f2c88105e998492be9ef929cafaa46c28d415ff08f7d4bf388c35cfa57064349cec44fa85c88c4366856d307f119da7e6b2a7eeaa"
        "7bade208e3b1ad5315e1ef0125ea87ed76ca53fc37847b4921232fd3ca1d7d45ca604dad11fa0027dbf02800daafed507a414cb1c5c8fa"
        "e6eb29b9b54637fa119748bea842bfcf1eaff81e89a70cd9214d26d312eecdf8d9f7b18e3de8884e7fd00d2813ccf7317bef4f04975bdd"
        "b8a8038bbaf159b2ca5064588d4ce1dccd";
    const char *q_str_3 =
        "b8fbe0df89f5f443aa65eab57fbc521f3094d2c47d2a4c78e7b1cf366ac8f322c3d3014cc8f68f735b7928da357bf9650fcc298cf67504"
        "dd27d313919b06b45ed9b5aaa2ab391506b23de0e7b8da4cc7a675539ceb5879122b8ca3c79257256b0dd00db80922bfbc609fa31879f0"
        "e01f6533bb508b3462b7c0067e525da3be6a73cb237deee2394e81dc66fbcc461a327ad682050ae5bccbeb3a1d48dedbb351a7d6ce95b8"
        "07e339e44934b0904fe20a25650f2f4accfa3506187de4f330369a06042fbe0c12f8f05c2ca0ec7b83f6a0e805d032d1ca67d8e2e4bcd6"
        "15f2701bf7a0da03b2d2f30efe93fa7a16f01afef6ea30c7b5979f977c5c6d914edeac457e864830583b3f57cf39d1e52aebe420d614e7"
        "8bac872228e390b9316e6577ececbf1c44fb5b9bca956cafbf5893df2ea8c6b9544ddb7c306b3562d99ad9c58698e2cb933d665dd2d30d"
        "912ba070594837129c84ed18b5594d1732e8f0f86e88da391c17a079ea73867b9e27ecf76f23090ca7f6245c4f7273825a637ac8ec5402"
        "086ba281551508d1b32fbdaa9ee0b3aa87a9cfe4f941577b9bbbf563a74965afb407300a1c38dbcf67987906782cb0ffd67fe3cfc03b4a"
        "5bf63fbf9d88244c3c2c7c4648b521181cb9912231bed33194521463134fad6e45ec2de274522eb07eabf7c66018de7cd2709f032e7234"
        "c2c44925d798d246debe73c032c5efe243";

    switch ( key ) {
        case Key_256o:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "b531fb97fb9cb7db28b7c8af0fd70c60ffe7b3faeceb9b8162c228ed5f9598544c9d02259f59a3c2fb7fe6f85044"
                          "e839515c51f9f1b93d46ffd7c524547b9b965948ea8eabf924d90f15f67ef006c8cc7fc035c01c502b49f99bb724"
                          "d706efc0965b9d1a8d77f065daaca2d8b92f6dd17d469ea00c5b80fee7ca6db8ab38132c9b94a5c8bbba1c720de7"
                          "a4363278707d9c4d0002f739825f1a4f8e7184166228034a42f0ec3ef1c00a1a2823bad92c908947d92d4c22c0dc"
                          "c0a35e0571ad123076559c7f546604c5a3bbc547d8ff0c7a34944e790ac635453f31bd7d23f3c6d5cc258f597049"
                          "94db01e9ac9e8487579af7ef7d672476c71058baa43f9459159f";
            data->d_str = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
            break;

        case Key_256o_256z:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "5d2e79c7213474294c91f76f890c5e125b85597da1cb75fad901528b4719b04d5de8cba142ac6ed5fa6da22a6a2b"
                          "dc428fd644f0a81f5548ad2dabfae5c95cc6d30d9873a00e1ae89141b11cc53a3b2d982862644a49cb6ddf4d8ad3"
                          "9c3d07c372b207c0562eaada27ff4a3fd8bce24d2d40a5aec672538da8d2148d4c99863513b8222434f7a522616f"
                          "379feb54517c0452be93c43508d92ccc5572ad5da7baaa0ff51308f2ecea090a25dfb0245a0ca2db638c2b126ae7"
                          "a16a72d85e83e75044738ba94a01c010d864b3bae790ebc759fba412e322778ea74a1a0db2b972baf0e5b989edcc"
                          "656f27c886deb9c3c6717054e6b65011450c4ab5f38a32d6d161";
            data->d_str = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000000000000000"
                          "000000000000000000000000000000000001";
            break;

        case Key_128o_128z_128o_128z:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "3b4a67e0ca62c8a5c5d2d6ecd3c80681a27f346f82c2d34728fe3917805152908ac589ab14f62c0532aa3260d3f3"
                          "f4d8542967fbd096ed242de242d208582912fc9897525cc5bc066786a83271cc8fcc78a57acb388bb8fd38cad127"
                          "05a10ec06123cdf83a3598fa3c28b2c07e07744acc4a7535a3216ef2d69b23b05776209435a3ae963d0c20d6bc15"
                          "c77771a57f93d7045f4a273670af2d7f78aad1e3598150073351cb514f382cf82898efb94f19fb98d9f96e529b9a"
                          "9f3a34e385a85e12fe7e9cf5ae15a09f43e56468da8cd204df0ed589569ad7864b22e79dcc6b8ac8a69246cb14a0"
                          "62c62d401a9bad55fc541ac2931dcf0328e458f403d85dffd321";
            data->d_str = "ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffff"
                          "ffff00000000000000000000000000000001";
            break;

        case Key_64o_64z_64o_64z_64o_64z_64o_64z:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "e66ee097e5dcb13d947833d3eb0e5d57bb95b389a02a3799b902d050276994b66bf04213c3bef62b0b336e1de767"
                          "3ecd494e8e9e1fced17cf65208105094081fca8113dbbe4a0cb45c6d9b744c8a1a5bcd1ca35de8cb0bcb3013fc26"
                          "030a89d92f8057ec2b684828a9413dde729b587fa1a2574d8018200bf85dfa890b7cb538564d0160a4d6463b4845"
                          "22460fe7f2551295e73b600d4274e794374cf072d5df99af46d128a098558e0546987bb8a01f369924bf5898841e"
                          "928c26a48f94dcfe1396da62874fe4daf70c389b5485497814672ab61e4914607ed9a2c9faa0533af28ec3075881"
                          "422fe1744140d77c86e0df726985caa4ff0846b6f06e728a9e1";
            data->d_str = "ffffffffffffffff0000000000000000ffffffffffffffff0000000000000000ffffffffffffffff000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_64o_64z_64o_192z_64o_64z:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "8828ac350be873e6fbbfeadc195a390dbb45037a95fbd04d84f988427494af279d002b32a3ac294be09d145a3ad9"
                          "010ba0ee7910ffaeec26cf66493db958c81dab7e819c43f8b0a4b94e7cc355899a980a861908ccf69bc23b14065f"
                          "027d95c16fdda2ef29616582e9090b1930945daf2e10d56d5ab69f738c8e96bdb1e2b35833a9ec694a29930d2143"
                          "a39dcc772f3b264254a46ea982d6cb5b696e91af820abe0ffd1ba7234f912c55192e64101a093d329b6de525039c"
                          "5a5a39233ddea5f566b2e748db6038f2b9a54f8c4a53fe9d232af32d5442b66426516f8026a8a915fd9178b094f6"
                          "2d18948fc9d8e5d026e5d6001bbf08c5a829c97bd9c79a038101";
            data->d_str = "ffffffffffffffff0000000000000000ffffffffffffffff00000000000000000000000000000000000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_32o_32z_32o_32z_32o_32z_32o_32z_128z_64o_64z:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "6d9b66c918203bfd85b7fa8f305008762d65ced340a5f6d88f5337b175788afc7c5c5c5bcc3b5ec3b59be083a6e4"
                          "8e0259e2bfc09e74aadc046e85719c008aa5fcc41b65d61ef591c99684321935bcee70931b981abd946d36777c49"
                          "4cc57c814466ca05e0a21a6c6808852f6bbda5331a25f9455024518bdcf1b98b74449f0b682db6143a1203f591bb"
                          "e27ac7671c1613880412bcca379bc6760886646278ed90c20eb2dad8df2351faac3959927cb610e9bee40b2fd5da"
                          "645b50184f58bf9496ba3f59155333191437225ee9529597112a10c71f8033da36798423fa8e6edc8b0b05fa8ecb"
                          "2a662e16f2b017d8d9db3b319e8b7bee69fb0a593065a62573c1";
            data->d_str = "ffffffff00000000ffffffff00000000ffffffff00000000ffffffff000000000000000000000000000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_32o_32z_32o_32z_16o_16z_16o_16z_16o_16z_16o_16z_128z_64o_64z:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "587afbdded2b0e092ec10c453be4db7c6e207476dfc248c74cd74dab0ee2fa6d28f3a4c803b02780213e4b30d885"
                          "6db71d596b28f19cf579331e9d352f50eb807dcc4bb23b181e8eea222182b7f23829dd3c1334e818400dbdbca933"
                          "a51a936bfaf83c5a8237f0ce39281f53e2629a72e0491917b3ef7d3425b38dff09d9cc65369b83b0334023b1c2b6"
                          "3c82aafba9546705cc6966d7bdcdc41f62549088d5aa92376762eaae6778f02adb30073b60a1a044cc661796487f"
                          "b6634cc8a7a27573edeee22f39ea554357fedbde3dab752dfc86a74a8de17d9320fdc84091128a252e1d0b68fa16"
                          "bb6e4f1c2f16c8ffb727ea20f4197e93196c6bd6b20ad6919701";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ffff0000ffff00000000000000000000000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_32oz_32oz_16oz_16oz_8oz_8oz_8oz_8oz_128z_64oz:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "2996323580202b972ec6f9c8f410f0a716e42676a0ef66a17eb8466a3e0b7721862a6c37dfdf256de195e3ef5d0d"
                          "bba5c233aae2e629b7b7bad1e1e79d52f1b3cf159b0c05d498e9492911249c7652cc31e4918513bae6dea7a4e372"
                          "ed8707be74bfd7baa19cde7ed856e9fa6d42e20f122bf774d184df7cfa468133736e8cadcb892497529cd6fd22f1"
                          "0e9c8494e4e289651b0774cbbc6a26cafca2d15e6d485e95b64215ce91fe71298211a86463369f1a67a695aa48bd"
                          "2c3f81248da85946db503fc1eedaf0da59f8214483090d8bf3d5df8d0a08ec3379aded777fe863d1235d7b312b28"
                          "e3476ed786e3fe2f68fae0a985439dd3a87b7257549b7d596861";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000000000000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_2x32oz_2x16oz_4x8oz_peaks_64oz:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "82cfa56b40fed19a440749d4d3bae276385b3424bee06f82abed5f47ed76c6640b8cbdc7d730e1aefb74b055c312"
                          "741259fad2ee9e8625ef531b196824bd48bc8a8359bd621a049f85cf280104c26aa7311c7ca3748cee25c5c891cc"
                          "1a9553732d71c72d2c3dd9005e7d1cfff13e3c7e03c31618af3a44d37dbdf5c4e36c9b3da02cc025f2966c7eb062"
                          "c19e5d9173cf07a1f3f7e59512dbbb3e0f9a920ccfd7e505febe868a6503ccd3f8a563314519eef8890cacb47a59"
                          "ec05630e2052a9606da00d96c748033dae9f74920857f21641342a3cc0cb25078545b8469bd2a9d37298094f1bcd"
                          "587ff5e293910a2af30d65d76573332b56fd0276838a62e4aa01";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000f00000000000000010000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_2x32oz_2x16oz_4x8oz_4x5h_64oz:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "cb1059502345fd61eeeb28f7d907f066978c703290da22c2e4225d8f9978761ea21130c43d85afdb3e0e710d5b59"
                          "1ce89d9a863bc86423e2e35cf084e35eca444fe15e0e5d2e2c84913218475cec8bd7133540885aa43b57e142b148"
                          "eba84a73099a6e51e621ba2611ecf48950278fc975e1a5221a6b3082569045bb4e6f7533e9b1036597c7f1a43389"
                          "c692535dd3e0d77d15ab759b313c0d4e512813dd302fde6575b284a75a2fa8346c946daf5fe2b96790f31fb47d17"
                          "e58dffe438c94ffefb28afae78d97e628bd23cbc4bdf1ac80d2fa913a6f8c549e4b289e64b1bd8d96ff4e09991b5"
                          "e10d3a3dbdc75eef727abf0d637c514ffdb1c6914d7f1af15de1";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000000000000055550000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_2x32oz_2x16oz_4x8oz_2x0fh_64oz:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "a358fbbe8e47cac473ef6ae5c261c72bce3d6fc6a879373a243a0b1958a8a14e0d9cd15784066c0251d16ce84cea"
                          "762471a63ec1d21f041c3774fba20564a7fbbaeef861e6572f98b7d12976a8168b08bd1736a3f679bf78ac79202b"
                          "83820b5ee4bd4ef600c4f289f7ba8a35749e395dfe76349e31579bc8f4a2527dd163543017c8d87df7def319152e"
                          "4f7b0cdef913513e207281c82c9eaf4734560b25a8461666742cd1492445a764b6c3ee02d0cf8c46b2b47be3c623"
                          "f212c2052acd9676ef6977e7fedd233102769eb844131c88fb8d4737346646504f74a37c94de29334f1fa4e64f22"
                          "2b6c16886fe18a41ca41baa736897896eedfbc3dc844559fee81";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff0000000000000000000f0f0f0f0000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_2x32oz_2x16oz_4x8oz_4x33h_64oz:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "299a2b30c08ba05f8a631228c28297bf716a1c0e586c550a6168e54e7bfd25bf30b52c16defcc93b8c45132990bb"
                          "2d16bc932e270fcee5d38dd31df95100235e7950c139299c3cf3d861efa985f3b9e9ac69caed7428b0a5280099b8"
                          "003c3e7842d51dfbe9de16513e89cd7bb438e7b9df6bd0b6c8585c0a9f8b97ff48c9ae406fca0eb4db74e4be6525"
                          "2915575b667c05ad51fc0eaada37fa41e4bce4ef50935d4dc2b1751cbd3f7d0eb62f00f265bda4b023ef2c01b1c0"
                          "a03182503b864b41ec8f315ff98ddf9a2fd6178c4624e8bdfdc7ca9240d71d8593d8f5bd7f1f4154a25217bd31d3"
                          "e0262094daf44e41dc50577d808eaf3a8ee86391bf717c720ce1";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000000000333333330000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key1_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "2059eb2c73ceace6684cb9de01e059a95ce33c44972440e62405aa023282a2d24321442e67fa7be479dd69409d5b"
                          "060de0f6aad31d4b9b374371165a9de9f18eded92d84b8f41a554ca7340a91927c65ab0de773ff91de4bb7c51321"
                          "5315c0c346a933bf6740c67672c4a6569f5b499e4359eaba4832e91f5caa39a0aa645a5a407d312505491f3da237"
                          "8f6aae414b312f9a653fd15cb81261a0f5a8b401dddf97ed8ffdc2b7679c9c8359fbaa298aab8874160b86415646"
                          "75cb8a934977626d45c09855f99317c59eeba5681083e9f1d76282771e70de6455749f8023142fed89904a215b32"
                          "9ce216f5136def4674ee45626ea8a01ae1fa5408042faa4e7361";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000003333000055550000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key2_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz:
            data->p_str = p_str_2;
            data->q_str = q_str_2;
            data->e_str =
                "a8dc9c80a481558cbf68c4f98b4b21bc1326cb05e6268c314e3fc4dd2cba8dc0f2e785a839369e801f7c70c283e3394218a5da"
                "d81176d75fa843f4495c8b44fdb1f4824ecaaaf40d5cfde1a4e9279bff9e7feb009bf517fad1f8c5f36e567091b31b2abd1530"
                "dc8cce6066dfddb062b523cf66121933d0a6555bb3023a0bc83d4ad2d94fd915306ad1658d2b170dabfba300521ee79369c748"
                "49816b0a72df47e99d65f88d59c84ddb3749434dc043bc90018383ed102abd90e484d6cc80809b13ae1fa6587b05d488965c00"
                "2f7fcaedcfac1319fc88779b49b27d0f2d3b4f07a52f69ee11bc1b3c8ce6663c8b439ed2429ae4fd3f104cfcbb235fb4733e83"
                "7da8f4ee161ee690729d375495b2fb9044290d03e869e23ef1babeef2541eef2668e08c822fff34926cb20bcbe07bfbefa1a3e"
                "9c3759b80ff28d8730dd817260490a36ca1b10ba16986fac94a1cb179ea41325709abebff957a2632ee59e33b039e331250510"
                "4ec0a886b1d63565ce2939b09f53b43d52b8f47c2505cc68c674e8b5b3ef01f3186b3f9eb22de1c1811bde8d5c61310165e689"
                "1452ee5f2b00d03d5d3f97db6ad000e21e497067dcaa3deb5896afa8dd5bae8254738c3fddfb1e7881deacc9b3c92ef7a58d59"
                "9ac16f47db7570d733d81b3846975b59a05a2130ddd16bbf0abbc1c5ce21e746a7a7c17048359efe570e36de4fea094463a6b1"
                "06ab";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000003333000055550000"
                          "0000ffffffffffffffff0000000000000003";
            break;

        case Key3_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz:
            data->p_str = p_str_3;
            data->q_str = q_str_3;
            data->e_str =
                "22cf19bc826ae2a7041f473d5304c2200ea7401d29fd583ff99f9200d69744b585a18036bb09c4c2bca338931a47d96aa9c083"
                "e4b189e10be6a9f1c1d3f0d0b2496da05c88df3955546bf5fe30f76d678cea7383ed494c0b03aae7fb05afcc32c8bd8c37dbd7"
                "0259d40c55cd1f4e5852dd819b86db38b02da24d62f1a5e1dd8f824674c75f29ae684c1e244b215577c5fdd4b56223fe98178b"
                "5fa2761d47a4dde6bbad53e1b30ef893380b5673b16459c7f48c13fcb4a1543cfa8919487664956df0b0767069fa914830cd3b"
                "51b70e7a4886b9ff2411389b7609f644bc5cae81d9bb27b12a70048e817f34a68650de47d1962c1070ca446ef290a7c339c600"
                "93d0f4d1a87cba7e963e21ff40271d7b8ba1000a6b5deb36c41b5383d321178ce1e0d305c2943ac9cdddab1819bbe17a9c7381"
                "8dd50007776186ce9240ad4fb43493fd8bb84044cfe9e4a498e364b5a711eb49bbe9e1c4072eef1ab3c54df9f13b565aaa5713"
                "b371e0f6fe2b0bd4541d9adf6359327d9bbcac4644665b05c444973b577dc41263f9cdef12ed08cb122da83c0ad51da1db0d0c"
                "f85f7ef82e6da7ec853f67f4a396277592ab839d222e70b95d0e340d5a8f23ed13da494039934c35c5700575461f5cd36b14b2"
                "5556df83529fcdc6c3dd6b386e14f293120b019d9b9a3f151bca5a7ee17f9583be9f758e6c3dbff28d3cf12754594630f55b80"
                "60ace63c7493799e6d7083c29fd3dad89e15cfdff7d32f2fc8f4791a26e262ae8b78e80bc43f8c876e7f608e35258a7a23593b"
                "804c9a08a9b5339d6d78f57964bad021b03b9c21a3427de9f6d334588e99f50b684ca11e56f1263e98eb931f5feb71589e877a"
                "72d67e606ccf5337d9b3c974eec4967545cd2619043cb505bbd40ae32c8a2d5d9f3abf3d4b7284ad934cb96dd33368cd962ec7"
                "b2592a18e1bffacda80a8c7082841b8c1300740a4040db9029be13802081ee17fb775883a1c9a34e909b4bcc270e126831407f"
                "8f0c9965687b9fe79b0322d2cf446f967f77c945704f3c7f53f53cb67f75f90bf75708daffb42b04899c50363542881a4a8583"
                "3293fd7e93f8e47e92806dba2e4dcaf72accbf25b099b61f414c2ae965fe07abee142336d3b753480739de7d58abc8eff30e09"
                "e49d0ed3f14c27a66dbdf098fb0ee07cd8bb08f285e7fca1021a51fe817bba1ceeb1d6a793038a46ba09984c2011f1e8ca6b25"
                "8f0d9604d4c5f273a086ea5454ca1adc464f6e0f16ac176c7a9065a0ed35400546c217e967c701d60f1022f2992f4354cd7bdb"
                "d1405b00838ef9152c267c43e7b5b370262aef30c02697cbb1fc3a99727758712008dcbff637e48107ad20537729c9a804f229"
                "6d025a50eaaf207500b36632a32cca65ca8946649ab6075c8f2d962a432bb155880d3b40a03a1cf9b2ca09eb45612b62afca3b"
                "f923458b";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000003333000055550000"
                          "0000ffffffffffffffff0000000000000003";
            break;

        case Key0_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz:
            data->p_str = p_str_0;
            data->q_str = q_str_0;
            data->e_str = "a04a00324454507596a9c47cf1f1775485988f56a89f9bb0249771d3c2bf4a029b24bc3718091fc9e20efc627f22"
                          "8e7c4f08f19d0e987f747cf2615b5800751b1170d88f06b4de8de74a4df27c8b9214a70533a1f7a7995cfc926078"
                          "01e74a46d709d0f1eb3b7c0b3257f9843989fb22cd765d7fdb8fd3c52045bd4c609f0af1";
            data->d_str = "ffffffff00000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000003333000055550000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Keym1_2x32oz_2x16oz_4x8oz_4x3h_4x5h_64oz:
            data->p_str = p_str_m1;
            data->q_str = q_str_m1;
            data->e_str = "18b9c316938d862299bb5307100f2d4e78234af2600328eee1cd19a62cc506340ec468fc33ac76467cf3a15b2ec5"
                          "bc77ab64b9cb696ce07404e2d3814f1d2cf7";
            data->d_str = "7000000000000000ffffffff00000000ffff0000ffff0000ff00ff00ff00ff000000000000003333000055550000"
                          "0000ffffffffffffffff0000000000000007";
            break;

        case Keym1_2x32oz_2x16oz_192z_64oz:
            data->p_str = p_str_m1;
            data->q_str = q_str_m1;
            data->e_str = "11df0ff03d2188255c7371ef57916830adb4c7b35dc4f8b666ffc6405a9e13d2717146b5790845056f6af5f97477"
                          "d4d2a3e3c764f3c2293a104af61d4dcdcbc1";
            data->d_str = "7000000000000000ffffffff00000000000000000000000000ffff0000ffff000000000000000000000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_rising_mulitple:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "a410164f84d09282bf9d7a40dbbf811457905007818e444c6b5fdf76a3ba163a7075ade5ec24ba7b2798bd269b3a"
                          "eab40ba74a49e7d5c0c221805450d49ae96d3318070052e905a77f98c0fa1094fe0bbf30e95d32a9db1b9c75de48"
                          "e64b04ab684ddbef364ba89ea6d14b5c333314ba5239f93b23013d173726dd6cdadcf7a047ac95102b9311514984"
                          "f62727e8588a114b1f1e2346f226dad0e62eda55c42773ed84cda3c2d8a2fee614c3067e0b5078f7f83f0cb82fc5"
                          "803311bf765454d481e8b1bd9560f81a8ec17ae44898e70babf80ba39b285bb8f63c39233941d11df1d501e8eb08"
                          "289c7c655bf24ce7e7de15b4701b2f11c2b1d2d547805f8270e1";
            data->d_str = "ffffffff0000000011112222444488883333555566669999aaaacccc7777bbbbddddeeeeffffffff000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_rising:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "85bcab3ea5c533ff9e014d5d4b093e895a164f276d3b74ec1de558d41a0afb27d97fb930410cc0bbc7c17df4f312"
                          "9b757d6e7fb506497e30acdaafcab572a8cdd411d6bc8b21f9c28b520f4b6132c60baf83b9eebc58a5c9a5b5ad9a"
                          "f4c75d076334e4bcc27c3692e92e55bdc71cc0f38e562d92d095fb628252910164697d4426a8155f529c40089699"
                          "f574431c78a395037b47d9c4a9b5bf5f7078ef662b46e419b49075de2d3530bacd0735253178ddcbdd9ba5fac471"
                          "4e508b1d2cafcdd2dc5e76bc2410b166dfcaf0495723bad83c31ef729f03ff67a4471a65a8d1e2318a6b351062b1"
                          "e2e606aa2ae1c53663121c3a66a2173f7b0960db4e33b0fcd141";
            data->d_str = "ffffffff00000000111111111111111133333333333333337777777777777777ffffffffffffffff000000000000"
                          "0000ffffffffffffffff0000000000000001";
            break;

        case Key_short:
            data->p_str = p_str_m1;
            data->q_str = q_str_m1;
            data->e_str = "23c5fea62113513ec7146b16250531535e2c8b33d4e43c2e6645aea2bbdc16fc0fd5d5b101d647e4fdfb3d3ff879"
                          "29e600b0d43710168d65bad6c8fe3d61aefb";
            data->d_str = "ff00ff00f0f0555533";
            break;

        case Key_short2:
            data->p_str = p_str_m1;
            data->q_str = q_str_m1;
            data->e_str = "1aaf0dbe90971e871f796ef39948c8338d42bec18c0f771167eae9573886ec29d630d9592219b9bca7eaa15b25ea"
                          "d2c9f882a21d7fa603f3df9c1dc60638d1c7";
            data->d_str = "660755fff0f0555537";
            break;

        case Key_even_shorter:
            data->p_str = p_str_m2;
            data->q_str = q_str_m2;
            data->e_str = "2e71496eca055240171077e0e294c085e2b4b7df869b5bd4658e2d4e45e684a1";
            data->d_str = "1321";
            break;

        case Key_bit_longer:
            data->p_str = p_str_m2;
            data->q_str = q_str_m2;
            data->e_str = "4e0bb3a5fd6cba548dd043d0f725728bebadfca2fa2ec2a3a45629e793861871";
            data->d_str = "13211321";
            break;

        case Key_bit_bit_longer:
            data->p_str = p_str_m2;
            data->q_str = q_str_m2;
            data->e_str = "2eab06d478cb7e3b4cfc667927070920f13871eb1c23f80ea336e0d48bee811b";
            data->d_str = "132113211323";
            break;
        case Key_only_leading_zeros:
            data->p_str = p_str_m2;
            data->q_str = q_str_m2;
            data->e_str = "423dd119f7ee16d7d556f12c3391029764ee6c761e438adb424f4984980b06fd";
            data->d_str = "5";
            break;

        case Key_real_512:
            data->p_str = p_str_m1;
            data->q_str = q_str_m1;
            data->e_str = "660755fff0f0555537";
            data->d_str = "1aaf0dbe90971e871f796ef39948c8338d42bec18c0f771167eae9573886ec29d630d9592219b9bca7eaa15b25ea"
                          "d2c9f882a21d7fa603f3df9c1dc60638d1c7";
            break;

        case Key_real_2048:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "2c711994d28f9e1b91f0cff25788e6e325725f579539c0c85643d747ba436ce5faf299f66bff6b493e18e50aa5ac"
                          "5fd7b81101de70456e8ce8c881b3f3e5db86f181ed5ac0ca98559e7eb5fe6a7a11ecad34a305d0ac2fe5dd12e596"
                          "134c1c33dafa02f162367d863f9377f60c947c9b28ed5c49155dd7470b8831bcb3c4602eaec601ec6ebb56a9b664"
                          "e21fd880697103d58e984d0b66547d552f3bc4f94af03601e4b817adedc0c89a77345e3840548b9b7a6a4afd7ccb"
                          "6167df431fa780381329c4d786768483a0c6e8386b493791fa8c756fd030be353bac9008ea9691f728378bee7dd9"
                          "e375eb3b8cdbc918333b0e9d8f0cfc6decc78b52a256f2b7f9f";

            data->d_str = "74c14484ae5f26ffd56abadf1e4f0ad3d40a09a63d66b2d3ea1d7c7bb4a7334565ba106565ae5eea780c53c4e3e2"
                          "8a943acda81ac8b3f3672ad10ce8511693aa12c0fb735fdbd0ebda3918088580cd479545b996c439b73d714c76e3"
                          "f3803745dcdbe09e86e4bd9b2b49bd65846ea63524d488679a033fa7391322821140d39b555c0db20db3a2075d68"
                          "7d1f22efadbd48ff762f104564822fcedee390214fcaf6010f8753d95883da02ab52d5ac6477454b62789f44da18"
                          "e11d0a5b5c0afc7207f7bdffb999d2695ab104e351e36920730507b15f870d21cc6ab29e4f1649e32ae260f2adf3"
                          "4e503930827ef765e68f4a5e4773e4a4b4f742de5031a027cb3f";
            break;

        case Key_short_2048:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->e_str = "65537";

            data->d_str = "a49f453bb251badce286113d36cf8652dad9806c6fbac7480ca4b8be3a7165aa752a67b8efb4019e64015e5210d7"
                          "02bbab0df48627c9f809c62f97b21c6ecc507617923bdb2c45c2b95db29421daf85dad0ee07cb2eb22565a8d4f48"
                          "a2d07a56c2696439f05020969561cf04fa444c2e3fbe2540bc76450cfe56cbf0c00da5293f9327a42bf697bb3ba5"
                          "fdeceded8ce8bc217aefd29705a244aff5ced0cdc143be68b5e2b538ef070de6793ea0d2ce35a2ef2c19b5e7e92b"
                          "05c3dfa44105161c92b1a01e88b6caa82b57a4a88b146913e851a00af28ccfae9ca03ef2150d3d7ed9ee935a3c55"
                          "672fffb90533fdb8e920d4552baed7bac2da4cc6d769905f75c7";
            break;

        case Key_short_2048_inv:
            data->p_str = p_str_1;
            data->q_str = q_str_1;
            data->d_str = "65537";

            data->e_str = "a49f453bb251badce286113d36cf8652dad9806c6fbac7480ca4b8be3a7165aa752a67b8efb4019e64015e5210d7"
                          "02bbab0df48627c9f809c62f97b21c6ecc507617923bdb2c45c2b95db29421daf85dad0ee07cb2eb22565a8d4f48"
                          "a2d07a56c2696439f05020969561cf04fa444c2e3fbe2540bc76450cfe56cbf0c00da5293f9327a42bf697bb3ba5"
                          "fdeceded8ce8bc217aefd29705a244aff5ced0cdc143be68b5e2b538ef070de6793ea0d2ce35a2ef2c19b5e7e92b"
                          "05c3dfa44105161c92b1a01e88b6caa82b57a4a88b146913e851a00af28ccfae9ca03ef2150d3d7ed9ee935a3c55"
                          "672fffb90533fdb8e920d4552baed7bac2da4cc6d769905f75c7";
            break;
    }
}

void ocall_print_string(char const *str) {
    enclave_puts(str);
}

extern "C" {

void crypto_init(int key) {
    enclave_init(key);
}

void crypto_clear() {
    mbedtls_mpi_read_string(&tmp, 16, "0");
}

void crypto_run() {
    int err;
    if ( (err = crypto_decrypt(&rsa_ctx, cipher_raw, tmp_raw)) != 0 ) {
        char buffer[500];
        mbedtls_strerror(err, buffer, 500);
        enclave_printf("error: %s\n", buffer);
    }
}

void crypto_run_no_check() {
    int err;
    if ( (err = crypto_decrypt_no_check(&rsa_ctx, cipher_raw, tmp_raw)) != 0 ) {
        char buffer[500];
        mbedtls_strerror(err, buffer, 500);
        enclave_printf("error: %s\n", buffer);
    }
}

int crypto_is_faulted() {
    return mbedtls_mpi_cmp_mpi(&cipher, &tmp) != 0;
}
}
