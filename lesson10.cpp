/// include some basic string support for test purposes
#include <iostream>
#include <vector>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON TEN:
/// STORING RELATED BUT DIFFERENT TYPED OBJECTS IN ONE COLLECTION
/// DYNAMIC CASTING
/// REINTERPRET CASTING


class Animal {
public:
 /// Animal class is a base class.
 /// Since we intend it to be a base class, it is a great idea to add a virtual destructor method.
 /// The reason is that it makes it "safe" for us to "delete by base type"
 /// If we do this, the destructor of the real type (and any intermediate classes, backwards order of ancestry)
 /// will get called when we try to "delete by base pointer"... if we do not do this,
 /// then only the destructor in the base class would get called...
 /// If we want our object class hierarchy to always destruct nicely, DO THIS in the most base class you have
 virtual ~Animal() {}
};

class Dog:public Animal {};
class Cat:public Animal {};

/// Application Entrypoint (for a Console Application)
int main()
{

    /// We'll create a vector of Animals using the base class
    vector<Animal*> animals;

    /// Collect some animals of specific types that derive from the common base class
    /// We can do this because they share the same base class and can all be cast to it
    animals.push_back(new Dog);
    animals.push_back(new Cat);

    /// Dynamic Cast is used to convert a pointer from base class to a more derived / specific class
    /// We can use it to convert from Animal* to Dog* or Cat*
    /// It actually checks at runtime if the conversion is legal,
    /// and will return nullptr if we specify a bad type... we can use it to do basic type checking!

    Animal* animal = animals[0];
    Dog* pDog = dynamic_cast<Dog*>(animal);
    if(pDog==nullptr)
        cout << "Animal in slot 0 is not a dog" << endl;
    else
        cout << "Animal in slot 0 is a dog" << endl;

    pDog = dynamic_cast<Dog*>(animals[1]);
    if(pDog==nullptr)
        cout << "Animal in slot 1 is not a dog" << endl;
    else
        cout << "Animal in slot 1 is a dog" << endl;

    /// Let's say we have been handed a void*
    /// A lot of public code api work with it because it is a rubber datatype.
    /// We can't use static cast on a void*, it won't work. Compiler says no, that is dangerous.
    /// And neither will dynamic cast, because that requires runtime type info, and void* has none.
    /// What can we do?
    void* pvoid = animals[1];

    /// If we feel safe that we "know" the true type, we can reinterpret cast.
    /// This is more like static cast, than dynamic, in that no type checking is done at runtime.
    /// You will receive a pointer whose value is potentially invalid!
    /// In this case, we know theres a dog in slot #0 and a cat in slot #1
    /// Reinterpret cast is very powerful and also very dangerous.
    /// It can convert a pointer from any type to any other type.
    /// It can turn a pointer into an integer number, and an integer number into a pointer to any type.
    /// And so it can certainly cast a void pointer to any type!
    Cat* pCat = reinterpret_cast<Cat*>(pvoid);

    /// Although this lesson is over, we should clean up our mess...
    /// Our vector contains objects that were created with "new"
    /// and this indicates we are responsible at some point for cleanup..
    while(!animals.empty()){
        delete animals.back();
        animals.pop_back();
    }

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
