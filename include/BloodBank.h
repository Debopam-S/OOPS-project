#pragma once
#include "FileManager.h"
#include <algorithm>
#include <map>

// Function templates: find a record by ID in a vector of objects or of unique_ptrs.
template <typename T> int idOf(const T& item) { return item.id(); }
template <typename T> int idOf(const std::unique_ptr<T>& item) { return item->id(); }
template <typename Container> auto findById(Container& c, int id) {
    return std::find_if(c.begin(), c.end(), [id](const auto& item) { return idOf(item) == id; });
}

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
inline std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

struct GroupStock { int available = 0, expiringSoon = 0; };

// Controller: owns all records and applies the rules.
class BloodBank {
    std::vector<std::unique_ptr<Person>> people_;
    std::vector<BloodUnit> units_;
    std::vector<Request> requests_;
    FileManager files_;
    Date today_;

    // Finds a Donor or Recipient by ID; dynamic_cast checks the real type.
    template <typename T> T& person(int id, const std::string& kind) {
        auto it = findById(people_, id);
        T* p = it == people_.end() ? nullptr : dynamic_cast<T*>(it->get());
        if (!p) throw RecordNotFoundException("no " + kind + " with ID " + std::to_string(id));
        return *p;
    }
    static std::string checkName(const std::string& raw) {
        std::string n = trim(raw);
        if (n.empty() || n.size() > 40) throw InvalidInputException("name must be 1-40 characters");
        for (char c : n)
            if (!std::isalpha(static_cast<unsigned char>(c)) && c != ' ' && c != '.')
                throw InvalidInputException("name may only have letters, spaces and dots");
        return n;
    }
    static std::string checkPhone(const std::string& raw) {
        std::string p = trim(raw);
        bool ok = p.size() == 10 && p[0] >= '6' && p[0] <= '9';
        for (char c : p) ok = ok && std::isdigit(static_cast<unsigned char>(c));
        if (!ok) throw InvalidInputException("phone must be a 10-digit mobile number starting 6-9");
        return p;
    }

public:
    static const int kExpiryAlertDays = 7, kLowStockLevel = 2;

    BloodBank(const std::string& dataDir, const Date& today) : files_(dataDir), today_(today) {}
    const Date& today() const { return today_; }
    const std::vector<Request>& requests() const { return requests_; }

    void load() {
        files_.load(people_, units_, requests_);
        // Continue IDs after the highest one already stored.
        auto next = [](const auto& v) {
            int m = 0;
            for (const auto& x : v) m = std::max(m, idOf(x));
            return m + 1;
        };
        Person::nextId = next(people_);
        BloodUnit::nextId = next(units_);
        Request::nextId = next(requests_);
    }
    void saveAll() const { files_.save(people_, units_, requests_); }

    int registerDonor(const std::string& name, int age, const std::string& phone,
                      const std::string& group, double weight, const std::string& lastDonation) {
        std::string n = checkName(name), p = checkPhone(phone), last = trim(lastDonation);
        BloodGroup g = BloodGroup::parse(group);
        if (age < 18 || age > 65) throw InvalidInputException("donor age must be 18-65");
        if (weight < 50 || weight > 200) throw InvalidInputException("donor weight must be 50-200 kg");
        bool gave = !last.empty() && last != "-";
        Date lastDate = gave ? Date::parse(last) : Date();
        if (gave && today_ < lastDate) throw InvalidInputException("last donation cannot be in the future");
        for (const auto& other : people_)
            if (other->typeCode() == 'D' && other->phone() == p)
                throw InvalidInputException("a donor with phone " + p + " is already registered (ID " +
                                            std::to_string(other->id()) + ")");
        int id = Person::nextId++;
        people_.push_back(std::make_unique<Donor>(id, n, age, p, g, weight, gave, lastDate, gave ? 1 : 0));
        saveAll();
        return id;
    }

    int registerRecipient(const std::string& name, int age, const std::string& phone,
                          const std::string& group, const std::string& hospital) {
        std::string n = checkName(name), p = checkPhone(phone), h = trim(hospital);
        BloodGroup g = BloodGroup::parse(group);
        if (age < 0 || age > 120) throw InvalidInputException("age must be 0-120");
        if (h.empty() || h.find('|') != std::string::npos)
            throw InvalidInputException("hospital name is required and cannot contain '|'");
        int id = Person::nextId++;
        people_.push_back(std::make_unique<Recipient>(id, n, age, p, g, h));
        saveAll();
        return id;
    }

    const BloodUnit& recordDonation(int donorId, const std::string& date) {
        Donor& d = person<Donor>(donorId, "donor");
        Date day = trim(date).empty() ? today_ : Date::parse(trim(date));
        if (today_ < day) throw InvalidInputException("donation date cannot be in the future");
        std::string reason;
        if (!d.isEligible(day, &reason))
            throw InvalidInputException(d.name() + " cannot donate: " + reason);
        d.recordDonation(day);
        units_.emplace_back(BloodUnit::nextId++, d.group(), d.id(), day);
        saveAll();
        return units_.back();
    }

    // Issues compatible units, or records a shortage and throws ShortageException.
    const Request& requestBlood(int recipientId, int count) {
        Recipient& r = person<Recipient>(recipientId, "recipient");
        if (count < 1 || count > 10) throw InvalidInputException("units must be 1-10");

        std::vector<BloodUnit*> match;   // all usable units this recipient can safely receive
        for (auto& u : units_)
            if (u.isUsable(today_) && u.group().canDonateTo(r.group())) match.push_back(&u);

        // 1) same group first, 2) then the least flexible group (keeps O- for emergencies),
        // 3) then the unit that expires first.
        BloodGroup want = r.group();
        std::sort(match.begin(), match.end(), [want](const BloodUnit* a, const BloodUnit* b) {
            bool aSame = a->group() == want, bSame = b->group() == want;
            if (aSame != bSame) return aSame;
            int aServes = a->group().servesCount(), bServes = b->group().servesCount();
            if (aServes != bServes) return aServes < bServes;
            return *a < *b;   // BloodUnit::operator< compares expiry dates
        });

        int id = Request::nextId++;
        if (static_cast<int>(match.size()) < count) {
            requests_.emplace_back(id, r.id(), want, count, today_, "Shortage", std::vector<int>{});
            saveAll();
            throw ShortageException(want.name(), count, static_cast<int>(match.size()));
        }
        std::vector<int> given;
        for (int i = 0; i < count; ++i) {
            match[i]->issue(id);
            given.push_back(match[i]->id());
        }
        requests_.emplace_back(id, r.id(), want, count, today_, "Fulfilled", given);
        saveAll();
        return requests_.back();
    }

    std::map<std::string, GroupStock> stockReport() const {
        std::map<std::string, GroupStock> report;
        for (const BloodGroup& g : BloodGroup::all()) report[g.name()];
        for (const auto& u : units_)
            if (u.isUsable(today_)) {
                GroupStock& s = report[u.group().name()];
                ++s.available;
                if (u.expiry() - today_ <= kExpiryAlertDays) ++s.expiringSoon;
            }
        return report;
    }

    std::vector<const BloodUnit*> unitsExpiringSoon() const {
        std::vector<const BloodUnit*> list;
        for (const auto& u : units_)
            if (u.isUsable(today_) && u.expiry() - today_ <= kExpiryAlertDays) list.push_back(&u);
        std::sort(list.begin(), list.end(), [](const BloodUnit* a, const BloodUnit* b) { return *a < *b; });
        return list;
    }

    int removeExpired() {
        int n = 0;
        for (auto& u : units_)
            if (u.status() == "Available" && !(today_ < u.expiry())) { u.markExpired(); ++n; }
        if (n) saveAll();
        return n;
    }

    // Search by ID (digits), blood group, or part of a name.
    std::vector<const Person*> search(const std::string& rawKey) const {
        std::string key = trim(rawKey);
        std::vector<const Person*> found;
        if (key.empty()) return found;
        if (std::all_of(key.begin(), key.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
            auto it = findById(people_, std::stoi(key));
            if (it == people_.end()) throw RecordNotFoundException("no person with ID " + key);
            found.push_back(it->get());
            return found;
        }
        bool isGroup = true;
        BloodGroup g;
        try { g = BloodGroup::parse(key); } catch (const InvalidInputException&) { isGroup = false; }
        for (const auto& p : people_)
            if (isGroup ? p->group() == g : lower(p->name()).find(lower(key)) != std::string::npos)
                found.push_back(p.get());
        return found;
    }

    // Eligible donors who can give to `group`: same group first, then longest since giving.
    std::vector<const Donor*> callList(const std::string& group) const {
        BloodGroup need = BloodGroup::parse(group);
        std::vector<const Donor*> list;
        for (const auto& p : people_) {
            const Donor* d = dynamic_cast<const Donor*>(p.get());
            if (d && d->group().canDonateTo(need) && d->isEligible(today_)) list.push_back(d);
        }
        std::sort(list.begin(), list.end(), [need](const Donor* a, const Donor* b) {
            bool aSame = a->group() == need, bSame = b->group() == need;
            if (aSame != bSame) return aSame;
            if (a->hasDonated() != b->hasDonated()) return !a->hasDonated();
            return a->lastDonation() < b->lastDonation();
        });
        return list;
    }
};
