/// include some basic string support for test purposes
#include <iostream>
#include <map>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON ELEVEN:
/// MAPS!
/// THIS CONTAINER HOLDS "KEY/VALUE PAIRS"
/// WHERE WE CAN STORE DATA USING A "LOOKUP KEY"
/// INSTEAD OF AN ARRAY STORAGE INDEX

class Animal {
public:
 virtual ~Animal() {}
};

class Dog:public Animal {};
class Cat:public Animal {};

/// Application Entrypoint (for a Console Application)
int main()
{

    /// We'll create a map of Animals using string as key, and base class pointer as value
    map<string,Animal*> animals;

    /// We'll add some new animals to our map
    animals["dog"] = new Dog;
    animals["cat"] = new Cat;

    /// We can look up object pointers by name, and use casting operations on them
    Dog* pDog = dynamic_cast<Dog*>( animals["dog"] );
    Cat* pCat = dynamic_cast<Cat*>( animals["cat"] );

    /// When we iterate a map, "it" is a key/value pair whose names are "first" (the key) and "second" (the value)
    /// Here I am making sure that every Animal* is being deleted (since we used new to create them)
    for(auto kvpair=animals.begin();kvpair!=animals.end();kvpair++)
        delete kvpair->second;

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}
