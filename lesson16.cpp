/// include some basic string support for test purposes
#include <iostream>


/// We're too lazy to type this everywhere
using namespace std;

/// LESSON SIXTEEN:
/// BINARY FLAGS AND BITWISE LOGICAL OPERATORS


/// Enumeration
/// If we don't give explicit values, they start at zero, and increase by one.
/// Here I am defining a binary enumeration, which means the values
/// must increase by a factor of 2 - they represent bitflags
enum AnimalType{
    None=0,
    Flying=1,
    Furry=2,
    Flaming=4,
    Screaming=8,
    Annoying=16
};



class Animal{
public:
    Animal(AnimalType t) { type_ = t; }
    AnimalType type_;
    void Describe(){

        cout << "I am best described as ";

        /// We'll use a macro to define something we otherwise would repeat a lot
        /// NOTE 1: the use of "#" causes a macro argument to be interpreted as a "literal string value"
        /// So if we hand in Dog, we get "Dog"
        /// NOTE 2: the use of trailing backslash \
        /// indicates that the macro continues on the next line, its a multi line macro
        /// NOTE 3: the use of & here means "binary logical AND operator"
        /// Binary AND is used here to determine if a single bit is turned on or off
        #define DescribeMe(typename) if(type_ & typename) \
        cout << #typename << " "

        /// The macro is expressing inline sourcecode that gets "expanded" by the compiler at buildtime
        DescribeMe(Flying);
        DescribeMe(Furry);
        DescribeMe(Flaming);
        DescribeMe(Screaming);
        DescribeMe(Annoying);

        cout << endl;

    }
};


/// Application Entrypoint (for a Console Application)
int main()
{

    /// This is not a lesson in binary logic, but I can provide that if you wish.
    /// Here I use binary logical OR operator to enable certain "bitflags"
    /// The single pipe character "|" indicates a logical OR operator.
    /// We use it to enable, or "turn on" specific bits in a bitflag value
    /// C++ treats enum values as integers, so we have to remind C++ what type we wanted.
    AnimalType myType = (AnimalType) (Annoying | Flying | Furry);

    /// Let's make one of those...
    Animal myAnimal( myType );

    /// Let's examine the bitflags
    myAnimal.Describe();


    ///////////////////////////////////////////////////////////////////////////////////////////

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}

