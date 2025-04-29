#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace std;

class Person {
public:
    Person(string name) : name_(move(name)), balance_(0) {}
    void addUPI(const string& upi) { upis_.insert(upi); }
    void adjustBalance(int amount) { balance_ += amount; }
    const string& name() const noexcept { return name_; }
    int balance() const noexcept { return balance_; }
    const auto& upis() const noexcept { return upis_; }
private:
    string name_;
    int balance_;
    set<string> upis_;
};

class CashFlowMinimizer {
public:
    void run() {
        readParticipants();
        readTransactions();
        computeNetBalances();
        settleDebts();
        printResults();
    }

private:
    int n_;
    vector<Person> people_;
    unordered_map<string,int> idx_;
    vector<vector<int>> debtMatrix_;
    vector<tuple<int,int,int,string>> result_;

    void readParticipants() {
        cout << "\n=== Cash Flow Minimizer ===\n";
        cout << "Participants count (including Treasurer): ";
        cin >> n_;
        if (n_ < 2) throw runtime_error("At least 2 participants required.");
        people_.reserve(n_);
        debtMatrix_.assign(n_, vector<int>(n_, 0));
        for (int i = 0; i < n_; ++i) {
            string role = (i == 0 ? "Treasurer" : "Member");
            cout << role << " " << (i+1) << " - Name and UPI count: ";
            string name; int k;
            cin >> name >> k;
            if (!cin || k < 0) throw runtime_error("Invalid entry.");
            people_.emplace_back(name);
            idx_[name] = i;
            cout << "Enter UPIs: ";
            while (k--) { string u; cin >> u; people_.back().addUPI(u); }
        }
        // Ensure Treasurer supports all UPIs
        for (int i = 1; i < n_; ++i) {
            for (auto& u : people_[i].upis())
                people_[0].addUPI(u);
        }
    }

    void readTransactions() {
        cout << "\nNumber of debts: "; int m;
        cin >> m; if (!cin || m < 0) throw runtime_error("Invalid number.");
        cout << "Format: Debtor Creditor Amount\n";
        while (m--) {
            string d, c; int a;
            cin >> d >> c >> a;
            if (!cin || a <= 0 || idx_.count(d)==0 || idx_.count(c)==0)
                throw runtime_error("Invalid debt entry.");
            debtMatrix_[idx_[d]][idx_[c]] += a;
        }
    }

    void computeNetBalances() {
        for (int i = 0; i < n_; ++i)
            for (int j = 0; j < n_; ++j)
                people_[i].adjustBalance(debtMatrix_[j][i] - debtMatrix_[i][j]);
    }

    int getMaxCreditor() const {
        return max_element(people_.begin(), people_.end(),
            [](auto& a, auto& b){ return a.balance() < b.balance(); }) - people_.begin();
    }
    int getMaxDebtor() const {
        return min_element(people_.begin(), people_.end(),
            [](auto& a, auto& b){ return a.balance() < b.balance(); }) - people_.begin();
    }

    tuple<int,int,string> findSettlement(int debtor) {
        int bestCred = -1, bestBal = numeric_limits<int>::min();
        string via;
        for (int i = 0; i < n_; ++i) {
            if (people_[i].balance() <= 0) continue;
            vector<string> common;
            set_intersection(
                people_[debtor].upis().begin(), people_[debtor].upis().end(),
                people_[i].upis().begin(), people_[i].upis().end(),
                back_inserter(common));
            if (!common.empty() && people_[i].balance() > bestBal) {
                bestBal = people_[i].balance(); bestCred = i; via = common.front();
            }
        }
        return make_tuple(bestCred, bestCred >=0 ? bestBal : 0, via);
    }

    void settleDebts() {
        int settledCount = 0;
        vector<bool> settled(n_, false);
        auto updateSettled = [&]() {
            settledCount = 0;
            for (int i = 0; i < n_; ++i)
                if (people_[i].balance() == 0) { settled[i]=true; ++settledCount; }
        };
        updateSettled();
        while (settledCount < n_) {
            int d = getMaxDebtor();
            int amtToPay = -people_[d].balance();
            auto [cBalance, bal, via] = findSettlement(d);
            if (cBalance >= 0) {
                int transfer = min(amtToPay, bal);
                result_.emplace_back(d, cBalance, transfer, via);
                people_[d].adjustBalance(transfer);
                people_[cBalance].adjustBalance(-transfer);
            } else {
                int t = 0;
                // Debtor -> Treasurer
                result_.emplace_back(d, t, amtToPay, *people_[d].upis().begin());
                people_[t].adjustBalance(amtToPay);
                people_[d].adjustBalance(amtToPay);
                // Treasurer -> Highest Creditor
                int fc = getMaxCreditor();
                int tamt = people_[t].balance();
                result_.emplace_back(t, fc, tamt, *people_[fc].upis().begin());
                people_[fc].adjustBalance(-tamt);
                people_[t].adjustBalance(-tamt);
            }
            updateSettled();
        }
    }

    void printResults() const {
        cout << "\n=== Settlement Summary ===\n";
        cout << left << setw(15) << "Payer" << setw(15) << "Payee"
             << setw(8)  << "Amount" << "UPI\n";
        cout << string(50, '-') << "\n";
        for (auto& t : result_) {
            int d,c,a; string u;
            tie(d,c,a,u) = t;
            cout << left << setw(15) << people_[d].name()
                 << setw(15) << people_[c].name()
                 << setw(8)  << a << u << "\n";
        }
    }
};

int main() {
    try { CashFlowMinimizer().run(); }
    catch (const exception& e) { cerr << "Error: " << e.what() << "\n"; return 1; }
    return 0;
}