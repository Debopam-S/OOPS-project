#pragma once
#include "Exceptions.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <ostream>
#include <string>

// A calendar date (DD-MM-YYYY) with day arithmetic.
class Date {
    int d_ = 1, m_ = 1, y_ = 2000;

    static int daysIn(int m, int y) {
        static const int len[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
        return len[m - 1] + (m == 2 && leap ? 1 : 0);
    }
    // Days counted from a fixed starting point; only differences are used.
    long toDays() const {
        int y = m_ < 3 ? y_ - 1 : y_, m = m_ < 3 ? m_ + 12 : m_;
        return 365L * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d_;
    }

public:
    Date() = default;
    Date(int d, int m, int y) : d_(d), m_(m), y_(y) {
        if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1 || d > daysIn(m, y))
            throw InvalidInputException("no such date " + std::to_string(d) + "-" +
                                        std::to_string(m) + "-" + std::to_string(y));
    }
    static Date parse(const std::string& s) {
        int d, m, y;
        char extra;
        if (std::sscanf(s.c_str(), "%d-%d-%d%c", &d, &m, &y, &extra) != 3)
            throw InvalidInputException("date must be DD-MM-YYYY, got '" + s + "'");
        return Date(d, m, y);
    }
    // System date, or BB_TODAY (DD-MM-YYYY) so the demo can run on a fixed date.
    static Date today() {
        if (const char* fixed = std::getenv("BB_TODAY")) return parse(fixed);
        std::time_t now = std::time(nullptr);
        std::tm* t = std::localtime(&now);
        return Date(t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    }

    Date operator+(int days) const {   // date after n days
        Date r = *this;
        while (days-- > 0)
            if (++r.d_ > daysIn(r.m_, r.y_)) {
                r.d_ = 1;
                if (++r.m_ > 12) { r.m_ = 1; ++r.y_; }
            }
        return r;
    }
    int operator-(const Date& o) const { return static_cast<int>(toDays() - o.toDays()); }
    bool operator<(const Date& o) const { return toDays() < o.toDays(); }
    bool operator==(const Date& o) const { return toDays() == o.toDays(); }

    std::string str() const {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%02d-%02d-%04d", d_, m_, y_);
        return buf;
    }
    friend std::ostream& operator<<(std::ostream& os, const Date& dt) { return os << dt.str(); }
};
