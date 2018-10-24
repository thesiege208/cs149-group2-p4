#include <forward_list>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <typeinfo>
#include "common.h"

using namespace std;

int main() {
    forward_list<Process> list {Process(), Process(), Process()};
    Process p;
    for (auto it = list.begin(); it != list.end(); ++it) {
    cout << typeid(*it).name() << endl;
    p = *it;
    cout << typeid(p).name() << endl;
    }
}
