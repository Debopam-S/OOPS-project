#pragma once
#include "BloodGroup.h"
#include "Date.h"
#include <iomanip>
#include <sstream>

// Abstract base for everyone the blood bank keeps a record of.
class Person {
protected:
    int id_, age_;
    std::string name_, phone_;
    BloodGroup group_;

public:
    inline static int nextId = 1;   // shared ID counter

    Person(int id, const std::string& name, int age, const std::string& phone, BloodGroup g)
        : id_(id), age_(age), name_(name), phone_(phone), group_(g) {}
    virtual ~Person() = default;

    virtual char typeCode() const = 0;      // 'D' or 'R'
    virtual std::string role() const = 0;   // "Donor" or "Recipient"
    virtual void display(std::ostream& os) const {
        os << std::left << std::setw(4) << id_ << std::setw(10) << role() << std::setw(20) << name_
           << std::setw(5) << age_ << std::setw(5) << group_.name() << std::setw(12) << phone_;
    }
    virtual std::string serialize() const {
        std::ostringstream out;
        out << typeCode() << '|' << id_ << '|' << name_ << '|' << age_ << '|' << phone_ << '|'
            << group_.name();
        return out.str();
    }

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& phone() const { return phone_; }
    BloodGroup group() const { return group_; }

    friend std::ostream& operator<<(std::ostream& os, const Person& p) {
        p.display(os);   // virtual call: Donor or Recipient version
        return os;
    }
};

class Donor : public Person {
    double weight_;
    bool hasDonated_;
    Date last_;
    int donations_;

public:
    static const int kMinGapDays = 90;   // whole-blood donors wait 3 months

    Donor(int id, const std::string& name, int age, const std::string& phone, BloodGroup g,
          double weight, bool hasDonated = false, Date last = Date(), int donations = 0)
        : Person(id, name, age, phone, g), weight_(weight), hasDonated_(hasDonated), last_(last),
          donations_(donations) {}

    char typeCode() const override { return 'D'; }
    std::string role() const override { return "Donor"; }
    void display(std::ostream& os) const override {
        Person::display(os);
        os << "last gave: " << (hasDonated_ ? last_.str() : "never") << " (" << donations_ << "x)";
    }
    std::string serialize() const override {
        std::ostringstream out;
        out << Person::serialize() << '|' << weight_ << '|' << (hasDonated_ ? last_.str() : "-")
            << '|' << donations_;
        return out.str();
    }

    // True if the donor may give blood on `day`. If not, `reason` explains why.
    bool isEligible(const Date& day, std::string* reason = nullptr) const {
        std::string why;
        if (age_ < 18 || age_ > 65) why = "age must be 18-65";
        else if (weight_ < 50) why = "weight must be at least 50 kg";
        else if (hasDonated_ && day - last_ < kMinGapDays)
            why = "last donation " + last_.str() + ", can give again from " +
                  (last_ + kMinGapDays).str();
        if (reason) *reason = why;
        return why.empty();
    }
    void recordDonation(const Date& day) { hasDonated_ = true; last_ = day; ++donations_; }
    bool hasDonated() const { return hasDonated_; }
    const Date& lastDonation() const { return last_; }
};

class Recipient : public Person {
    std::string hospital_;

public:
    Recipient(int id, const std::string& name, int age, const std::string& phone, BloodGroup g,
              const std::string& hospital)
        : Person(id, name, age, phone, g), hospital_(hospital) {}

    char typeCode() const override { return 'R'; }
    std::string role() const override { return "Recipient"; }
    void display(std::ostream& os) const override {
        Person::display(os);
        os << "hospital: " << hospital_;
    }
    std::string serialize() const override { return Person::serialize() + '|' + hospital_; }
};
