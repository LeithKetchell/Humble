
/// include some basic string support for test purposes
#include <iostream>
#include <string>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON SIX:
/// TEMPLATED CONSTRUCTOR SPECIALIZATION
/// CONVERTING A POINTER TO A REFERENCE

template <typename T>
class ValueHolder
{
private:
    T myValue;
public:
    ValueHolder(const T& v){ myValue = v; } // new: type-specialized constructor
    const T& Get() { return myValue; }
    void Set(const T& v) { myValue = v; }
};


/// Application Entrypoint (for a Console Application)
int main()
{

    ValueHolder<string> v1("hello world!");

    // create reference to v1
    // writing to this handle will modify v1
    ValueHolder<string>& handleToVal = v1;

    // create pointer to v1
    // NOTE: since we did not create the pointer using "new", we don't need to "delete" it later.
    ValueHolder<string>* pointerToVal = &v1;

    cout << "by ref is " << handleToVal.Get() << endl;

    cout << "by ptr is " << pointerToVal->Get() << endl;

    // Convert pointer into a reference
    // NOTE: All these handles and pointers are really referring to our v1 variable - they are not copies!
    // Setting value on any of them will be visible in all the others
    ValueHolder<string>& anotherHandle = *pointerToVal;
    cout << "by deferenced ptr is " << anotherHandle.Get() << endl;

    /// NOTE: We can't convert a reference back into a pointer!
    /// This is one key difference between pointers and references.


    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
