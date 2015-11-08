#ifndef _FACTORY_H_
#define _FACTORY_H_

#define REGISTER_DEF_TYPE(NAME) \
    DerivedRegister<NAME> NAME::reg(#NAME)

template<typename T> evaluation_function * createT() { return new T; }
//template<typename T> BaseFactory * createInstance() { return new T; }

    

typedef std::map<std::string, evaluation_function*(*)()> map_type;


struct BaseFactory {

    static evaluation_function * createInstance(std::string const& s) {
        map_type::iterator it = getMap()->find(s);
        if(it == getMap()->end())
            return 0;
        return it->second();
    }

public:
    static map_type * getMap() {
        // never delete'ed. (exist until program termination)
        // because we can't guarantee correct destruction order 
        if(!map) { map = new map_type; } 
        return map; 
    }

private:
    static map_type* map;


};




template<typename T>
struct DerivedRegister : BaseFactory { 
    DerivedRegister(std::string const& s) { 
        getMap()->insert(std::make_pair(s, &createT<T>));
    }
};




#endif
