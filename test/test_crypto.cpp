//
// Created by maxxie on 16-7-20.
//

#include "../src/SharpCrypto.h"

int main(int argc, char **args) {
    unsigned char m[1500];
    unsigned char c[1500 + SHARPVPN_CRYPTO_OVERHEAD];
    SharpCrypto *crypto_a = new SharpCrypto("MAXXIE");
    SharpCrypto *crypto_b = new SharpCrypto("MAXXIE");
    int a = crypto_a->encrypt(m, c, 1500);
    crypto_b->decrypt(m, c, a);
    return 0;
}