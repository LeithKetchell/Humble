/// include some basic string support for test purposes
#include <iostream>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON NINE:
/// POINTERS TO POINTERS
/// VOID POINTERS
/// STATIC CASTING

class Dummy{};

Dummy* CreateDummy(){
    return new Dummy;
}

void CreateDummy(Dummy** ppOut){
    *ppOut = new Dummy;
}


/// Application Entrypoint (for a Console Application)
int main()
{

    /// Methods and Functions can return new objects by pointer
    /// These are often called "factory functions".
    /// We should decorate pointer variables with a p so we can easily know its a pointer..
    Dummy* pDummy = CreateDummy();

    /// Trash that guy...
    delete pDummy;
    /// Whenever you delete a raw pointer, you should set it to null
    /// This makes debugging later a lot easier!
    pDummy = nullptr;

    /// This is a more interesting example.
    /// pDummy is a Pointer to a Dummy
    /// This example function requires "a pointer to a pointer to a Dummy" (Dummy**)
    /// It wants to know where to store the pointer to the new object it will create,
    /// and it wants that information to also be a pointer...
    /// Here we are using & to create a pointer to our existing pointer
    /// When the function returns, pDummy will contain a valid pointer
    CreateDummy(&pDummy);

    /// Trash that guy
    delete pDummy;     pDummy = nullptr;


    /// What is a void pointer?
    /// It is "a pointer to something" - could be anything, it is an "any" pointer.
    void* myPointer = nullptr;

    /// Let's define an integer variable to play with
    int myValue = 666;
    cout << "Initial value: " << myValue << endl;


    /// And let's create a pointer to that integer
    /// We will deliberately "forget" what datatype the pointer is pointing at! Its just void*
    myPointer = &myValue;

    /// Static Cast
    /// Use this when you "know" the true type of the pointer
    /// You are TELLING the compiler what type of pointer it is
    /// The compiler will obey, but if the type is wrong, the app will crash at runtime!
    /// There will be precious little information about what went wrong...
    int* pInteger = static_cast<int*>(myPointer);

    /// We use * prefix to access the data "behind" the pointer
    /// We can both read and write using * prefix
    /// In this case, the operation will work perfectly because we casted to the correct type
    /// but if we did a static cast to the wrong type, we would be crashing the app when we
    /// try to access the invalid (but not null!) pointer
    *pInteger += 5;

    cout << "New value: " << *pInteger << endl;

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
