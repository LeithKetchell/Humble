/// include some basic string support for test purposes
#include <iostream>
#include <map>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON THIRTEEN:
/// PLACEMENT NEW
/// 
/// The regular new operator does two things:
/// - allocate enough memory for a single object instance
/// - execute a specified constructor method on the new object
/// Regular new operator returns a pointer to a new object...
///
/// Placement new is designed for situations where we already allocated memory
/// and allows us to execute a specified constructor on a GIVEN object.
/// The placement new operator takes a pointer to an existing object!
/// Its very good for "object pools" which are typically "flat arrays"

/// Advantages? 
/// - We can preallocate all our memory upfront as contiguous blocks (reduced cache misses)
/// - Reduced heap memory fragmentation
/// - Runtime allocations cannot fail due to low available system memory 
/// - Objects can be easily and efficiently recycled without being destroyed/recreated

/// Our crash test dummy class has two different constructors...
class myClass {
public:
 myClass(string ugh):something(ugh){ cout << "string constructor: "<<ugh<<endl;}
 myClass(float  ugh):somefloat(ugh){ cout << "float constructor: "<<ugh<<endl;}

 virtual ~myClass() {}
 string something;
 float somefloat;
};



/// Application Entrypoint (for a Console Application)
int main()
{


    /// FLAT ARRAYS
    /// Allocate enough room for three "flat objects"
    /// NOTE: allocating memory for an object won't cause Constructor to be called!
    /// We can use "placement new" to execute constructors of preallocated objects
    myClass* myClassArray = (myClass*) malloc(3 * sizeof(myClass));

    /// Create a working copy of the bufferpointer that we can mess with
    myClass* workPtr = myClassArray;

    /// EXAMPLE: We'll perform a "placement new" on each preallocated object
    ///
    for(int i=0; i<3;i++)
        new (workPtr++) myClass("hello world!");

    /// EXAMPLE: We can RE-INITIALIZE / RE-USE existing objects with placement-new
    for(int i=0; i<3;i++)
        new (workPtr++) myClass(666.6f);

    /// We don't need to "delete" preallocated objects individually
    free(myClassArray);




    ///////////////////////////////////////////////////////////////////////////////////////////

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}

