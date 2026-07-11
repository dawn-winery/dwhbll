#pragma once

namespace dwhbll::network::http {
    /**
     * @brief Multipurpose HTTP version enum
     *
     * Can be used as bitfield to represent
     */
    enum class HTTP_VERSION {
        HTTP_0_9 = 1 << 0,
        HTTP_1_0 = 1 << 1,
        HTTP_1_1 = 1 << 2,
        HTTP_2_0 = 1 << 3,
        HTTP_3_0 = 1 << 4,
    };
}
