#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <tuple>
#include <vector>

#include <cassert>
#include <numeric>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <utility>

#ifdef _MSC_VER
#endif

namespace atcoder {

    namespace internal {

        // @param m `1 <= m`
        // @return x mod m
        constexpr long long safe_mod(long long x, long long m) {
            x %= m;
            if (x < 0) x += m;
            return x;
        }

        // Fast modular multiplication by barrett reduction
        // Reference: https://en.wikipedia.org/wiki/Barrett_reduction
        // NOTE: reconsider after Ice Lake
        struct barrett {
            unsigned int _m;
            unsigned long long im;

            // @param m `1 <= m < 2^31`
            explicit barrett(unsigned int m) : _m(m), im((unsigned long long)(-1) / m + 1) {}

            // @return m
            unsigned int umod() const { return _m; }

            // @param a `0 <= a < m`
            // @param b `0 <= b < m`
            // @return `a * b % m`
            unsigned int mul(unsigned int a, unsigned int b) const {
                // [1] m = 1
                // a = b = im = 0, so okay

                // [2] m >= 2
                // im = ceil(2^64 / m)
                // -> im * m = 2^64 + r (0 <= r < m)
                // let z = a*b = c*m + d (0 <= c, d < m)
                // a*b * im = (c*m + d) * im = c*(im*m) + d*im = c*2^64 + c*r + d*im
                // c*r + d*im < m * m + m * im < m * m + 2^64 + m <= 2^64 + m * (m + 1) < 2^64 * 2
                // ((ab * im) >> 64) == c or c + 1
                unsigned long long z = a;
                z *= b;
#ifdef _MSC_VER
                unsigned long long x;
                _umul128(z, im, &x);
#else
                unsigned long long x =
                    (unsigned long long)(((unsigned __int128) (z) *im) >> 64);
#endif
                unsigned int v = (unsigned int) (z - x * _m);
                if (_m <= v) v += _m;
                return v;
            }
        };

        // @param n `0 <= n`
        // @param m `1 <= m`
        // @return `(x ** n) % m`
        constexpr long long pow_mod_constexpr(long long x, long long n, int m) {
            if (m == 1) return 0;
            unsigned int _m = (unsigned int) (m);
            unsigned long long r = 1;
            unsigned long long y = safe_mod(x, m);
            while (n) {
                if (n & 1) r = (r * y) % _m;
                y = (y * y) % _m;
                n >>= 1;
            }
            return r;
        }

        // Reference:
        // M. Forisek and J. Jancina,
        // Fast Primality Testing for Integers That Fit into a Machine Word
        // @param n `0 <= n`
        constexpr bool is_prime_constexpr(int n) {
            if (n <= 1) return false;
            if (n == 2 || n == 7 || n == 61) return true;
            if (n % 2 == 0) return false;
            long long d = n - 1;
            while (d % 2 == 0) d /= 2;
            constexpr long long bases[3] = { 2, 7, 61 };
            for (long long a : bases) {
                long long t = d;
                long long y = pow_mod_constexpr(a, t, n);
                while (t != n - 1 && y != 1 && y != n - 1) {
                    y = y * y % n;
                    t <<= 1;
                }
                if (y != n - 1 && t % 2 == 0) {
                    return false;
                }
            }
            return true;
        }
        template <int n> constexpr bool is_prime = is_prime_constexpr(n);

        // @param b `1 <= b`
        // @return pair(g, x) s.t. g = gcd(a, b), xa = g (mod b), 0 <= x < b/g
        constexpr std::pair<long long, long long> inv_gcd(long long a, long long b) {
            a = safe_mod(a, b);
            if (a == 0) return { b, 0 };

            // Contracts:
            // [1] s - m0 * a = 0 (mod b)
            // [2] t - m1 * a = 0 (mod b)
            // [3] s * |m1| + t * |m0| <= b
            long long s = b, t = a;
            long long m0 = 0, m1 = 1;

            while (t) {
                long long u = s / t;
                s -= t * u;
                m0 -= m1 * u;  // |m1 * u| <= |m1| * s <= b

                // [3]:
                // (s - t * u) * |m1| + t * |m0 - m1 * u|
                // <= s * |m1| - t * u * |m1| + t * (|m0| + |m1| * u)
                // = s * |m1| + t * |m0| <= b

                auto tmp = s;
                s = t;
                t = tmp;
                tmp = m0;
                m0 = m1;
                m1 = tmp;
            }
            // by [3]: |m0| <= b/g
            // by g != b: |m0| < b/g
            if (m0 < 0) m0 += b / s;
            return { s, m0 };
        }

        // Compile time primitive root
        // @param m must be prime
        // @return primitive root (and minimum in now)
        constexpr int primitive_root_constexpr(int m) {
            if (m == 2) return 1;
            if (m == 167772161) return 3;
            if (m == 469762049) return 3;
            if (m == 754974721) return 11;
            if (m == 998244353) return 3;
            int divs[20] = {};
            divs[0] = 2;
            int cnt = 1;
            int x = (m - 1) / 2;
            while (x % 2 == 0) x /= 2;
            for (int i = 3; (long long) (i) *i <= x; i += 2) {
                if (x % i == 0) {
                    divs[cnt++] = i;
                    while (x % i == 0) {
                        x /= i;
                    }
                }
            }
            if (x > 1) {
                divs[cnt++] = x;
            }
            for (int g = 2;; g++) {
                bool ok = true;
                for (int i = 0; i < cnt; i++) {
                    if (pow_mod_constexpr(g, (m - 1) / divs[i], m) == 1) {
                        ok = false;
                        break;
                    }
                }
                if (ok) return g;
            }
        }
        template <int m> constexpr int primitive_root = primitive_root_constexpr(m);

        // @param n `n < 2^32`
        // @param m `1 <= m < 2^32`
        // @return sum_{i=0}^{n-1} floor((ai + b) / m) (mod 2^64)
        unsigned long long floor_sum_unsigned(unsigned long long n,
            unsigned long long m,
            unsigned long long a,
            unsigned long long b) {
            unsigned long long ans = 0;
            while (true) {
                if (a >= m) {
                    ans += n * (n - 1) / 2 * (a / m);
                    a %= m;
                }
                if (b >= m) {
                    ans += n * (b / m);
                    b %= m;
                }

                unsigned long long y_max = a * n + b;
                if (y_max < m) break;
                // y_max < m * (n + 1)
                // floor(y_max / m) <= n
                n = (unsigned long long)(y_max / m);
                b = (unsigned long long)(y_max % m);
                std::swap(m, a);
            }
            return ans;
        }

    }  // namespace internal

}  // namespace atcoder

namespace atcoder {

    namespace internal {

#ifndef _MSC_VER
        template <class T>
        using is_signed_int128 =
            typename std::conditional<std::is_same<T, __int128_t>::value ||
            std::is_same<T, __int128>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using is_unsigned_int128 =
            typename std::conditional<std::is_same<T, __uint128_t>::value ||
            std::is_same<T, unsigned __int128>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using make_unsigned_int128 =
            typename std::conditional<std::is_same<T, __int128_t>::value,
            __uint128_t,
            unsigned __int128>;

        template <class T>
        using is_integral = typename std::conditional<std::is_integral<T>::value ||
            is_signed_int128<T>::value ||
            is_unsigned_int128<T>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using is_signed_int = typename std::conditional<(is_integral<T>::value&&
            std::is_signed<T>::value) ||
            is_signed_int128<T>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using is_unsigned_int =
            typename std::conditional<(is_integral<T>::value&&
                std::is_unsigned<T>::value) ||
            is_unsigned_int128<T>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using to_unsigned = typename std::conditional<
            is_signed_int128<T>::value,
            make_unsigned_int128<T>,
            typename std::conditional<std::is_signed<T>::value,
            std::make_unsigned<T>,
            std::common_type<T>>::type>::type;

#else

        template <class T> using is_integral = typename std::is_integral<T>;

        template <class T>
        using is_signed_int =
            typename std::conditional<is_integral<T>::value&& std::is_signed<T>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using is_unsigned_int =
            typename std::conditional<is_integral<T>::value&&
            std::is_unsigned<T>::value,
            std::true_type,
            std::false_type>::type;

        template <class T>
        using to_unsigned = typename std::conditional<is_signed_int<T>::value,
            std::make_unsigned<T>,
            std::common_type<T>>::type;

#endif

        template <class T>
        using is_signed_int_t = std::enable_if_t<is_signed_int<T>::value>;

        template <class T>
        using is_unsigned_int_t = std::enable_if_t<is_unsigned_int<T>::value>;

        template <class T> using to_unsigned_t = typename to_unsigned<T>::type;

    }  // namespace internal

}  // namespace atcoder

namespace atcoder {

    namespace internal {

        struct modint_base {};
        struct static_modint_base : modint_base {};

        template <class T> using is_modint = std::is_base_of<modint_base, T>;
        template <class T> using is_modint_t = std::enable_if_t<is_modint<T>::value>;

    }  // namespace internal

    template <int m, std::enable_if_t<(1 <= m)>* = nullptr>
    struct static_modint : internal::static_modint_base {
        using mint = static_modint;

    public:
        static constexpr int mod() { return m; }
        static mint raw(int v) {
            mint x;
            x._v = v;
            return x;
        }

        static_modint() : _v(0) {}
        template <class T, internal::is_signed_int_t<T>* = nullptr>
        static_modint(T v) {
            long long x = (long long) (v % (long long) (umod()));
            if (x < 0) x += umod();
            _v = (unsigned int) (x);
        }
        template <class T, internal::is_unsigned_int_t<T>* = nullptr>
        static_modint(T v) {
            _v = (unsigned int) (v % umod());
        }

        unsigned int val() const { return _v; }

        mint& operator++() {
            _v++;
            if (_v == umod()) _v = 0;
            return *this;
        }
        mint& operator--() {
            if (_v == 0) _v = umod();
            _v--;
            return *this;
        }
        mint operator++(int) {
            mint result = *this;
            ++*this;
            return result;
        }
        mint operator--(int) {
            mint result = *this;
            --*this;
            return result;
        }

        mint& operator+=(const mint& rhs) {
            _v += rhs._v;
            if (_v >= umod()) _v -= umod();
            return *this;
        }
        mint& operator-=(const mint& rhs) {
            _v -= rhs._v;
            if (_v >= umod()) _v += umod();
            return *this;
        }
        mint& operator*=(const mint& rhs) {
            unsigned long long z = _v;
            z *= rhs._v;
            _v = (unsigned int) (z % umod());
            return *this;
        }
        mint& operator/=(const mint& rhs) { return *this = *this * rhs.inv(); }

        mint operator+() const { return *this; }
        mint operator-() const { return mint() - *this; }

        mint pow(long long n) const {
            assert(0 <= n);
            mint x = *this, r = 1;
            while (n) {
                if (n & 1) r *= x;
                x *= x;
                n >>= 1;
            }
            return r;
        }
        mint inv() const {
            if (prime) {
                assert(_v);
                return pow(umod() - 2);
            } else {
                auto eg = internal::inv_gcd(_v, m);
                assert(eg.first == 1);
                return eg.second;
            }
        }

        friend mint operator+(const mint& lhs, const mint& rhs) {
            return mint(lhs) += rhs;
        }
        friend mint operator-(const mint& lhs, const mint& rhs) {
            return mint(lhs) -= rhs;
        }
        friend mint operator*(const mint& lhs, const mint& rhs) {
            return mint(lhs) *= rhs;
        }
        friend mint operator/(const mint& lhs, const mint& rhs) {
            return mint(lhs) /= rhs;
        }
        friend bool operator==(const mint& lhs, const mint& rhs) {
            return lhs._v == rhs._v;
        }
        friend bool operator!=(const mint& lhs, const mint& rhs) {
            return lhs._v != rhs._v;
        }

    private:
        unsigned int _v;
        static constexpr unsigned int umod() { return m; }
        static constexpr bool prime = internal::is_prime<m>;
    };

    template <int id> struct dynamic_modint : internal::modint_base {
        using mint = dynamic_modint;

    public:
        static int mod() { return (int) (bt.umod()); }
        static void set_mod(int m) {
            assert(1 <= m);
            bt = internal::barrett(m);
        }
        static mint raw(int v) {
            mint x;
            x._v = v;
            return x;
        }

        dynamic_modint() : _v(0) {}
        template <class T, internal::is_signed_int_t<T>* = nullptr>
        dynamic_modint(T v) {
            long long x = (long long) (v % (long long) (mod()));
            if (x < 0) x += mod();
            _v = (unsigned int) (x);
        }
        template <class T, internal::is_unsigned_int_t<T>* = nullptr>
        dynamic_modint(T v) {
            _v = (unsigned int) (v % mod());
        }

        unsigned int val() const { return _v; }

        mint& operator++() {
            _v++;
            if (_v == umod()) _v = 0;
            return *this;
        }
        mint& operator--() {
            if (_v == 0) _v = umod();
            _v--;
            return *this;
        }
        mint operator++(int) {
            mint result = *this;
            ++*this;
            return result;
        }
        mint operator--(int) {
            mint result = *this;
            --*this;
            return result;
        }

        mint& operator+=(const mint& rhs) {
            _v += rhs._v;
            if (_v >= umod()) _v -= umod();
            return *this;
        }
        mint& operator-=(const mint& rhs) {
            _v += mod() - rhs._v;
            if (_v >= umod()) _v -= umod();
            return *this;
        }
        mint& operator*=(const mint& rhs) {
            _v = bt.mul(_v, rhs._v);
            return *this;
        }
        mint& operator/=(const mint& rhs) { return *this = *this * rhs.inv(); }

        mint operator+() const { return *this; }
        mint operator-() const { return mint() - *this; }

        mint pow(long long n) const {
            assert(0 <= n);
            mint x = *this, r = 1;
            while (n) {
                if (n & 1) r *= x;
                x *= x;
                n >>= 1;
            }
            return r;
        }
        mint inv() const {
            auto eg = internal::inv_gcd(_v, mod());
            assert(eg.first == 1);
            return eg.second;
        }

        friend mint operator+(const mint& lhs, const mint& rhs) {
            return mint(lhs) += rhs;
        }
        friend mint operator-(const mint& lhs, const mint& rhs) {
            return mint(lhs) -= rhs;
        }
        friend mint operator*(const mint& lhs, const mint& rhs) {
            return mint(lhs) *= rhs;
        }
        friend mint operator/(const mint& lhs, const mint& rhs) {
            return mint(lhs) /= rhs;
        }
        friend bool operator==(const mint& lhs, const mint& rhs) {
            return lhs._v == rhs._v;
        }
        friend bool operator!=(const mint& lhs, const mint& rhs) {
            return lhs._v != rhs._v;
        }

    private:
        unsigned int _v;
        static internal::barrett bt;
        static unsigned int umod() { return bt.umod(); }
    };
    template <int id> internal::barrett dynamic_modint<id>::bt(998244353);

    using modint998244353 = static_modint<998244353>;
    using modint1000000007 = static_modint<1000000007>;
    using modint = dynamic_modint<-1>;

    namespace internal {

        template <class T>
        using is_static_modint = std::is_base_of<internal::static_modint_base, T>;

        template <class T>
        using is_static_modint_t = std::enable_if_t<is_static_modint<T>::value>;

        template <class> struct is_dynamic_modint : public std::false_type {};
        template <int id>
        struct is_dynamic_modint<dynamic_modint<id>> : public std::true_type {};

        template <class T>
        using is_dynamic_modint_t = std::enable_if_t<is_dynamic_modint<T>::value>;

    }  // namespace internal

}  // namespace atcoder

#include <algorithm>
#include <array>
#include <vector>

#ifdef _MSC_VER
#endif

namespace atcoder {

    namespace internal {

        // @param n `0 <= n`
        // @return minimum non-negative `x` s.t. `n <= 2**x`
        int ceil_pow2(int n) {
            int x = 0;
            while ((1U << x) < (unsigned int) (n)) x++;
            return x;
        }

        // @param n `1 <= n`
        // @return minimum non-negative `x` s.t. `(n & (1 << x)) != 0`
        constexpr int bsf_constexpr(unsigned int n) {
            int x = 0;
            while (!(n & (1 << x))) x++;
            return x;
        }

        // @param n `1 <= n`
        // @return minimum non-negative `x` s.t. `(n & (1 << x)) != 0`
        int bsf(unsigned int n) {
#ifdef _MSC_VER
            unsigned long index;
            _BitScanForward(&index, n);
            return index;
#else
            return __builtin_ctz(n);
#endif
        }

    }  // namespace internal

}  // namespace atcoder

namespace atcoder {

    namespace internal {

        template <class mint,
            int g = internal::primitive_root<mint::mod()>,
            internal::is_static_modint_t<mint>* = nullptr>
        struct fft_info {
            static constexpr int rank2 = bsf_constexpr(mint::mod() - 1);
            std::array<mint, rank2 + 1> root;   // root[i]^(2^i) == 1
            std::array<mint, rank2 + 1> iroot;  // root[i] * iroot[i] == 1

            std::array<mint, std::max(0, rank2 - 2 + 1)> rate2;
            std::array<mint, std::max(0, rank2 - 2 + 1)> irate2;

            std::array<mint, std::max(0, rank2 - 3 + 1)> rate3;
            std::array<mint, std::max(0, rank2 - 3 + 1)> irate3;

            fft_info() {
                root[rank2] = mint(g).pow((mint::mod() - 1) >> rank2);
                iroot[rank2] = root[rank2].inv();
                for (int i = rank2 - 1; i >= 0; i--) {
                    root[i] = root[i + 1] * root[i + 1];
                    iroot[i] = iroot[i + 1] * iroot[i + 1];
                }

                {
                    mint prod = 1, iprod = 1;
                    for (int i = 0; i <= rank2 - 2; i++) {
                        rate2[i] = root[i + 2] * prod;
                        irate2[i] = iroot[i + 2] * iprod;
                        prod *= iroot[i + 2];
                        iprod *= root[i + 2];
                    }
                }
                {
                    mint prod = 1, iprod = 1;
                    for (int i = 0; i <= rank2 - 3; i++) {
                        rate3[i] = root[i + 3] * prod;
                        irate3[i] = iroot[i + 3] * iprod;
                        prod *= iroot[i + 3];
                        iprod *= root[i + 3];
                    }
                }
            }
        };

        template <class mint, internal::is_static_modint_t<mint>* = nullptr>
        void butterfly(std::vector<mint>& a) {
            int n = int(a.size());
            int h = internal::ceil_pow2(n);

            static const fft_info<mint> info;

            int len = 0;  // a[i, i+(n>>len), i+2*(n>>len), ..] is transformed
            while (len < h) {
                if (h - len == 1) {
                    int p = 1 << (h - len - 1);
                    mint rot = 1;
                    for (int s = 0; s < (1 << len); s++) {
                        int offset = s << (h - len);
                        for (int i = 0; i < p; i++) {
                            auto l = a[i + offset];
                            auto r = a[i + offset + p] * rot;
                            a[i + offset] = l + r;
                            a[i + offset + p] = l - r;
                        }
                        if (s + 1 != (1 << len))
                            rot *= info.rate2[bsf(~(unsigned int) (s))];
                    }
                    len++;
                } else {
                    // 4-base
                    int p = 1 << (h - len - 2);
                    mint rot = 1, imag = info.root[2];
                    for (int s = 0; s < (1 << len); s++) {
                        mint rot2 = rot * rot;
                        mint rot3 = rot2 * rot;
                        int offset = s << (h - len);
                        for (int i = 0; i < p; i++) {
                            auto mod2 = 1ULL * mint::mod() * mint::mod();
                            auto a0 = 1ULL * a[i + offset].val();
                            auto a1 = 1ULL * a[i + offset + p].val() * rot.val();
                            auto a2 = 1ULL * a[i + offset + 2 * p].val() * rot2.val();
                            auto a3 = 1ULL * a[i + offset + 3 * p].val() * rot3.val();
                            auto a1na3imag =
                                1ULL * mint(a1 + mod2 - a3).val() * imag.val();
                            auto na2 = mod2 - a2;
                            a[i + offset] = a0 + a2 + a1 + a3;
                            a[i + offset + 1 * p] = a0 + a2 + (2 * mod2 - (a1 + a3));
                            a[i + offset + 2 * p] = a0 + na2 + a1na3imag;
                            a[i + offset + 3 * p] = a0 + na2 + (mod2 - a1na3imag);
                        }
                        if (s + 1 != (1 << len))
                            rot *= info.rate3[bsf(~(unsigned int) (s))];
                    }
                    len += 2;
                }
            }
        }

        template <class mint, internal::is_static_modint_t<mint>* = nullptr>
        void butterfly_inv(std::vector<mint>& a) {
            int n = int(a.size());
            int h = internal::ceil_pow2(n);

            static const fft_info<mint> info;

            int len = h;  // a[i, i+(n>>len), i+2*(n>>len), ..] is transformed
            while (len) {
                if (len == 1) {
                    int p = 1 << (h - len);
                    mint irot = 1;
                    for (int s = 0; s < (1 << (len - 1)); s++) {
                        int offset = s << (h - len + 1);
                        for (int i = 0; i < p; i++) {
                            auto l = a[i + offset];
                            auto r = a[i + offset + p];
                            a[i + offset] = l + r;
                            a[i + offset + p] =
                                (unsigned long long)(mint::mod() + l.val() - r.val()) *
                                irot.val();
                            ;
                        }
                        if (s + 1 != (1 << (len - 1)))
                            irot *= info.irate2[bsf(~(unsigned int) (s))];
                    }
                    len--;
                } else {
                    // 4-base
                    int p = 1 << (h - len);
                    mint irot = 1, iimag = info.iroot[2];
                    for (int s = 0; s < (1 << (len - 2)); s++) {
                        mint irot2 = irot * irot;
                        mint irot3 = irot2 * irot;
                        int offset = s << (h - len + 2);
                        for (int i = 0; i < p; i++) {
                            auto a0 = 1ULL * a[i + offset + 0 * p].val();
                            auto a1 = 1ULL * a[i + offset + 1 * p].val();
                            auto a2 = 1ULL * a[i + offset + 2 * p].val();
                            auto a3 = 1ULL * a[i + offset + 3 * p].val();

                            auto a2na3iimag =
                                1ULL *
                                mint((mint::mod() + a2 - a3) * iimag.val()).val();

                            a[i + offset] = a0 + a1 + a2 + a3;
                            a[i + offset + 1 * p] =
                                (a0 + (mint::mod() - a1) + a2na3iimag) * irot.val();
                            a[i + offset + 2 * p] =
                                (a0 + a1 + (mint::mod() - a2) + (mint::mod() - a3)) *
                                irot2.val();
                            a[i + offset + 3 * p] =
                                (a0 + (mint::mod() - a1) + (mint::mod() - a2na3iimag)) *
                                irot3.val();
                        }
                        if (s + 1 != (1 << (len - 2)))
                            irot *= info.irate3[bsf(~(unsigned int) (s))];
                    }
                    len -= 2;
                }
            }
        }

        template <class mint, internal::is_static_modint_t<mint>* = nullptr>
        std::vector<mint> convolution_naive(const std::vector<mint>& a,
            const std::vector<mint>& b) {
            int n = int(a.size()), m = int(b.size());
            std::vector<mint> ans(n + m - 1);
            if (n < m) {
                for (int j = 0; j < m; j++) {
                    for (int i = 0; i < n; i++) {
                        ans[i + j] += a[i] * b[j];
                    }
                }
            } else {
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < m; j++) {
                        ans[i + j] += a[i] * b[j];
                    }
                }
            }
            return ans;
        }

        template <class mint, internal::is_static_modint_t<mint>* = nullptr>
        std::vector<mint> convolution_fft(std::vector<mint> a, std::vector<mint> b) {
            int n = int(a.size()), m = int(b.size());
            int z = 1 << internal::ceil_pow2(n + m - 1);
            a.resize(z);
            internal::butterfly(a);
            b.resize(z);
            internal::butterfly(b);
            for (int i = 0; i < z; i++) {
                a[i] *= b[i];
            }
            internal::butterfly_inv(a);
            a.resize(n + m - 1);
            mint iz = mint(z).inv();
            for (int i = 0; i < n + m - 1; i++) a[i] *= iz;
            return a;
        }

    }  // namespace internal

    template <class mint, internal::is_static_modint_t<mint>* = nullptr>
    std::vector<mint> convolution(std::vector<mint>&& a, std::vector<mint>&& b) {
        int n = int(a.size()), m = int(b.size());
        if (!n || !m) return {};
        if (std::min(n, m) <= 60) return convolution_naive(a, b);
        return internal::convolution_fft(a, b);
    }

    template <class mint, internal::is_static_modint_t<mint>* = nullptr>
    std::vector<mint> convolution(const std::vector<mint>& a,
        const std::vector<mint>& b) {
        int n = int(a.size()), m = int(b.size());
        if (!n || !m) return {};
        if (std::min(n, m) <= 60) return convolution_naive(a, b);
        return internal::convolution_fft(a, b);
    }

    template <unsigned int mod = 998244353,
        class T,
        std::enable_if_t<internal::is_integral<T>::value>* = nullptr>
    std::vector<T> convolution(const std::vector<T>& a, const std::vector<T>& b) {
        int n = int(a.size()), m = int(b.size());
        if (!n || !m) return {};

        using mint = static_modint<mod>;
        std::vector<mint> a2(n), b2(m);
        for (int i = 0; i < n; i++) {
            a2[i] = mint(a[i]);
        }
        for (int i = 0; i < m; i++) {
            b2[i] = mint(b[i]);
        }
        auto c2 = convolution(move(a2), move(b2));
        std::vector<T> c(n + m - 1);
        for (int i = 0; i < n + m - 1; i++) {
            c[i] = c2[i].val();
        }
        return c;
    }

    std::vector<long long> convolution_ll(const std::vector<long long>& a,
        const std::vector<long long>& b) {
        int n = int(a.size()), m = int(b.size());
        if (!n || !m) return {};

        static constexpr unsigned long long MOD1 = 754974721;  // 2^24
        static constexpr unsigned long long MOD2 = 167772161;  // 2^25
        static constexpr unsigned long long MOD3 = 469762049;  // 2^26
        static constexpr unsigned long long M2M3 = MOD2 * MOD3;
        static constexpr unsigned long long M1M3 = MOD1 * MOD3;
        static constexpr unsigned long long M1M2 = MOD1 * MOD2;
        static constexpr unsigned long long M1M2M3 = MOD1 * MOD2 * MOD3;

        static constexpr unsigned long long i1 =
            internal::inv_gcd(MOD2 * MOD3, MOD1).second;
        static constexpr unsigned long long i2 =
            internal::inv_gcd(MOD1 * MOD3, MOD2).second;
        static constexpr unsigned long long i3 =
            internal::inv_gcd(MOD1 * MOD2, MOD3).second;

        auto c1 = convolution<MOD1>(a, b);
        auto c2 = convolution<MOD2>(a, b);
        auto c3 = convolution<MOD3>(a, b);

        std::vector<long long> c(n + m - 1);
        for (int i = 0; i < n + m - 1; i++) {
            unsigned long long x = 0;
            x += (c1[i] * i1) % MOD1 * M2M3;
            x += (c2[i] * i2) % MOD2 * M1M3;
            x += (c3[i] * i3) % MOD3 * M1M2;
            // B = 2^63, -B <= x, r(real value) < B
            // (x, x - M, x - 2M, or x - 3M) = r (mod 2B)
            // r = c1[i] (mod MOD1)
            // focus on MOD1
            // r = x, x - M', x - 2M', x - 3M' (M' = M % 2^64) (mod 2B)
            // r = x,
            //     x - M' + (0 or 2B),
            //     x - 2M' + (0, 2B or 4B),
            //     x - 3M' + (0, 2B, 4B or 6B) (without mod!)
            // (r - x) = 0, (0)
            //           - M' + (0 or 2B), (1)
            //           -2M' + (0 or 2B or 4B), (2)
            //           -3M' + (0 or 2B or 4B or 6B) (3) (mod MOD1)
            // we checked that
            //   ((1) mod MOD1) mod 5 = 2
            //   ((2) mod MOD1) mod 5 = 3
            //   ((3) mod MOD1) mod 5 = 4
            long long diff =
                c1[i] - internal::safe_mod((long long) (x), (long long) (MOD1));
            if (diff < 0) diff += MOD1;
            static constexpr unsigned long long offset[5] = {
                0, 0, M1M2M3, 2 * M1M2M3, 3 * M1M2M3 };
            x -= offset[diff % 5];
            c[i] = x;
        }

        return c;
    }

}  // namespace atcoder

using mint = atcoder::modint998244353;

using fps = std::vector<mint>;

fps operator+(const fps& f, const fps& g) {
    const int siz_f = f.size(), siz_g = g.size();
    fps res = f;
    if (siz_f < siz_g) {
        res.resize(siz_g);
    }
    for (int i = 0; i < siz_g; ++i) {
        res[i] += g[i];
    }
    return res;
}
fps operator-(const fps& f, const fps& g) {
    const int siz_f = f.size(), siz_g = g.size();
    fps res = f;
    if (siz_f < siz_g) {
        res.resize(siz_g);
    }
    for (int i = 0; i < siz_g; ++i) {
        res[i] -= g[i];
    }
    return res;
}
fps operator*(const fps& f, const fps& g) {
    return atcoder::convolution(f, g);
}

fps fps_inv(const fps& f, int n) {
    fps res { f[0].inv() };
    for (int k = 1; k < n; k *= 2) {
        fps tmp = f * (res * res);
        tmp.resize(2 * k);
        res.resize(2 * k);
        for (int i = 0; i < 2 * k; ++i) {
            res[i] = 2 * res[i] - tmp[i];
        }
    }
    res.resize(n);
    return res;
}

// polynomial division
fps operator/(fps f, fps g) {
    while (f.size() and f.back() == 0) f.pop_back();
    while (g.size() and g.back() == 0) g.pop_back();
    const int fd = f.size() - 1, gd = g.size() - 1;
    assert(gd >= 0);
    if (fd < gd) return {};
    std::reverse(f.begin(), f.end());
    std::reverse(g.begin(), g.end());
    const int qd = fd - gd;
    f.resize(qd + 1);
    fps q = f * fps_inv(g, qd + 1);
    q.resize(qd + 1);
    std::reverse(q.begin(), q.end());
    return q;
}
std::pair<fps, fps> divmod(fps f, fps g) {
    fps q = f / g, r = f - g * q;
    while (r.size() and r.back() == 0) r.pop_back();
    assert(r.size() < g.size());
    return { q, r };
}
fps operator%(const fps& f, const fps& g) {
    return divmod(f, g).second;
}


/**
 * @brief Computes [x^{k+i}] P/Q for i=0,1,...,m-1 assuming that deg P < deg Q
 * @sa https://qiita.com/ryuhe1/items/da5acbcce4ac1911f47a
 * @sa https://qiita.com/ryuhe1/items/c18ddbb834eed724a42b
 * @sa https://maspypy.com/%E5%A4%9A%E9%A0%85%E5%BC%8F%E3%83%BB%E5%BD%A2%E5%BC%8F%E7%9A%84%E3%81%B9%E3%81%8D%E7%B4%9A%E6%95%B0%EF%BC%88%EF%BC%93%EF%BC%89%E7%B7%9A%E5%BD%A2%E6%BC%B8%E5%8C%96%E5%BC%8F%E3%81%A8%E5%BD%A2%E5%BC%8F#toc7
 * @sa https://noshi91.hatenablog.com/entry/2023/06/04/233447
 * @param p P(x)  (required that deg P < deg Q)
 * @param q Q(x)  (required that Q must be invertible and have no trailing zeros)
 * @param k first term to compute
 * @param m number of terms to compute
 * @return [x^{k+i}] P/Q for i=0,1,...,m-1
 */
std::vector<mint> consecutive_terms_of_rational(const fps& p, const fps& q, const uint64_t k, const size_t m) {
    // Q must be invertible and have no trailing zeros
    assert(q.size() and q.front() != 0 and q.back() != 0);
    // deg P < deg Q
    assert(p.size() < q.size());

    // deg Q
    const size_t d = q.size() - 1;

    // P/Q = R + x^k P_k/Q      (deg R <= k, deg P_k < d)
    // P = QR + x^k P_k
    // P_k = x^{-k} P (mod Q)

    // x^{-k} mod Q
    auto rec = [&](auto rec, uint64_t k) -> fps {
        if (k == 0) {
            return d > 0 ? fps{ 1 } : fps{};
        }
        if (k == 1) {
            //     Q = c + xQ' (c := Q(0))
            // --> x * (-Q'/c) = 1 (mod Q)
            // --> x^{-1} = (-Q'/c) (mod Q)
            const mint inv_q0 = q.front().inv();
            // x^{-1} mod Q
            fps ix(d);
            for (size_t i = 1; i <= d; ++i) {
                ix[i - 1] = -q[i] * inv_q0;
            }
            return ix;
        }

        // f := x^(-ceil(k/2)) mod Q

        // x^(-k) mod Q =
        //      f^2 mod Q  if k is even,
        //    x f^2 mod Q  if k is odd.

        fps f = rec(rec, (k + 1) / 2);

        // f^2
        fps sq_f = f * f;
        if (k & 1) {
            // multiplies x
            sq_f.insert(sq_f.begin(), 0);
        }
        return sq_f % q;
    };
    // x^{-k} (mod Q)
    fps ix_k = rec(rec, k);
    // P_k = x^{-k} P (mod Q)
    fps pk = ix_k * p % q;
    // P_k/Q
    std::vector<mint> res = pk * fps_inv(q, m);
    res.resize(m);
    return res;
}

/**
 * @brief Computes [A_k, A_{k+1}, ..., A_{k+m-1}] from the first d terms [A_0, A_1, ..., A_{d-1}] and the linear reccurrence A_i=Sum[j=0,d-1] A_{i-1-j} C_j  (i>=d)
 * @sa https://qiita.com/ryuhe1/items/da5acbcce4ac1911f47a
 * @sa https://qiita.com/ryuhe1/items/c18ddbb834eed724a42b
 * @sa https://maspypy.com/%E5%A4%9A%E9%A0%85%E5%BC%8F%E3%83%BB%E5%BD%A2%E5%BC%8F%E7%9A%84%E3%81%B9%E3%81%8D%E7%B4%9A%E6%95%B0%EF%BC%88%EF%BC%93%EF%BC%89%E7%B7%9A%E5%BD%A2%E6%BC%B8%E5%8C%96%E5%BC%8F%E3%81%A8%E5%BD%A2%E5%BC%8F#toc7
 * @sa https://noshi91.hatenablog.com/entry/2023/06/04/233447
 * @param a [A_0, A_1, ..., A_{d-1}]
 * @param c [C_0, C_1, ..., C_{d-1}]
 * @param k first term to compute
 * @param m number of terms to compute
 * @return [A_k, A_{k+1}, ..., A_{k+m-1}]
 */
std::vector<mint> consecutive_terms_of_linear_reccurrent_sequence(const std::vector<mint>& a, const std::vector<mint>& c, const uint64_t k, const size_t m) {
    /**
     * Step 1. Represents the generating function as P(x)/Q(x) + R(x) (deg P < deg Q)
     * Step 2. Computes [x^{k+i}] P(x)/Q(x) and [x^{k+i}] R(x) for i=0,1,...,m-1
     * 
     * NOTE. "example_01.in", "example_02.in" and "trailing_zeros_xx.in" is some of the cases where R(x)!=0
     */

    // -------------------- Step 1 -------------------- //

    const size_t d = c.size();
    // Q(x)
    fps q(d + 1);
    q[0] = 1;
    for (size_t i = 0; i < d; ++i) {
        q[i + 1] = -c[i];
    }
    while (q.back() == 0) {
        q.pop_back();
    }
    // P(x)+Q(x)R(x)
    fps p = a * q;
    p.resize(a.size());

    // R(x)
    fps r;
    std::tie(r, p) = divmod(p, q);

    // -------------------- Step 2 -------------------- //

    // [x^{k+i}] R(x)  for i=0,1,...,m-1
    r.erase(r.begin(), r.begin() + std::min<uint64_t>(r.size(), k));
    r.resize(m);

    // [x^{k+i}] P(x)/Q(x)  for i=0,1,...,m-1
    fps s = consecutive_terms_of_rational(p, q, k, m);

    return r + s;
}

int main() {
    int d;
    long long k;
    int m;

    ::scanf("%d %lld %d", &d, &k, &m);

    std::vector<mint> a(d), c(d);
    for (int i = 0, v; i < d; ++i) {
        ::scanf("%d", &v);
        a[i] = v;
    }
    for (int i = 0, v; i < d; ++i) {
        ::scanf("%d", &v);
        c[i] = v;
    }
    std::vector<mint> ans = consecutive_terms_of_linear_reccurrent_sequence(a, c, k, m);
    for (int i = 0; i < m; ++i) {
        ::printf("%d", ans[i].val());
        ::printf(i + 1 == m ? "\n" : " ");
    }
}

