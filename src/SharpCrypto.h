//
// Created by maxxie on 16-7-19.
//

#ifndef SHARPVPN_SHARPCRYPTO_H
#define SHARPVPN_SHARPCRYPTO_H


#include <string>
#include <sodium.h>

#define SHARPVPN_CRYPTO_KEYBYTES crypto_secretbox_KEYBYTES
#define SHARPVPN_CRYPTO_OVERHEAD (crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES)

class SharpCrypto {
private:
    std::string key;
    unsigned char key_buf[SHARPVPN_CRYPTO_KEYBYTES];
public:
    std::string get_key() { return key; }
    SharpCrypto(std::string encryption_key);
    unsigned long long encrypt(unsigned char *message, unsigned char *ciphertext, unsigned long long mlen);
    unsigned long long decrypt(unsigned char *message, unsigned char *ciphertext, unsigned long long clen);
};


#endif //SHARPVPN_SHARPCRYPTO_H
