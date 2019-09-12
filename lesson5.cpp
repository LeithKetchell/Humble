
/// include some basic string support for test purposes
#include <iostream>
#include <string>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON FIVE:
/// TEMPLATE SPECIALIZATION
template <typename T>
class ValueHolder
{
private:
    T myValue;
public:
    const T& Get() { return myValue; }
    void Set(const T& v) { myValue = v; }
};


/// Application Entrypoint (for a Console Application)
int main()
{

    /// Create two value holders using templated type specialization
    ValueHolder<int> v1, v2;

    /// Set value in holder1
    v1.Set(666);

    /// Copy the value holder
    v2 = v1;

    /// Verify that value holder is copyable
    cout << "Value is " << v2.Get() << endl;

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
