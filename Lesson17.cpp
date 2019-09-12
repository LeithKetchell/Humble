/// include some basic string support for test purposes
#include <iostream>
#include <map>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON SEVENTEEN:
/// BIT SHIFTING
/// EXAMPLE APPLICATION OF MAP CONTAINER

enum AnimalType{
    None=0,
    Flying=1,
    Furry=2,
    Flaming=4,
    Screaming=8,
    Annoying=16
};

/// Map
/// We'll use this map to decode bitflags at runtime
/// Think of it as a "lookup table"
map<unsigned int, string> myMap {
    {0, "None"},
    {1, "Flying"},
    {2, "Furry"},
    {4, "Flaming"},
    {8, "Screaming"},
    {16,"Annoying"}
};


class Animal{
public:
    Animal(AnimalType t) { type_ = t; }
    AnimalType type_;
    void Describe(){

        cout << "I am best described as ";

        /// In this loop, "i" will begin with a value of 1
        /// NOTE: The << operator in this case means "binary left shift"
        /// Instead of incrementing the value of i, we will double it,
        /// by shifting the value one place to the left!
        /// Therefore the value of "i" will be 1, 2, 4, 8, 16
        for(int i=1; i<=16; i = i<<1)
        {
            if(type_ & i)
                cout << myMap[i] << " ";
        }

        cout << endl;

    }
};


/// Application Entrypoint (for a Console Application)
int main()
{

    /// Play it again, Sam
    Animal myAnimal( (AnimalType) (Annoying | Flying | Furry) );
    myAnimal.Describe();


    ///////////////////////////////////////////////////////////////////////////////////////////

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}

