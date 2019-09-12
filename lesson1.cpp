
/// include some basic string support for test purposes
#include <iostream>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON ONE:
/// SIMPLE CLASSES


class Animal
{
public:

    /// Constructor, if present, is called when a new object is created
    /// Note that there is no return type 
    Animal()    { }

    /// Destructor, if present, is called when an object is being destroyed
    /// Again, no return type is required
    ~Animal()   { }

    /// Our first public method
    /// Return type of "void" means that the method does not return anything
    void Talk()
    {
        cout << "Animal says Hello world!" << endl;
    }
};

/// Application Entrypoint (for a Console Application)
int main()
{
    /// Create local instances of "Animal" class
    Animal testAnimal;

    /// Call a public method on the Animal objects
    testAnimal.Talk();

    /// Local objects will get destroyed when this function returns to its caller
    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
