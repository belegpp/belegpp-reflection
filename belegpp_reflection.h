#include <map>
#include <array>
#include <string>
#include <functional>
#include <typeindex>
#include "beleg_any.h"

#define BRF_REGISTRATION(clazz) inline auto clazz##Reflected = []() { using _clazz = clazz; beleg::reflection::classes.add( beleg::reflection::makeReflected<clazz>(#clazz)

#define BRF_CONSTRUCTOR2(n) .constructor<n>()
#define BRF_CONSTRUCTOR3(n, type) .constructor<n, type>()

#define BRF_GETMACRO(_1, _2, NAME, ...) NAME
#define BRF_CONSTRUCTOR(...) BRF_GETMACRO(__VA_ARGS__, BRF_CONSTRUCTOR3, BRF_CONSTRUCTOR2)(__VA_ARGS__)

#define BRF_METHOD(name) .method(#name, &_clazz::name)
#define BRF_PROPERTY(name) .property(#name, &_clazz::name)
#define BRF_END ); return true; }();

namespace beleg
{
	namespace reflection
	{
		template <typename> struct member_function_traits;
		template <typename Return, typename Object, typename... Args>
		struct member_function_traits<Return(Object::*)(Args...)>
		{
			typedef Return return_type;
			typedef Object instance_type;
			typedef Object & instance_reference;

			static constexpr size_t argument_count = sizeof...(Args);
		};
		template<typename>
		struct array_size;
		template<typename T, std::size_t N>
		struct array_size<std::array<T, N> > {
			static size_t const size = N;
		};
		class reflectedMethod
		{
			std::string name;
			std::size_t argCount;
			std::type_index returnType;
			beleg::beleg_any functionPtr;
			beleg::beleg_any simpleCallFunc;
		public:
			reflectedMethod() : returnType(typeid(nullptr)) {};
			template <class C, typename T>
			reflectedMethod(std::string name, T C::*fptr) : returnType(typeid(member_function_traits<decltype(fptr)>::return_type))
			{
				using traits = member_function_traits<decltype(fptr)>;
				this->name = name;
				this->functionPtr = fptr;
				this->argCount = traits::argument_count;

				std::function<beleg::beleg_any(C&, std::array<beleg::beleg_any, traits::argument_count>)> func;
				if constexpr (std::is_same<member_function_traits<decltype(fptr)>::return_type, void>::value)
					func =
					[fptr](auto& obj, std::array<beleg::beleg_any, traits::argument_count> args)
				{
					std::apply([&](auto... argz) { (obj.*fptr)(argz...); }, args);
					return nullptr;
				};
				else
					func =
					[fptr](auto& obj, std::array<beleg::beleg_any, traits::argument_count> args)
				{
					beleg::beleg_any rtn;
					std::apply([&](auto... argz) { rtn = (obj.*fptr)(argz...); }, args);
					return rtn;
				};
				this->simpleCallFunc = func;
			}
			template <typename Object, typename ...Args>
			beleg::beleg_any call(Object& obj, Args... args)
			{
				std::array<beleg::beleg_any, sizeof...(args)> _args = { args... };
				return simpleCallFunc.get<
					std::function<
					beleg::beleg_any(decltype(obj), decltype(_args))
					>
				>()(obj, _args);
			}
			template <typename ReturnType, typename Object, typename ...Args>
			ReturnType call(Object& object, Args... args)
			{
				auto ptr = functionPtr.get<ReturnType(Object::*)(Args...)>();
				return (object.*ptr)(args...);
			}
			template <typename T>
			bool returns()
			{
				return this->returnType == typeid(T);
			}
			std::size_t getArgCount()
			{
				return this->argCount;
			}
			std::string getName()
			{
				return this->name;
			}
		};
		class reflectedProperty
		{
		private:
			std::string name;
			std::type_index type;
			beleg::beleg_any pptr;
		public:
			template <class C, typename T>
			reflectedProperty(std::string name, T C::*ptr) : type(typeid(T))
			{
				pptr = ptr;
				this->name = name;
			}
			template <class C, typename T>
			void set(C& obj, T what)
			{
				auto ptr = pptr.get<T C::*>();
				obj.*ptr = what;
			}
			template <typename T, class C>
			T& get(C& obj)
			{
				auto ptr = pptr.get<T C::*>();
				return (obj.*ptr);
			}
			std::string getName()
			{
				return this->name;
			}
			template<typename T>
			bool is()
			{
				return this->type == typeid(T);
			}
		};
		class reflectedClass
		{
			class methods
			{
			protected:
				std::map<std::string, reflectedMethod> _methods;
			public:
				reflectedMethod getByName(std::string name)
				{
					return _methods.at(name);
				}
				std::vector<reflectedMethod> getAll()
				{
					std::vector<reflectedMethod> rtn;
					for (auto& item : _methods)
					{
						rtn.push_back(item.second);
					}
					return rtn;
				}
				void add(reflectedMethod method)
				{
					this->_methods.insert({ method.getName(), method });
				}
			};
			class properties
			{
			protected:
				std::map<std::string, reflectedProperty> _properties;
			public:
				reflectedProperty getByName(std::string name)
				{
					return _properties.at(name);
				}
				std::vector<reflectedProperty> getAll()
				{
					std::vector<reflectedProperty> rtn;
					for (auto& item : _properties)
					{
						rtn.push_back(item.second);
					}
					return rtn;
				}
				void add(reflectedProperty property)
				{
					this->_properties.insert({ property.getName(), property });
				}
			};
		protected:
			std::string name;
			std::type_index typeInfo;
			std::map<std::size_t, beleg::beleg_any> constructFuncs;

			reflectedClass() : typeInfo(typeid(nullptr)) {}
			reflectedClass(std::string name, std::type_index info) : typeInfo(info)
			{
				this->name = name;
			}
		public:
			methods methods;
			properties properties;

			template <typename ...T>
			beleg::beleg_any create(T... args)
			{
				std::array<beleg::beleg_any, sizeof...(args)> _args = { args... };
				this->constructFuncs.at(sizeof...(args)).get<
					std::function<beleg::beleg_any(
						std::array<beleg::beleg_any, sizeof...(args)>
					)>
				>()(_args);
				return 0;
			}
			std::string getName()
			{
				return name;
			}
		};
		template <class C>
		class makeReflected : public reflectedClass
		{
		public:
			makeReflected(std::string name) : reflectedClass(name, typeid(C)) { }
			template<std::size_t N = 0, std::enable_if_t<N != 1>* = nullptr>
			makeReflected<C>& constructor()
			{
				std::function<beleg::beleg_any(std::array<beleg::beleg_any, N> args)> func = [](std::array<beleg::beleg_any, N> args)
				{
					beleg::beleg_any rtn;
					std::apply([&](auto... argz) { rtn = C(argz...); }, args);
					return rtn;
				};
				this->constructFuncs.insert({ N, func });
				return *this;
			}
			template<std::size_t N = 1, typename T, std::enable_if_t<N == 1>* = nullptr>
			makeReflected<C>& constructor()
			{
				std::function<beleg::beleg_any(std::array<beleg::beleg_any, N> args)> func = [](std::array<beleg::beleg_any, N> args)
				{
					beleg::beleg_any rtn;
					rtn = C((T)std::get<0>(args));
					return rtn;
				};
				this->constructFuncs.insert({ N, func });
				return *this;
			}
			template <typename T>
			makeReflected<C>& method(std::string name, T C::*func)
			{
				this->methods.add(reflectedMethod(name, func));
				return *this;
			}
			template <typename T>
			makeReflected<C>& property(std::string name, T C::*prop)
			{
				this->properties.add(reflectedProperty(name, prop));
				return *this;
			}
		};
		class classes_
		{
		protected:
			std::map<std::string, reflectedClass> _classes;
		public:
			reflectedClass getByName(std::string name)
			{
				return _classes.at(name);
			}
			std::vector<reflectedClass> getAll()
			{
				std::vector<reflectedClass> rtn;
				for (auto& item : _classes)
				{
					rtn.push_back(item.second);
				}
				return rtn;
			}
			void add(reflectedClass clazz)
			{
				this->_classes.insert({ clazz.getName(), clazz });
			}
		};
		inline classes_ classes;
	}
}
