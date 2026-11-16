// Unit tests for the core rules. Build and run: make test
#include "BloodBank.h"
#include "Exceptions.h"

#include <cstdlib>
#include <iostream>
#include <string>

static int passed = 0, failed = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) ++passed;                                                      \
        else { ++failed; std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; } \
    } while (0)

template <typename E, typename F>
bool throws(F f) {
    try { f(); } catch (const E&) { return true; } catch (...) { return false; }
    return false;
}

int main() {
    // Date arithmetic and validation
    CHECK(Date(28, 2, 2024) + 1 == Date(29, 2, 2024));        // leap year
    CHECK(Date(28, 2, 2026) + 1 == Date(1, 3, 2026));
    CHECK(Date(1, 10, 2026) + 42 == Date(12, 11, 2026));
    CHECK(Date(30, 12, 2026) - Date(1, 10, 2026) == 90);
    CHECK(throws<InvalidInputException>([] { Date::parse("31-02-2026"); }));
    CHECK(throws<InvalidInputException>([] { Date::parse("2026-11-16"); }));

    // Blood group compatibility: all 64 donor/recipient pairs
    const char* order[] = {"O-", "O+", "A-", "A+", "B-", "B+", "AB-", "AB+"};
    // row = donor, column = recipient (1 = safe)
    const int table[8][8] = {
        {1, 1, 1, 1, 1, 1, 1, 1}, {0, 1, 0, 1, 0, 1, 0, 1}, {0, 0, 1, 1, 0, 0, 1, 1},
        {0, 0, 0, 1, 0, 0, 0, 1}, {0, 0, 0, 0, 1, 1, 1, 1}, {0, 0, 0, 0, 0, 1, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 1}, {0, 0, 0, 0, 0, 0, 0, 1}};
    for (int d = 0; d < 8; ++d)
        for (int r = 0; r < 8; ++r)
            CHECK(BloodGroup::parse(order[d]).canDonateTo(BloodGroup::parse(order[r])) ==
                  (table[d][r] == 1));
    CHECK(BloodGroup::parse(" ab+ ").name() == "AB+");
    CHECK(throws<InvalidInputException>([] { BloodGroup::parse("C+"); }));

    // Bank rules on a fresh data folder
    std::system("rm -rf tests/tmp");
    BloodBank bank("tests/tmp", Date(16, 11, 2026));
    bank.load();
    int oneg = bank.registerDonor("Rohan Mehta", 24, "9876543210", "O-", 68, "");
    int opos = bank.registerDonor("Neha Singh", 22, "9001122334", "O+", 52, "");
    int apos = bank.registerDonor("Priya Sharma", 31, "9812345678", "A+", 55, "20-07-2026");
    int pat = bank.registerRecipient("Sunita Devi", 58, "9123456780", "A+", "City Hospital");

    CHECK(throws<InvalidInputException>([&] {          // same phone twice
        bank.registerDonor("Copy", 30, "9876543210", "B+", 60, "");
    }));
    CHECK(throws<InvalidInputException>([&] {          // under 50 kg
        bank.registerDonor("Light", 30, "9000000002", "B+", 45, "");
    }));
    CHECK(throws<InvalidInputException>([&] {          // 82 days after last donation
        bank.recordDonation(apos, "10-10-2026");
    }));
    CHECK(throws<RecordNotFoundException>([&] { bank.recordDonation(pat, ""); }));  // not a donor

    bank.recordDonation(oneg, "08-10-2026");   // expires 19-11
    bank.recordDonation(opos, "");             // today
    bank.recordDonation(apos, "18-10-2026");   // exactly 90 days: allowed
    CHECK(bank.stockReport()["O-"].expiringSoon == 1);

    const Request& r = bank.requestBlood(pat, 2);
    CHECK(r.status() == "Fulfilled");
    CHECK(r.unitIds().size() == 2 && r.unitIds()[0] == 3 && r.unitIds()[1] == 2);  // A+, then O+
    CHECK(bank.stockReport()["O-"].available == 1);                                // O- kept

    CHECK(throws<ShortageException>([&] { bank.requestBlood(pat, 2); }));
    CHECK(bank.requests().back().status() == "Shortage");

    // Persistence: a second object reads the same files
    BloodBank again("tests/tmp", Date(20, 11, 2026));
    again.load();
    CHECK(again.requests().size() == 2);
    CHECK(again.search("sharma").size() == 1);
    CHECK(again.removeExpired() == 1);        // O- unit expired on 19-11
    CHECK(again.registerDonor("New Donor", 40, "9000000003", "AB+", 70, "") == 5);  // IDs continue

    std::cout << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
