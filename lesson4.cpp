
/// include some basic string support for test purposes
#include <iostream>
#include <string>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON FOUR:
/// CONSTRUCTOR SPECIALIZATION


class Animal
{
public:
    string Name_;
    string Bleh_;

    Animal(const string& name, const string& bleh) : Name_(name), Bleh(bleh) { }

    void Talk()
    {
        cout << Name_ << " says " << Bleh_ << endl;
    }
};

class Cow:public Animal
{
public:
    Cow() : Animal("Cow", "Moo") { }
};

class Dog:public Animal
{
public:
    Dog() : Animal("Dog", "Woof") { }
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
