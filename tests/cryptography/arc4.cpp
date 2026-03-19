#include <cstring>
#include <optional>
#include <string>
#include <dwhbll/console/Logging.h>

#include <dwhbll/cryptography/arc4/arc4.h>

bool crypto_arc4_test(std::optional<std::string> _) {
    {
        dwhbll::cryptography::arc4::arc4 engine({'K', 'e', 'y'});
        std::uint8_t plaintext[] = "Plaintext";
        engine.crypt_inplace(plaintext, 9);

        std::uint8_t expected[] = "\xBB\xF3\x16\xE8\xD9\x40\xAF\x0A\xD3";

        if (memcmp(plaintext, expected, 9) != 0) {
            dwhbll::console::info(R"([ARC4] Encryption of plaintext "Plaintext" with key "Key" did not match expected!)");
            return false;
        }
    }

    {
        dwhbll::cryptography::arc4::arc4 engine({'S', 'e', 'c', 'r', 'e', 't'});
        std::uint8_t plaintext[] = "Attack at dawn";
        engine.crypt_inplace(plaintext, 14);

        std::uint8_t expected[] = "\x45\xA0\x1F\x64\x5F\xC3\x5B\x38\x35\x52\x54\x4B\x9B\xF5";

        if (memcmp(plaintext, expected, 14) != 0) {
            dwhbll::console::info(R"([ARC4] Encryption of plaintext "Attack at dawn" with key "Secret" did not match expected!)");
            return false;
        }
    }

    return true;
}
