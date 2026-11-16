// LifeLine: Blood Bank Management System (console, C++17)
#include "BloodBank.h"
#include <iostream>

bool echoInput = false;   // print input back when it comes from a file (demo logs)

std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) throw std::ios_base::failure("end of input");
    if (echoInput) std::cout << line << '\n';
    return line;
}

// Template: read an int or a double, reject anything else.
template <typename T> T askNumber(const std::string& prompt) {
    std::string s = ask(prompt);
    std::istringstream in(s);
    T value;
    if (!(in >> value) || !(in >> std::ws).eof())
        throw InvalidInputException("expected a number, got '" + s + "'");
    return value;
}

void printCallList(const BloodBank& bank, const std::string& group) {
    auto list = bank.callList(group);
    std::cout << "Donors who can give to " << group << " today: " << list.size() << '\n';
    for (const Donor* d : list) std::cout << "  " << *d << '\n';
}

void handle(BloodBank& bank, int choice) {
    switch (choice) {
    case 1: {
        std::string name = ask("Name: ");
        int age = askNumber<int>("Age: ");
        std::string phone = ask("Phone: "), group = ask("Blood group: ");
        double weight = askNumber<double>("Weight (kg): ");
        std::string last = ask("Last donation date DD-MM-YYYY (blank if never): ");
        int id = bank.registerDonor(name, age, phone, group, weight, last);
        std::cout << "Donor registered with ID " << id << '\n';
        break;
    }
    case 2: {
        std::string name = ask("Name: ");
        int age = askNumber<int>("Age: ");
        std::string phone = ask("Phone: "), group = ask("Blood group: "), hospital = ask("Hospital: ");
        int id = bank.registerRecipient(name, age, phone, group, hospital);
        std::cout << "Recipient registered with ID " << id << '\n';
        break;
    }
    case 3: {
        int id = askNumber<int>("Donor ID: ");
        std::string date = ask("Donation date DD-MM-YYYY (blank = today): ");
        const BloodUnit& unit = bank.recordDonation(id, date);
        std::cout << "Stored: " << unit << '\n';
        break;
    }
    case 4: {
        int id = askNumber<int>("Recipient ID: ");
        int n = askNumber<int>("Units needed: ");
        try {
            const Request& r = bank.requestBlood(id, n);
            std::cout << "Issued. " << r << '\n';
        } catch (const ShortageException& e) {
            std::cout << e.what() << '\n';
            printCallList(bank, e.group());
        }
        break;
    }
    case 5: {
        std::cout << std::left << std::setw(8) << "Group" << std::setw(12) << "Available"
                  << std::setw(14) << "Expiring<=7d" << "Note\n" << std::string(64, '-') << '\n';
        for (const auto& [group, s] : bank.stockReport())
            std::cout << std::setw(8) << group << std::setw(12) << s.available << std::setw(14)
                      << s.expiringSoon << (s.available < BloodBank::kLowStockLevel ? "LOW STOCK" : "")
                      << '\n';
        auto soon = bank.unitsExpiringSoon();
        if (!soon.empty()) std::cout << "\nUse these first:\n";
        for (const BloodUnit* u : soon) std::cout << "  " << *u << '\n';
        break;
    }
    case 6:
        std::cout << bank.removeExpired() << " expired unit(s) removed from stock\n";
        break;
    case 7: {
        auto found = bank.search(ask("Search by ID, name or blood group: "));
        if (found.empty()) std::cout << "No record found.\n";
        for (const Person* p : found) std::cout << "  " << *p << '\n';   // virtual display()
        break;
    }
    case 8:
        printCallList(bank, BloodGroup::parse(ask("Blood group needed: ")).name());
        break;
    case 9:
        if (bank.requests().empty()) std::cout << "No requests yet.\n";
        for (const Request& r : bank.requests()) std::cout << "  " << r << '\n';
        break;
    default:
        throw InvalidInputException("choose 0-9");
    }
}

int main(int argc, char* argv[]) {
    std::string dataDir = "data";
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--echo") echoInput = true;
        else dataDir = argv[i];
    }
    try {
        BloodBank bank(dataDir, Date::today());
        bank.load();
        while (true) {
            std::cout << "\n=========== LifeLine Blood Bank  |  Today: " << bank.today() << " ===========\n"
                      << " 1. Register donor          2. Register recipient\n"
                      << " 3. Record donation         4. Request blood\n"
                      << " 5. Stock report            6. Remove expired units\n"
                      << " 7. Search person           8. Donor call list\n"
                      << " 9. Request history         0. Save and exit\n";
            try {
                int choice = askNumber<int>("Choice: ");
                if (choice == 0) break;
                handle(bank, choice);
            } catch (const BankException& e) {
                std::cout << "Error: " << e.what() << '\n';
            }
        }
        bank.saveAll();
        std::cout << "Data saved. Goodbye.\n";
    } catch (const std::ios_base::failure&) {
        std::cout << "\nInput closed. Data is saved after every change.\n";
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
