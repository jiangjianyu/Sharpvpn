//
// Created by maxxie on 16-7-19.
//

#include <string>
#include <memory.h>
#include "SharpCrypto.h"
#include "Sharp.h"

SharpCrypto::SharpCrypto(std::string encryption_key) {
    key = encryption_key;
    if (encryption_key.length() > SHARPVPN_CRYPTO_KEYBYTES) {
        key = encryption_key.substr(0, SHARPVPN_CRYPTO_KEYBYTES);
    }
    memset(key_buf, 0,SHARPVPN_CRYPTO_KEYBYTES);
    memcpy(key_buf, key.c_str(), key.length());
}

unsigned long long SharpCrypto::encrypt(unsigned char *message, unsigned char *ciphertext, unsigned long long mlen) {
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
    crypto_secretbox_easy(ciphertext + crypto_secretbox_NONCEBYTES, message, mlen, nonce, key_buf);
    memcpy(ciphertext, nonce, crypto_secretbox_NONCEBYTES);
    return mlen + SHARPVPN_CRYPTO_OVERHEAD;
}

unsigned long long SharpCrypto::decrypt(unsigned char *message, unsigned char *ciphertext, unsigned long long clen) {
    if (crypto_secretbox_open_easy(message, ciphertext + crypto_secretbox_NONCEBYTES, clen- crypto_secretbox_NONCEBYTES, ciphertext, key_buf) != 0) {
        LOG(ERROR) << "decrypting message fail";
        return 0;
    }
    else
        return clen - SHARPVPN_CRYPTO_OVERHEAD;
}