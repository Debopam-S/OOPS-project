#pragma once
#include "Exceptions.h"
#include <cctype>
#include <string>
#include <vector>

// One of the 8 ABO/Rh blood groups, stored as 3 bits: A = 1, B = 2, Rh = 4.
// A donor can give to a recipient only if the donor has no antigen the
// recipient lacks:  (donor & ~recipient) == 0.
class BloodGroup {
    int bits_ = 0;   // O-
    explicit BloodGroup(int bits) : bits_(bits) {}

public:
    BloodGroup() = default;
    static std::vector<BloodGroup> all() {   // O-, O+, A-, A+, B-, B+, AB-, AB+
        std::vector<BloodGroup> v;
        for (int b : {0, 4, 1, 5, 2, 6, 3, 7}) v.push_back(BloodGroup(b));
        return v;
    }
    static BloodGroup parse(const std::string& text) {   // "AB+", " o- ", ...
        std::string t;
        for (char c : text)
            if (!std::isspace(static_cast<unsigned char>(c)))
                t += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        for (const BloodGroup& g : all())
            if (g.name() == t) return g;
        throw InvalidInputException("blood group must be one of O- O+ A- A+ B- B+ AB- AB+, got '" +
                                    text + "'");
    }

    bool canDonateTo(const BloodGroup& r) const { return (bits_ & ~r.bits_) == 0; }

    // How many of the 8 groups this blood can be given to (O- = 8, AB+ = 1).
    // Lower means less flexible, so it should be used first.
    int servesCount() const {
        int n = 0;
        for (const BloodGroup& g : all()) n += canDonateTo(g) ? 1 : 0;
        return n;
    }
    std::string name() const {
        static const char* abo[] = {"O", "A", "B", "AB"};
        return std::string(abo[bits_ & 3]) + ((bits_ & 4) ? "+" : "-");
    }
    bool operator==(const BloodGroup& o) const { return bits_ == o.bits_; }
};
