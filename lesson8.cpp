
/// include some basic string support for test purposes
#include <iostream>
#include <memory>

/// We're too lazy to type this everywhere
using namespace std;

class Dummy{
public:
    string meh;
};

/// LESSON EIGHT:
/// SMART POINTERS #1: SharedPtr

/// Application Entrypoint (for a Console Application)
int main()
{

    /// Danger #1 - we may forget to set a pointer to a valid value
    /// Hopefully we at least remembered to set it to zero / null
    /// otherwise there are no guarantees what the initial value might be...
    /// Either way, trying to use an invalid pointer will crash our app.
    Dummy* dummy = nullptr;

    /// Create a new object, store its pointer
    dummy = new Dummy();

    /// Danger #2 - we may delete the object, but forget to set the pointer to null
    /// This is called a "dangling pointer", if we try to use it, we will crash..
//    delete dummy;
//    dummy->meh = "testing";

    /// Danger #3 - we may forget to delete the object when we're finished with it
    /// This is called a memory leak.
    /// return 0;

    /// SharedPtr is a safety wrapper for raw object pointers.
    /// It is a reference-counting object.
    /// Multiple SharedPtrs can wrap the same raw object pointer

    /// If we store our raw pointer inside a SharedPtr wrapper,
    /// our object will be automatically deleted when
    /// the reference count of owner wrapper objects reaches zero,
    /// so when the last living sharedptr that owns an object goes out of scope
    /// or is otherwise reset or set to null, only then will our object get deleted
    shared_ptr<Dummy> safeptr(dummy);

    /// shared pointers act like regular pointers
    safeptr->meh="test2";


    /// If we really need to we can still access the raw pointer inside a sharedptr
    Dummy* rawptr = *safeptr;


    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
