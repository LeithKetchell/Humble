
/// include some basic string support for test purposes
#include <iostream>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON THREE:
/// POINTER OBJECTS - NEW AND DELETE OPERATORS


/// This is our "base class", for all kinds of "Animal"
class Animal
{
public:
    /// Let's define a "virtual method" in the base class...
    /// This means that the method can be "replaced" in derived classes
    virtual void Talk()
    {
        cout << "Animal says Hello world!" << endl;
    }
};

/// The Cow class derives from (ie is a "kind of") Animal
class Cow:public Animal
{
public:

    /// We are overriding a virtual method defined in the base class
    void Talk()
    {
        cout << "Cow says Moo!" << endl;
    }
};

/// Let's define another kind of Animal...
class Dog:public Animal
{
public:
    void Talk() { cout << "Dog says Woof!" << endl; }
};

/// Application Entrypoint (for a Console Application)
int main()
{
    /// Create new instances of "Animal" class
    Animal* animal1 = new Cow();
    Animal* animal2 = new Dog();

    /// Calling methods by Pointer, cannot use "dot calling convention", must use "->"
    animal1->Talk();
    animal2->Talk();

    /// We're responsible for destroying the objects when no longer required
    delete animal1;
    delete animal2;

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
