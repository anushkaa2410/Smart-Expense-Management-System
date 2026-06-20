#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cmath>
#include <ctime>
#include <sstream>
#include <map>
#include <fstream>
#include <cstdio>

using namespace std;

static int globalExpenseIdCounter = 1;
const string FILE_NAME = "expenses.txt";
const string PROFILE_FILE = "profile.txt";

string getCurrentDate() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    stringstream ss;
    ss << 1900 + ltm->tm_year << "-" 
       << setfill('0') << setw(2) << 1 + ltm->tm_mon << "-" 
       << setfill('0') << setw(2) << ltm->tm_mday;
    return ss.str();
}

enum ExpenseType { FIXED = 0, SEMI_FLEXIBLE = 1, FLEXIBLE = 2 };

string getExpenseTypeName(ExpenseType t) {
    if (t == FIXED) return "Fixed";
    if (t == SEMI_FLEXIBLE) return "Semi-Flexible";
    return "Flexible";
}

struct UserProfile {
    double monthlyIncome;
    double targetSavings;
};

void saveProfile(const UserProfile& profile) {
    ofstream file(PROFILE_FILE, ios::trunc);
    if (file.is_open()) {
        file << fixed << setprecision(2) << profile.monthlyIncome << "\n" << profile.targetSavings << "\n";
    }
}

bool loadProfile(UserProfile& profile) {
    ifstream file(PROFILE_FILE);
    if (file.is_open()) {
        file >> profile.monthlyIncome >> profile.targetSavings;
        return true;
    }
    return false;
}

class Expense {
public:
    string id;
    string category;
    string subcategory;
    ExpenseType type;
    double amount;
    string date;
    int priority;         
    int importanceValue;

    Expense(string id, string cat, string subcat, ExpenseType type, double amt, string d, int prio, int imp)
        : id(id), category(cat), subcategory(subcat), type(type), amount(amt), date(d), priority(prio), importanceValue(imp) {}

    Expense(string cat, string subcat, ExpenseType type, double amt) {
        id = "EXP" + to_string(globalExpenseIdCounter++);
        category = cat;
        subcategory = subcat;
        this->type = type;
        amount = amt;
        date = getCurrentDate();
        
        if (type == FIXED) { priority = 1; importanceValue = 100; }
        else if (type == SEMI_FLEXIBLE) { priority = 2; importanceValue = 50; }
        else { priority = 3; importanceValue = 10; }
    }
};

class AlgorithmEngine {
public:
    
    static void merge(vector<Expense>& arr, int left, int mid, int right) {
        int n1 = mid - left + 1;
        int n2 = right - mid;
        
        vector<Expense> L(arr.begin() + left, arr.begin() + mid + 1);
        vector<Expense> R(arr.begin() + mid + 1, arr.begin() + right + 1);
        
        int i = 0, j = 0, k = left;
        
        while (i < n1 && j < n2) {
            if (L[i].amount <= R[j].amount) arr[k++] = L[i++];
            else arr[k++] = R[j++];
        }

        while (i < n1) arr[k++] = L[i++];
        while (j < n2) arr[k++] = R[j++];
    }

    static void mergeSort(vector<Expense>& arr, int left, int right) {
        if (left >= right) return; 
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);       
        mergeSort(arr, mid + 1, right); 
        merge(arr, left, mid, right); 
    }

    static int binarySearchAmount(const vector<Expense>& arr, double targetAmount) {
        int left = 0, right = arr.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            
            if (abs(arr[mid].amount - targetAmount) < 0.01) return mid; 
            
            if (arr[mid].amount < targetAmount) left = mid + 1; 
            else right = mid - 1; 
        }
        return -1;
    }

    static void dpSavingsGoal(const vector<Expense>& expenses, double targetDeficit) {
        double totalRemovable = 0;
        vector<Expense> cuttableItems;
        
        for (const auto& e : expenses) {
            if (e.type != FIXED) { 
                totalRemovable += e.amount;
                cuttableItems.push_back(e);
            }
        }

        if (targetDeficit > totalRemovable) {
            cout << "[PROBLEM] Shortfall is $" << targetDeficit << ", but you only have $" << totalRemovable << " in flexible expenses.\n";
            cout << "-> ACTION: You must cut all non-fixed expenses and find ways to increase income.\n";
            return;
        }

        double newBudgetForFlexible = totalRemovable - targetDeficit;
        int capacity = round(newBudgetForFlexible); 
        int n = cuttableItems.size();

        vector<vector<int>> dp(n + 1, vector<int>(capacity + 1, 0));

        for (int i = 1; i <= n; i++) {
            int cost = round(cuttableItems[i - 1].amount);
            int value = cuttableItems[i - 1].importanceValue;
            
            for (int w = 0; w <= capacity; w++) {
                if (cost <= w) {
                    dp[i][w] = max(dp[i - 1][w], dp[i - 1][w - cost] + value);
                } else {
                    dp[i][w] = dp[i - 1][w];
                }
            }
        }

        int w = capacity;
        vector<Expense> cutExpenses;

        for (int i = n; i > 0; i--) {
            if (w >= round(cuttableItems[i - 1].amount) && 
                dp[i][w] == dp[i - 1][w - round(cuttableItems[i - 1].amount)] + cuttableItems[i - 1].importanceValue) {
                w -= round(cuttableItems[i - 1].amount);
            } else {
                cutExpenses.push_back(cuttableItems[i - 1]);
            }
        }

        cout << "[OPTIMAL CUT PLAN]\n";
        if (cutExpenses.empty()) {
            cout << "-> No specific cuts needed internally.\n";
        } else {
            for (const auto& exp : cutExpenses) {
                cout << "- Cut " << exp.subcategory << " " << exp.category << " (Save $" << exp.amount << ")\n";
            }
        }
    }

    static void backtrackCuts(const vector<Expense>& expenses, int index, double currentSum, double targetSavings, 
                              vector<Expense>& currentCombo, int& solutionsFound) {
        if (solutionsFound >= 3) return; 

        if (currentSum >= targetSavings) {
            cout << "Option " << solutionsFound + 1 << ":\n";
            cout << "Cut -> ";
            for (size_t i = 0; i < currentCombo.size(); i++) {
                cout << currentCombo[i].category << " (" << currentCombo[i].subcategory << ")";
                if (i < currentCombo.size() - 1) cout << ", ";
            }
            cout << "\nSavings -> $" << fixed << setprecision(2) << currentSum << "\n\n";
            solutionsFound++;
            return;
        }

        for (int i = index; i < expenses.size(); i++) {
            if (expenses[i].type == FIXED) continue; 

            currentCombo.push_back(expenses[i]);
            backtrackCuts(expenses, i + 1, currentSum + expenses[i].amount, targetSavings, currentCombo, solutionsFound);
            currentCombo.pop_back();
        }
    }

    static double calculateTotalDAC(const vector<Expense>& expenses, int left, int right) {
        if (left == right) return expenses[left].amount;
        if (left > right) return 0;

        int mid = left + (right - left) / 2;

        double leftSum = calculateTotalDAC(expenses, left, mid);
        double rightSum = calculateTotalDAC(expenses, mid + 1, right);

        return leftSum + rightSum;
    }
};

class ExpenseManager {
private:
    vector<Expense> expenses;
    UserProfile userProfile;

    void loadExpensesFromFile() {
        ifstream file(FILE_NAME);
        if (!file.is_open()) return;
        
        expenses.clear();
        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string id, cat, subcat, typeStr, amtStr, date, prioStr;
            
            getline(ss, id, ',');
            getline(ss, cat, ',');
            getline(ss, subcat, ',');
            getline(ss, typeStr, ',');
            getline(ss, amtStr, ',');
            getline(ss, date, ',');
            getline(ss, prioStr, ',');
            
            try {
                ExpenseType type = static_cast<ExpenseType>(stoi(typeStr));
                double amt = stod(amtStr);
                int prio = stoi(prioStr);
                int imp = (type == FIXED) ? 100 : (type == SEMI_FLEXIBLE ? 50 : 10);
                
                expenses.emplace_back(id, cat, subcat, type, amt, date, prio, imp);

                string idNum = id.substr(3);
                int num = stoi(idNum);
                if (num >= globalExpenseIdCounter) globalExpenseIdCounter = num + 1;
            } catch (...) { continue; }
        }
        file.close();
        if (!expenses.empty()) {
            cout << "[SYSTEM] Loaded " << expenses.size() << " past expenses.\n";
        }
    }

    void rewriteExpenseFile() {
        ofstream file(FILE_NAME, ios::trunc);
        for (const auto& exp : expenses) {
            file << exp.id << "," << exp.category << "," << exp.subcategory << "," 
                 << exp.type << "," << exp.amount << "," << exp.date << "," << exp.priority << "\n";
        }
        file.close();
    }
    void appendExpenseToFile(const Expense& exp) {
        ofstream file(FILE_NAME, ios::app);
        if (file.is_open()) {
            file << exp.id << "," << exp.category << "," << exp.subcategory << "," 
                 << exp.type << "," << exp.amount << "," << exp.date << "," << exp.priority << "\n";
            file.close();
        }
    }

    void quickInsight(const string& category, double amount) {
        cout << "\n[AUTO INSIGHT]\n";
        if (category == "Rent") cout << "-> Rent is marked as an essential, non-removable housing cost.\n";
        else if (category == "Food" && amount > 100) cout << "-> High food cost detected. This is partially reducible later if you need to hit saving goals.\n";
        else if (category == "Entertainment" || category == "Misc") cout << "-> This is a non-essential luxury. It will be the first target for cuts if you overspend.\n";
        else cout << "-> Expense recorded successfully. Keep an eye on your overall trend.\n";
    }

    void selectCategory(string& category, string& subcategory, ExpenseType& type) {
        cout << "Categories:\n1. Food\n2. Travel\n3. Entertainment\n4. Rent\n5. Misc\nChoice: ";
        int catChoice; cin >> catChoice;
        
        if (catChoice == 1) {
            category = "Food";
            cout << "Subcategories:\n1. Essentials (Groceries)\n2. Luxury (Dining Out)\nChoice: ";
            int subChoice; cin >> subChoice;
            if (subChoice == 1) { subcategory = "Essentials"; type = SEMI_FLEXIBLE; }
            else { subcategory = "Luxury"; type = FLEXIBLE; }
        } else if (catChoice == 2) {
            category = "Travel";
            cout << "Subcategories:\n1. Necessary (Commute)\n2. Optional (Vacation)\nChoice: ";
            int subChoice; cin >> subChoice;
            if (subChoice == 1) { subcategory = "Necessary"; type = SEMI_FLEXIBLE; }
            else { subcategory = "Optional"; type = FLEXIBLE; }
        } else if (catChoice == 3) {
            category = "Entertainment";
            cout << "Subcategories:\n1. Subscriptions\n2. Outings\nChoice: ";
            int subChoice; cin >> subChoice;
            if (subChoice == 1) { subcategory = "Subscriptions"; type = FLEXIBLE; }
            else { subcategory = "Outings"; type = FLEXIBLE; }
        } else if (catChoice == 4) {
            category = "Rent"; subcategory = "Fixed Cost"; type = FIXED;
        } else {
            category = "Misc"; subcategory = "General"; type = FLEXIBLE;
        }
    }

public:
    ExpenseManager(UserProfile profile) : userProfile(profile) {
        loadExpensesFromFile();
    }
    void addExpenseInteractive() {
        cout << "\n============================================\n";
        cout << "            [ADD NEW EXPENSE]               \n";
        cout << "============================================\n";
        
        while (true) {
            string category, subcategory;
            ExpenseType type;
            selectCategory(category, subcategory, type);
            
            double amount = -1;
            while (amount < 0) {
                cout << "Enter Amount: $";
                cin >> amount;
                if (cin.fail() || amount < 0) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "[ERROR] Invalid amount.\n";
                    amount = -1;
                }
            }
            
            Expense newExp(category, subcategory, type, amount);
            expenses.push_back(newExp);
            appendExpenseToFile(newExp); 
            
            cout << "\n[SUCCESS] Expense Added!\n";
            quickInsight(category, amount);
            
            cout << "\nAdd another expense? (y/n): ";
            string choiceInput;
            cin >> ws; getline(cin, choiceInput);
            if (choiceInput.empty() || tolower(choiceInput[0]) != 'y') {
                break;
            }
        }
        
        if (expenses.size() > 1) {
            runAnalysisPipeline();
        }
    }

    void viewAndEditExpenses() {
        if (expenses.empty()) { cout << "No expenses to show.\n"; return; }
        
        cout << "\n1. View All Expenses\n2. Search by Exact Amount\n3. Edit Expense\n4. Delete Expense\nChoice: ";
        int choice; cin >> choice;
        
        if (choice == 1) {
            cout << "\n======================================================================================\n";
            cout << left << setw(10) << "ID" << setw(15) << "Category" << setw(18) << "Subcategory" 
                 << setw(16) << "Flexibility" << setw(10) << "Amount" << setw(15) << "Date\n";
            cout << string(86, '-') << "\n";
            
            for (const auto& e : expenses) {
                cout << left << setw(10) << e.id << setw(15) << e.category << setw(18) << e.subcategory 
                     << setw(16) << getExpenseTypeName(e.type) << "$" << setw(9) << fixed << setprecision(2) << e.amount 
                     << setw(15) << e.date << "\n";
            }
            cout << "======================================================================================\n";
        } else if (choice == 2) {
            vector<Expense> sortedExp = expenses;
            AlgorithmEngine::mergeSort(sortedExp, 0, sortedExp.size() - 1);
            
            double target;
            cout << "Enter exact amount to search: $";
            cin >> target;
            
            int idx = AlgorithmEngine::binarySearchAmount(sortedExp, target);
            if (idx != -1) {
                cout << "\n[SUCCESS] Found: " << sortedExp[idx].subcategory << " " << sortedExp[idx].category 
                     << " -> $" << sortedExp[idx].amount << " on " << sortedExp[idx].date << "\n";
            } else {
                cout << "\n[INFO] No expense found with exact amount $" << target << "\n";
            }
        } else if (choice == 3) {
            cout << "Enter Expense ID to edit (e.g., EXP1): ";
            string targetId; cin >> targetId;
            for (auto& exp : expenses) {
                if (exp.id == targetId) {
                    cout << "\n[EDITING " << exp.id << " | " << exp.category << " - $" << exp.amount << "]\n";
                    cout << "1. Edit Amount\n2. Edit Category & Amount\nChoice: ";
                    int editChoice; cin >> editChoice;
                    
                    if (editChoice == 1) {
                        cout << "Enter New Amount: $"; cin >> exp.amount;
                    } else if (editChoice == 2) {
                        string category, subcategory;
                        ExpenseType type;
                        selectCategory(category, subcategory, type);
                        exp.category = category;
                        exp.subcategory = subcategory;
                        exp.type = type;
                        if (type == FIXED) { exp.priority = 1; exp.importanceValue = 100; }
                        else if (type == SEMI_FLEXIBLE) { exp.priority = 2; exp.importanceValue = 50; }
                        else { exp.priority = 3; exp.importanceValue = 10; }
                        cout << "Enter New Amount: $"; cin >> exp.amount;
                    }
                    rewriteExpenseFile();
                    cout << "\n[SUCCESS] Expense updated.\n";
                    runAnalysisPipeline();
                    return;
                }
            }
            cout << "[ERROR] Expense ID not found.\n";
        } else if (choice == 4) {
            cout << "Enter Expense ID to delete (e.g., EXP1): ";
            string targetId; cin >> targetId;
            auto it = remove_if(expenses.begin(), expenses.end(), [&](const Expense& e){ return e.id == targetId; });
            if (it != expenses.end()) {
                expenses.erase(it, expenses.end());
                rewriteExpenseFile();
                cout << "\n[SUCCESS] Expense deleted.\n";
                if (!expenses.empty()) runAnalysisPipeline(); // Auto Update
            } else {
                cout << "[ERROR] Expense ID not found.\n";
            }
        }
    }

    void runAnalysisPipeline() {
        if (expenses.empty()) { cout << "No data for analysis.\n"; return; }
        
        int mid = expenses.size() / 2;
        double firstHalf = AlgorithmEngine::calculateTotalDAC(expenses, 0, mid - 1);
        double secondHalf = AlgorithmEngine::calculateTotalDAC(expenses, mid, expenses.size() - 1);
        
        double fixedTotal = 0, semiTotal = 0, flexTotal = 0;
        for (const auto& e : expenses) {
            if (e.type == FIXED) fixedTotal += e.amount;
            else if (e.type == SEMI_FLEXIBLE) semiTotal += e.amount;
            else flexTotal += e.amount;
        }
        double totalSpent = fixedTotal + semiTotal + flexTotal;
        double essentialTotal = fixedTotal + semiTotal;
        double nonEssentialTotal = flexTotal;
        
        cout << "\n============================================\n";
        cout << "      [SMART ANALYSIS & SUGGESTIONS]        \n";
        cout << "============================================\n";
        
        cout << "Total Monthly Spending: $" << fixed << setprecision(2) << totalSpent << "\n";
        cout << "Essential Expenses (Fixed + Semi): " << fixed << setprecision(1) << (essentialTotal/totalSpent*100) << "%\n";
        cout << "Non-Essential Expenses (Flexible): " << (nonEssentialTotal/totalSpent*100) << "%\n";
        
        double savingsRate = ((userProfile.monthlyIncome - totalSpent) / userProfile.monthlyIncome) * 100;
        cout << "Current Savings Rate: " << fixed << setprecision(1) << savingsRate << "%\n";
        
        cout << "\n[SPENDING HEALTH]\n";
        if (savingsRate >= 20) cout << "-> EXCELLENT (Saving >= 20% of income)\n";
        else if (savingsRate >= 10) cout << "-> GOOD (Saving 10-20% of income)\n";
        else if (savingsRate > 0) cout << "-> FAIR (Saving < 10% - consider reducing flexible costs)\n";
        else cout << "-> POOR (Deficit! You are overspending your income)\n";
        
        cout << "\n[TREND INSIGHT]\n";
        if (expenses.size() > 1 && secondHalf > firstHalf) cout << "-> Spending is accelerating toward the end of the records.\n";
        else cout << "-> Spending pace is well-controlled chronologically.\n";
        
        cout << "\n[PRIORITIZED ACTION PLAN]\n";
        vector<Expense> flexExpenses;
        for(const auto& e : expenses) if (e.type == FLEXIBLE) flexExpenses.push_back(e);

        AlgorithmEngine::mergeSort(flexExpenses, 0, flexExpenses.size() - 1);
        
        int suggestionCount = 0;
        for (int i = flexExpenses.size() - 1; i >= 0 && suggestionCount < 3; i--) {
            string impact = (flexExpenses[i].amount > 100) ? "HIGH IMPACT" : 
                            (flexExpenses[i].amount > 40) ? "MEDIUM IMPACT" : "LOW IMPACT";
            cout << "-> Reduce " << flexExpenses[i].category << " (" << flexExpenses[i].subcategory 
                 << ") spending by $" << flexExpenses[i].amount << " (" << impact << ")\n";
            suggestionCount++;
        }
        
        if (suggestionCount == 0) {
            cout << "-> No purely flexible expenses detected to cut. You are very frugal!\n";
        }
        
        cout << "============================================\n";
    }

    void savingsPlanner() {
        if (expenses.empty()) { cout << "No data for planning.\n"; return; }
        
        double totalSpent = 0;
        for (const auto& e : expenses) totalSpent += e.amount;
        
        double requiredSavings = userProfile.targetSavings;
        double currentSavings = userProfile.monthlyIncome - totalSpent;
        
        cout << "\n============================================\n";
        cout << "            [SAVINGS PLANNER]               \n";
        cout << "============================================\n";
        
        if (currentSavings >= requiredSavings) {
            cout << "-> Great news! You are projected to save $" << currentSavings << ".\n";
            cout << "-> This exceeds your target of $" << requiredSavings << ".\n";
            cout << "-> No budget cuts required.\n";
            cout << "============================================\n";
            return;
        }
        
        double deficit = requiredSavings - currentSavings;
        cout << "-> Target Savings: $" << requiredSavings << "\n";
        cout << "-> Current Projected Savings: $" << currentSavings << "\n";
        cout << "-> Shortfall: You need to cut $" << deficit << " from your budget.\n\n";

        AlgorithmEngine::dpSavingsGoal(expenses, deficit);

        cout << "\n[ALTERNATIVE COMBINATIONS (Backtracking)]\n";
        vector<Expense> currentCombo;
        int solutionsFound = 0;
        AlgorithmEngine::backtrackCuts(expenses, 0, 0.0, deficit, currentCombo, solutionsFound);
        
        if (solutionsFound == 0) cout << "-> No alternative exact cut combinations found.\n";
        
        cout << "============================================\n";
    }

    void predictAndRisk() {
        if (expenses.empty()) { cout << "No data.\n"; return; }
        
        double total = 0;
        for (const auto& e : expenses) total += e.amount;
        
        // 0.8% realistic monthly inflation compound
        double inflationRate = 0.008; 
        double prediction = total * (1.0 + inflationRate);
        
        cout << "\n============================================\n";
        cout << "           [PREDICTION & RISK]              \n";
        cout << "============================================\n";
        cout << "Current Total Spending: $" << fixed << setprecision(2) << total << "\n";
        cout << "Estimated Market Inflation: " << (inflationRate * 100) << "% this month.\n";
        cout << "Predicted Next Month Spending: $" << prediction << "\n\n";
        
        if (prediction > userProfile.monthlyIncome) {
            cout << "[RISK: CRITICAL]\n";
            cout << "-> Your baseline spending adjusted for inflation will exceed your income by $" 
                 << (prediction - userProfile.monthlyIncome) << ".\n";
            cout << "-> Please use the Savings Planner immediately.\n";
        } else {
            cout << "[RISK: LOW]\n";
            cout << "-> Projected spending is well within your income bounds.\n";
            cout << "-> Inflation will passively consume an extra $" << (prediction - total) << " of your savings.\n";
        }
        cout << "============================================\n";
    }

    void dataManagement() {
        cout << "\n============================================\n";
        cout << "      [DATA & PROFILE MANAGEMENT]           \n";
        cout << "============================================\n";
        cout << "1. Update Monthly Income\n";
        cout << "2. Update Target Savings\n";
        cout << "3. Reload Expenses from File\n";
        cout << "4. Clear All Expense Data (Deletes File)\n";
        cout << "5. Load Sample Data\n";
        cout << "6. Cancel\n";
        cout << "Choice: ";
        int choice;
        cin >> choice;
        if (choice == 1) {
            cout << "Enter new Monthly Income: $"; cin >> userProfile.monthlyIncome;
            saveProfile(userProfile);
            cout << "-> Income updated.\n";
            runAnalysisPipeline();
        } else if (choice == 2) {
            cout << "Enter new Target Savings: $"; cin >> userProfile.targetSavings;
            saveProfile(userProfile);
            cout << "-> Target savings updated.\n";
            runAnalysisPipeline();
        } else if (choice == 3) {
            loadExpensesFromFile();
            cout << "-> Data reloaded.\n";
        } else if (choice == 4) {
            expenses.clear();
            remove(FILE_NAME.c_str());
            globalExpenseIdCounter = 1;
            cout << "-> All expense data cleared and file deleted.\n";
        } else if (choice == 5) {
            expenses.clear();
            expenses.emplace_back("EXP1", "Rent", "Fixed Cost", FIXED, 1200.0, getCurrentDate(), 1, 100);
            expenses.emplace_back("EXP2", "Food", "Essentials", SEMI_FLEXIBLE, 300.0, getCurrentDate(), 2, 50);
            expenses.emplace_back("EXP3", "Food", "Luxury", FLEXIBLE, 150.0, getCurrentDate(), 3, 10);
            expenses.emplace_back("EXP4", "Travel", "Necessary", SEMI_FLEXIBLE, 100.0, getCurrentDate(), 2, 50);
            expenses.emplace_back("EXP5", "Entertainment", "Subscriptions", FLEXIBLE, 40.0, getCurrentDate(), 3, 10);
            expenses.emplace_back("EXP6", "Entertainment", "Outings", FLEXIBLE, 200.0, getCurrentDate(), 3, 10);
            globalExpenseIdCounter = 7;
            
            rewriteExpenseFile();
            cout << "-> Sample data loaded and saved.\n";
            runAnalysisPipeline();
        }
        cout << "============================================\n";
    }
};

void displayMenu() {
    cout << "\n============================================\n";
    cout << "   SMART MONTHLY EXPENSE DECISION SYSTEM    \n";
    cout << "============================================\n";
    cout << "1. Add Expense\n";
    cout << "2. View / Edit Expenses\n";
    cout << "3. Smart Analysis & Suggestions\n";
    cout << "4. Savings Planner (Achieve your Goal)\n";
    cout << "5. Prediction & Risk\n";
    cout << "6. Data & Profile Management\n";
    cout << "7. Exit\n";
    cout << "============================================\n";
    cout << "Enter choice: ";
}

int main() {
    cout << "\n============================================\n";
    UserProfile profile;
    
    if (loadProfile(profile)) {
        cout << "   Welcome back to your Finance Assistant!  \n";
        cout << "============================================\n";
        cout << "Current Monthly Income: $" << fixed << setprecision(2) << profile.monthlyIncome << "\n";
        cout << "Target Savings Goal: $" << profile.targetSavings << "\n";
    } else {
        cout << "   Welcome to your Personal Finance Assistant!\n";
        cout << "============================================\n";
        cout << "Before we begin, let's set up your profile.\n";
        cout << "Enter your expected Monthly Income: $";
        cin >> profile.monthlyIncome;
        cout << "Enter your Target Savings for this month: $";
        cin >> profile.targetSavings;
        saveProfile(profile);
    }
    
    ExpenseManager manager(profile);
    int choice;

    while (true) {
        displayMenu();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        switch (choice) {
            case 1: manager.addExpenseInteractive(); break;
            case 2: manager.viewAndEditExpenses(); break;
            case 3: manager.runAnalysisPipeline(); break;
            case 4: manager.savingsPlanner(); break;
            case 5: manager.predictAndRisk(); break;
            case 6: manager.dataManagement(); break;
            case 7: 
                cout << "Exiting Smart Expense System. Goodbye!\n"; 
                return 0;
            default: cout << "Invalid choice! Try again.\n";
        }
    }

    return 0;
}
