#pragma once
#include "Person.h"
#include "Records.h"
#include <filesystem>
#include <fstream>
#include <memory>

// Reads and writes the three data files. One record per line, fields split by '|'.
class FileManager {
    std::string dir_;

    // Template helpers: get the text of an object or of a unique_ptr to one.
    template <typename T> static std::string text(const T& x) { return x.serialize(); }
    template <typename T> static std::string text(const std::unique_ptr<T>& p) { return p->serialize(); }

    template <typename T>
    void writeLines(const std::string& file, const std::vector<T>& items) const {
        std::ofstream out(dir_ + "/" + file);
        if (!out) throw BankException("cannot write " + dir_ + "/" + file);
        for (const auto& item : items) out << text(item) << '\n';
    }
    template <typename Handler>
    void readLines(const std::string& file, Handler handle) const {
        std::ifstream in(dir_ + "/" + file);
        std::string line;
        while (std::getline(in, line))
            if (!line.empty()) handle(split(line, '|'));
    }

public:
    explicit FileManager(const std::string& dir) : dir_(dir) {}

    static std::vector<std::string> split(const std::string& line, char sep) {
        std::vector<std::string> parts;
        std::string field;
        std::istringstream in(line);
        while (std::getline(in, field, sep)) parts.push_back(field);
        if (!line.empty() && line.back() == sep) parts.push_back("");
        return parts;
    }

    void load(std::vector<std::unique_ptr<Person>>& people, std::vector<BloodUnit>& units,
              std::vector<Request>& requests) const {
        people.clear(); units.clear(); requests.clear();
        readLines("people.txt", [&](const std::vector<std::string>& f) {
            if (f[0] == "D" && f.size() == 9) {
                bool gave = f[7] != "-";
                people.push_back(std::make_unique<Donor>(
                    std::stoi(f[1]), f[2], std::stoi(f[3]), f[4], BloodGroup::parse(f[5]),
                    std::stod(f[6]), gave, gave ? Date::parse(f[7]) : Date(), std::stoi(f[8])));
            } else if (f[0] == "R" && f.size() == 7) {
                people.push_back(std::make_unique<Recipient>(
                    std::stoi(f[1]), f[2], std::stoi(f[3]), f[4], BloodGroup::parse(f[5]), f[6]));
            }
        });
        readLines("units.txt", [&](const std::vector<std::string>& f) {
            if (f.size() == 6)
                units.emplace_back(std::stoi(f[0]), BloodGroup::parse(f[1]), std::stoi(f[2]),
                                   Date::parse(f[3]), f[4], std::stoi(f[5]));
        });
        readLines("requests.txt", [&](const std::vector<std::string>& f) {
            if (f.size() != 7) return;
            std::vector<int> ids;
            if (f[6] != "-")
                for (const auto& s : split(f[6], ',')) ids.push_back(std::stoi(s));
            requests.emplace_back(std::stoi(f[0]), std::stoi(f[1]), BloodGroup::parse(f[2]),
                                  std::stoi(f[3]), Date::parse(f[4]), f[5], ids);
        });
    }

    void save(const std::vector<std::unique_ptr<Person>>& people,
              const std::vector<BloodUnit>& units, const std::vector<Request>& requests) const {
        std::filesystem::create_directories(dir_);
        writeLines("people.txt", people);   // calls the virtual serialize()
        writeLines("units.txt", units);
        writeLines("requests.txt", requests);
    }
};
