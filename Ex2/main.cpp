#include "kosaraju-sharir.hpp"
#include <iostream>

using namespace std;

int main() {
    int choice;
    cout << "Choose implementation:\n";
    cout << "1. Using std::list\n";
    cout << "2. Using std::deque\n";
    cin >> choice;

    if (choice == 1) {
        runKosarajuSharirWithList();
    } else if (choice == 2) {
        runKosarajuSharirWithDeque();
    } else {
        cout << "Invalid choice" << endl;
    }

    return 0;
}
