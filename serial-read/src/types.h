#ifndef __TYPES_H
#define __TYPES_H

#define BUFFER_SIZE 256

#include <string.h>
#include <synapse/SynapseCore/Utils/MathUtils.hpp>


//
template<typename T>
struct Array
{
    size_t capacity = BUFFER_SIZE;
    size_t size = 0;
    T *data = 0;
    T min = static_cast<T>(-FLT_MAX);
    T max = static_cast<T>( FLT_MAX);

    //
    Array()
    {
        data = new T[capacity];
        set(0.0f);
    }

    //
    ~Array() { delete[] data; }

    //
    T last()
    {
        return data[size-1];
    }

    //
    void append(T _val)
    {
        if (size < capacity - 1)
        {
            data[size++] = _val;
        }
        else
        {
            memmove(data, data + 1, sizeof(T) * capacity - 1);
            data[capacity - 1] = _val;
        }

    }

    //
    void limits()
    {
        min = static_cast<T>(-FLT_MAX);
        max = static_cast<T>( FLT_MAX);
        for (size_t i = 0; i < size; i++)
        {
            min = Syn::min(min, data[i]);
            max = Syn::max(max, data[i]);
        }
    }

    //
    void set(T _val)
    {
        for (size_t i = 0; i < capacity; i++)
            append(_val);
    }

};





#endif // __TYPES_H
