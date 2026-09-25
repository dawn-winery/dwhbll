module;

#include <dwhbll/cryptography/ihash.h>
#include <dwhbll/cryptography/hmac.h>
#include <dwhbll/cryptography/hash/sha1.h>
#include <dwhbll/cryptography/arc4/arc4.h>

export module dwhbll.cryptography;

export namespace dwhbll::cryptography {
    using dwhbll::cryptography::ihash;
    using dwhbll::cryptography::hmac;
    using dwhbll::cryptography::SHA1;

    namespace arc4 {
        using dwhbll::cryptography::arc4::arc4;
    }
}
