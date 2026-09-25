#include <dwhbll/macros/testing.h>

import std;
import dwhbll.testing;
import dwhbll.cryptography;
import dwhbll.sanify;

using namespace dwhbll::test;

namespace crypto::arc4 {

[[=test]]
void plaintext_with_key()
{
    dwhbll::cryptography::arc4::arc4 engine({'K', 'e', 'y'});
    u8 plaintext[] = "Plaintext";
    engine.crypt_inplace(plaintext, 9);

    u8 expected[] = "\xBB\xF3\x16\xE8\xD9\x40\xAF\x0A\xD3";

    EXPECT(std::memcmp(plaintext, expected, 9) == 0);
}

[[=test]]
void attack_at_dawn_with_secret()
{
    dwhbll::cryptography::arc4::arc4 engine({'S', 'e', 'c', 'r', 'e', 't'});
    u8 plaintext[] = "Attack at dawn";
    engine.crypt_inplace(plaintext, 14);

    u8 expected[] = "\x45\xA0\x1F\x64\x5F\xC3\x5B\x38\x35\x52\x54\x4B\x9B\xF5";

    EXPECT(std::memcmp(plaintext, expected, 14) == 0);
}

}

TEST_REGISTER_FILE();
