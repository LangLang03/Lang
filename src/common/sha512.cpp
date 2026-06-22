// SHA-512 摘要实现。
// 采用与 FIPS 180-4 一致的 64 位字长算法，提供 1024 位（128 个十六进制字符）摘要输出。
// 该实现刻意保持自包含，不引入任何第三方依赖，方便编译期与运行期共用。

#include "common/sha512.h"

#include <array>
#include <cstring>
#include <vector>

namespace torture::common {
namespace {

// 80 个 64 位轮常量。
constexpr std::array<std::uint64_t, 80> kRoundConstants = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
};

constexpr std::array<std::uint64_t, 8> kInitialHash = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
};

inline std::uint64_t rotr(std::uint64_t value, unsigned int count) {
    return (value >> count) | (value << (64 - count));
}

// 对单个 1024 位（128 字节）块进行处理。
void processBlock(const std::uint8_t* block, std::array<std::uint64_t, 8>& state) {
    std::array<std::uint64_t, 80> schedule{};
    for (int i = 0; i < 16; ++i) {
        std::uint64_t word = 0;
        for (int b = 0; b < 8; ++b) {
            word = (word << 8) | static_cast<std::uint64_t>(block[i * 8 + b]);
        }
        schedule[i] = word;
    }
    for (int i = 16; i < 80; ++i) {
        const std::uint64_t s0 = rotr(schedule[i - 15], 1) ^ rotr(schedule[i - 15], 8) ^ (schedule[i - 15] >> 7);
        const std::uint64_t s1 = rotr(schedule[i - 2], 19) ^ rotr(schedule[i - 2], 61) ^ (schedule[i - 2] >> 6);
        schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
    }

    std::uint64_t a = state[0];
    std::uint64_t b = state[1];
    std::uint64_t c = state[2];
    std::uint64_t d = state[3];
    std::uint64_t e = state[4];
    std::uint64_t f = state[5];
    std::uint64_t g = state[6];
    std::uint64_t h = state[7];

    for (int i = 0; i < 80; ++i) {
        const std::uint64_t s1 = rotr(e, 14) ^ rotr(e, 18) ^ rotr(e, 41);
        const std::uint64_t ch = (e & f) ^ (~e & g);
        const std::uint64_t temp1 = h + s1 + ch + kRoundConstants[i] + schedule[i];
        const std::uint64_t s0 = rotr(a, 28) ^ rotr(a, 34) ^ rotr(a, 39);
        const std::uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint64_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

std::string toHex(const std::array<std::uint64_t, 8>& state) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(128);
    for (const auto word : state) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            const std::uint8_t byte = static_cast<std::uint8_t>((word >> shift) & 0xffULL);
            out.push_back(digits[byte >> 4]);
            out.push_back(digits[byte & 0x0fU]);
        }
    }
    return out;
}

}  // namespace

std::string sha512Hex(std::string_view text) {
    std::array<std::uint64_t, 8> state = kInitialHash;

    const auto messageLengthBits = static_cast<std::uint64_t>(text.size()) * 8ULL;
    std::vector<std::uint8_t> buffer(text.begin(), text.end());
    buffer.push_back(0x80U);

    // 填充 0 直至 buffer 长度 ≡ 112 (mod 128)。
    while ((buffer.size() % 128U) != 112U) {
        buffer.push_back(0x00U);
    }

    // 追加 128 位长度（大端），SHA-512 要求 128 位长度字段。
    for (int shift = 56; shift >= 0; shift -= 8) {
        buffer.push_back(static_cast<std::uint8_t>((messageLengthBits >> shift) & 0xffULL));
    }

    // 长度字段为 128 位时，文本长度被假设不会超过 2^64，剩余 64 位填 0。
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(0x00U);
    }

    for (std::size_t i = 0; i < buffer.size(); i += 128U) {
        processBlock(buffer.data() + i, state);
    }

    return toHex(state);
}

}  // namespace torture::common
