/// include some basic string support for test purposes
#include <iostream>
#include <map>

/// We're too lazy to type this everywhere
using namespace std;

/// LESSON TWELVE:
/// USER BUFFERS : MALLOC AND FREE
/// -

class myClass {
public:
 virtual ~myClass() {}
 string something;
 float somefloat;
};



/// Application Entrypoint (for a Console Application)
int main()
{


    /// Let's allocate a user memory buffer big enough for 100 signed integers
    /// NOTE: malloc (memory alloc) wants the number of BYTES to allocate
    /// We want enough memory for 100 ints: #bytes = #elements * elementsize
    int* myBuffer = (int*) malloc(100 * sizeof(int));

    /// Make a copy of the pointer that we can manipulate without trashing the original
    int* myWorkPointer = myBuffer;

    /// We use "*PointerName" to indicate that we want to acess the data "behind" the pointer..
    /// In this example, we use ++ to increase the value of the pointer by "sizeof (int)" as a "post-operation"
    /// Note that ++ does not add ONE, it adds "sizeof (type)" when used on a typed array buffer
    for(int i=0;i<100;i++)
	*myWorkPointer++ = i;

    /// Reset value of workpointer
    myWorkPointer = myBuffer;

    /// Spew the values to debug console
    for(int i=0;i<100;i++){
	cout << *myWorkPointer++;
	if(i<99)
	    cout << ", ";
    }
    cout << endl;

    /// Since we allocated the memory, it's typically our job to release it when we're done with it...
    free(myBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// FLAT ARRAYS
    /// Enough room for three "flat objects"
    /// myClassArray is a pointer to the first element in an array of, well, structures basically
    myClass* myClassArray = (myClass*) malloc(3 * sizeof(myClass));

    myClassArray[1].somefloat = 666.0f;

    free(myClassArray);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// POINTER ARRAYS
    /// enough room for three object pointers
    /// myClassPointerArray is a pointer to the first element in an array of (myClass*) pointers
    myClass** myClassPointerArray = (myClass**) malloc(3*sizeof(myClass*));

    /// instantiate array members by hand - there is an array new operator to look at, next lesson?...
    /// Note the array operator [] and how its being used
    /// The only difference from previous example is we're being forced to use ->
    for(int i=0;i<3;i++)
	myClassPointerArray[i] = new myClass();

    myClassPointerArray[1]->somefloat = 666.0f;

    /// delete array members by hand - there is an array delete operator...
    for(int i=0;i<3;i++)
	delete myClassPointerArray[i];

    /// free the memory we used to hold the pointers to the objects
    free(myClassPointerArray);

    ///////////////////////////////////////////////////////////////////////////////////////////

    /// Set a breakpoint on line below to "pause" the app so you can see its output
    return 0;
}

