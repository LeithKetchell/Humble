/// include some basic string support for test purposes
#include <iostream>
#include <map>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON FOURTEEN:
/// ARRAY NEW AND HOW TO "PLACEMENT DELETE"
///
/// This is an alternative to using malloc and free
/// array new[] was meant to be used with primitive value types
/// such as ints and floats and strings
/// However we shall see that we can use it for more complex objects..

/// Our crash test dummy class has two different constructors...
class myClass {
public:

 myClass(string ugh):something(ugh){ cout << "string constructor: "<<ugh<<endl;}
 myClass(float  ugh):somefloat(ugh){ cout << "float constructor: "<<ugh<<endl;}

 virtual ~myClass() {
 cout << "destructor!"<<endl;
 }

 string something;
 float somefloat;
};



/// Application Entrypoint (for a Console Application)
int main()
{

    /// Allocate a flat array using "array-new" operator
    myClass* myArray = (myClass*)operator new[] (3*sizeof(myClass));

    /// Hey, we already got full access to the array elements
    myArray[1].somefloat = 667.0f;

    /// OPTIONAL:
    /// Initialize our objects using placement new (call constructor method)
    for(int i=0;i<3;i++)
        new (&myArray[i]) myClass(i);

    /// OPTIONAL:
    /// De-initialize our objects by manually calling their destructors
    /// DO NOT USE REGULAR DELETE - WE DID NOT USE REGULAR NEW
    for(int i=0;i<3;i++)
        myArray[i].~myClass(); // <-- Note: we need to know what type we are

    /// Deallocate the entire array using "array-delete"
    operator delete[](myArray);



    ///////////////////////////////////////////////////////////////////////////////////////////

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}

