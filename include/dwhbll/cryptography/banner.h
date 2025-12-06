#pragma once

#include <iostream>

namespace dwhbll::cryptography {
    inline void printBanner() {
        static bool banner_printed = false;

        if (!banner_printed) {
            std::cerr << "---------------------------------------------------------------------------------" << std::endl;
            std::cerr << "*** DWHBLL::CRYPTOGRAPHY IS NOT VERIFIED UNDER CMVP, DO NOT USE IN PRODUCTION ***" << std::endl;
            std::cerr << "---------------------------------------------------------------------------------" << std::endl;

            banner_printed = true;
        }
    };
}
