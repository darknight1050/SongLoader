#pragma once
#include "beatsaber-hook/shared/utils/typedefs.h"

namespace ArrayUtil {

    template <class T>
    inline T* First(Array<T*>* array)
    {
        if(array->Length() > 0)
            return array->values[0];
        return nullptr;
    }

    template <class T>
    inline T* Last(Array<T*>* array)
    {
        if(array->Length() > 0)
            return array->values[array->Length() - 1];
        return nullptr;
    }

    template <class T, class Predicate>
    inline T* First(Array<T*>* array, Predicate pred)
    {
        for (int i = 0; i < array->Length(); i++) {
            T* item = array->values[i];
            if (pred(item)){
                return item;
            }
        }
        return nullptr;
    }
    
    template <class T, class Out, class Predicate>
    inline Array<Out>* Select(Array<T*>* array, Predicate pred)
    {
        Array<Out>* newArray = Array<Out>::NewLength(array->Length());
        for (int i = 0; i < array->Length(); i++) {
            newArray->values[i] = pred(array->values[i]);
        }
        return newArray;
    }

    template <class T, class Predicate>
    inline T* Last(Array<T*>* array, Predicate pred)
    {
        for (int i = array->Length()-1; i >= 0; i--) {
            T* item = array->values[i];
            if (pred(item)){
                return item;
            }
        }
        return nullptr;
    }
    
}