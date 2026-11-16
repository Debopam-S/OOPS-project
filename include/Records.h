#pragma once
#include "BloodGroup.h"
#include "Date.h"
#include <iomanip>
#include <sstream>
#include <vector>

// One bag of whole blood (about 350 ml).
class BloodUnit {
    int id_, donorId_, requestId_;   // requestId_ = request it was issued to, 0 if none
    BloodGroup group_;
    Date collected_, expiry_;
    std::string status_;             // Available, Issued or Expired

public:
    inline static int nextId = 1;
    static const int kShelfLifeDays = 42;   // whole blood in a CPDA-1 bag

    BloodUnit(int id, BloodGroup g, int donorId, Date collected,
              const std::string& status = "Available", int requestId = 0)
        : id_(id), donorId_(donorId), requestId_(requestId), group_(g), collected_(collected),
          expiry_(collected + kShelfLifeDays), status_(status) {}

    int id() const { return id_; }
    BloodGroup group() const { return group_; }
    const Date& expiry() const { return expiry_; }
    const std::string& status() const { return status_; }

    // Usable on `day` if still in stock and the expiry date has not been reached.
    bool isUsable(const Date& day) const { return status_ == "Available" && day < expiry_; }
    void issue(int requestId) { status_ = "Issued"; requestId_ = requestId; }
    void markExpired() { status_ = "Expired"; }

    // Units are ordered by expiry date: the one that expires first comes first.
    bool operator<(const BloodUnit& o) const { return expiry_ < o.expiry_; }

    std::string serialize() const {
        std::ostringstream out;
        out << id_ << '|' << group_.name() << '|' << donorId_ << '|' << collected_ << '|'
            << status_ << '|' << requestId_;
        return out.str();
    }
    friend std::ostream& operator<<(std::ostream& os, const BloodUnit& u) {
        return os << "Unit " << std::left << std::setw(4) << u.id_ << std::setw(4) << u.group_.name()
                  << "from donor " << std::setw(4) << u.donorId_ << "collected " << u.collected_
                  << "  expires " << u.expiry_ << "  " << u.status_;
    }
};

// A hospital request for blood for one recipient.
class Request {
    int id_, recipientId_, units_;
    BloodGroup group_;
    Date date_;
    std::string status_;          // Fulfilled or Shortage
    std::vector<int> unitIds_;    // units given out

public:
    inline static int nextId = 1;

    Request(int id, int recipientId, BloodGroup g, int units, Date date, const std::string& status,
            const std::vector<int>& unitIds)
        : id_(id), recipientId_(recipientId), units_(units), group_(g), date_(date),
          status_(status), unitIds_(unitIds) {}

    int id() const { return id_; }
    const std::string& status() const { return status_; }
    const std::vector<int>& unitIds() const { return unitIds_; }

    std::string serialize() const {
        std::ostringstream out;
        out << id_ << '|' << recipientId_ << '|' << group_.name() << '|' << units_ << '|' << date_
            << '|' << status_ << '|';
        if (unitIds_.empty()) out << '-';
        for (size_t i = 0; i < unitIds_.size(); ++i) out << (i ? "," : "") << unitIds_[i];
        return out.str();
    }
    friend std::ostream& operator<<(std::ostream& os, const Request& r) {
        os << "Request " << std::left << std::setw(4) << r.id_ << r.date_ << "  recipient "
           << std::setw(4) << r.recipientId_ << std::setw(4) << r.group_.name() << r.units_
           << " unit(s)  " << std::setw(10) << r.status_;
        if (!r.unitIds_.empty()) os << " units:";
        for (int id : r.unitIds_) os << ' ' << id;
        return os;
    }
};
