# LifeLine: Blood Bank Management System

Console application in C++17. Mini project for Object-Oriented Programming Using C++ (CIC-211).

LifeLine keeps records of donors, recipients and blood units for a small blood bank. It checks
blood-group compatibility, refuses donors who gave blood less than 90 days ago, issues the
unit that expires first, keeps O- blood for emergencies, and warns about units that expire
within 7 days. All data is saved in text files.

## Build and run

```sh
make            # builds ./lifeline (needs g++ or clang++ with C++17)
./lifeline      # data is stored in ./data
make test       # 86 unit tests + demo session checks
```

To replay the demo on a fixed date:

```sh
BB_TODAY=16-11-2026 ./lifeline demo/data --echo < demo/session1.txt
```

## Menu

| No. | Option | What it does |
|---|---|---|
| 1 | Register donor | Name, age 18-65, phone, group, weight 50 kg+, last donation date |
| 2 | Register recipient | Name, age, phone, group, hospital |
| 3 | Record donation | Checks the 90-day rule, stores a unit that expires after 42 days |
| 4 | Request blood | Issues compatible units, or shows the shortage and donors to call |
| 5 | Stock report | Units per group, low-stock flags, units expiring within 7 days |
| 6 | Remove expired units | Marks out-of-date units as Expired |
| 7 | Search person | By ID, part of a name, or blood group |
| 8 | Donor call list | Eligible donors who can give to a group |
| 9 | Request history | All requests, fulfilled or short |

## OOP concepts used

| Concept | Where |
|---|---|
| Abstraction, pure virtual functions | `Person::typeCode()`, `role()` |
| Inheritance | `Donor`, `Recipient` from `Person`; exceptions from `BankException` |
| Runtime polymorphism | `display()`, `serialize()` through `Person*` |
| Operator overloading | `Date` (`+`, `-`, `<`, `==`, `<<`), `BloodUnit::operator<`, `BloodGroup::operator==` |
| Templates | `findById()`, `idOf()`, `writeLines()` |
| Exception handling | `InvalidInputException`, `RecordNotFoundException`, `ShortageException` |
| Static members | ID counters in `Person`, `BloodUnit`, `Request` |
| Friend functions | `operator<<` for `Date`, `Person`, `BloodUnit`, `Request` |
| File handling | `FileManager` with `ifstream` / `ofstream` |
| STL | `vector`, `map`, `unique_ptr`, `sort`, `find_if` |

## Files

```
include/   one header per part, each class written in full
src/       main.cpp (menu)
tests/     unit_tests.cpp and run_tests.sh
demo/      scripted session input and output logs
```

## Data files

| File | Format |
|---|---|
| `people.txt` | `D\|id\|name\|age\|phone\|group\|weight\|lastDonation\|count` or `R\|id\|name\|age\|phone\|group\|hospital` |
| `units.txt` | `id\|group\|donorId\|collected\|status\|requestId` |
| `requests.txt` | `id\|recipientId\|group\|units\|date\|status\|unitIds` |
