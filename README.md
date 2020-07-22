# Belegpp-Reflection

[![Last Commit](https://img.shields.io/github/last-commit/belegpp/belegpp-reflection?style=for-the-badge)](https://github.com/belegpp/belegpp-reflection/commits)
[![License](https://img.shields.io/github/license/belegpp/belegpp-reflection?style=for-the-badge)](https://github.com/belegpp/belegpp-reflection/blob/master/LICENSE)

## What is this?
This is a header-only c++17 reflection library.

## Dependencies
This library depends on the single-header library [beleg-any]([https://github.com/belegpp/belegpp-any](https://github.com/belegpp/belegpp-any)).

## How to use
### Example Class
```cpp
struct ReflectionTest
{
	int a;
	
	ReflectionTest()
	{
		std::cout << "Constructor called!" << std::endl;
	}
	ReflectionTest(int a)
	{
		std::cout << "Constructor called with: " << a << std::endl;
		this->a = a;
	}
	ReflectionTest(int a, int b)
	{
		std::cout << "Constructor called with: " << a << std::endl;
		this->a = a;
	}
	void test()
	{
		std::cout << "Test called!" << std::endl;
	}
	bool testWithRtn(int a)
	{
		std::cout << "TestWithRtn called with " << a << "!" << std::endl;
		return a > 0;
	}
};
```
### Making the class reflectable
There are two ways to make your class reflectable, either by using the provided macros or by doing what the macros do yourself.

Using macros (recommended):
```cpp
// Anywhere outside of a function or class:
BRF_REGISTRATION(ReflectionTest)
BRF_CONSTRUCTOR(2/*2 is the numbers of parameters required by the constructor*/)
BRF_CONSTRUCTOR(1, int/*The constructor requires one parameter, so we need to specify the data-type of the arugment that is required - this is only required for constructors that have one parameter.*/)
BRF_CONSTRUCTOR() /*Default constructor*/
BRF_METHOD(test)
BRF_METHOD(testWithRtn)
BRF_PROPERTY(a)
BRF_END
```

Not using macros:
```cpp
// Anywhere inside of a function
beleg::reflection::classes.add(
	beleg::reflection::makeReflected<ReflectionTest>("ReflectionTest")
	.constructor<>()
	.constructor<1, int>()
	.constructor<2>()
	.method("test", &ReflectionTest::test)
	.method("testWithRtn", &ReflectionTest::testWithRtn)
	.property("a", &ReflectionTest::a)
);
```

### Creating a class instance
```cpp
auto reflectionTest = beleg::reflection::classes.getByName("ReflectionTest");
auto classInstance = reflectionTest.create(); //-> Will call the default constructor
auto classInstance = reflectionTest.create(10); //-> Will call the constructor that takes one argument
auto classInstance = reflectionTest.create(10, 10); //-> Will call the constructor that takes two arguments
```

### Accessing Methods / Properties | Working with an instance
```cpp
auto reflectionTest = beleg::reflection::classes.getByName("ReflectionTest");
auto classInstance = reflectionTest.create(); /*or*/ ReflectionTest classInstance;

for (auto meth : reflectionTest.methods.getAll()) //Iterate over all class methods
{
	auto name = meth.getName(); //Get the methods name
	auto argCount = meth.getArgCount(); //How many arguments the method takes
	bool returnsInt = meth.returns<int>(); //Check the return type of the function
	beleg::beleg_any returnVal = meth.call(classInstance, /*params*/); //Can hold any value
	bool definedReturnVal = meth.call<bool>(classInstance, /*params*/);
}

for (auto property : reflectionTest.properties.getAll()) //Iterate over all class properties
{
	auto name = property.getName(); //Get the property name
	int value = property.get<int>(classInstance); //Get the property as an int
	bool isInt = property.is<int>();
	property.set(classInstance, 1337); //Set the value of the property.
}

auto testWithRtnFunc = reflectionTest.methods.getByName("testWithRtn"); //Get function by name
beleg::beleg_any result = testWithRtnFunc.call(classInstance, 10); //Call the function and store the return value as beleg::beleg_any

auto propertyA = reflectionTest.properties.getByName("a");
propertyA.set(classInstance, 10);
int aValue = propertyA.get<int>(classInstance);
```