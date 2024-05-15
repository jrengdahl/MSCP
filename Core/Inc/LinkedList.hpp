////////////////////////////////////////////////////////////////////////////////
// LinkedList.hpp
// A FIFO linked list.
// This version is not lock-free, and may need to be protected by a critical region.
//
// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file
//
///////////////////////////////////////////////////////////////////////////////


#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include "cmsis.h"


///////////////////////////////////////////////////////////////////////////////
// class LinkedList
//
// template parameter T      - Type of data this FIFO will hold.
// template parameter mpNext - member pointer to the "next" field to use for the link
//
// Example: LinkedList<foo, &foo::next> list;
/////////////////////////////////////////////////////////////////////////////

template<typename T, T *T::*mpNext>
class LinkedList
    {
    public:

    T *head = 0;
    T **tail = &head;

    ///////////////////////////////////////////////////////////////////////////////
    //  LinkedList::operator bool
    //
    // returns true if list has any content
    ///////////////////////////////////////////////////////////////////////////////

    inline operator bool() { return head != 0; }


    ///////////////////////////////////////////////////////////////////////////////
    //  LinkedList::add
    //
    // Adds an entry to the list.
    //
    // input: value  The value to be added.
    ///////////////////////////////////////////////////////////////////////////////

    void add(T *value)
        {
        value->*mpNext = 0;
        *tail = value;
        tail = &(value->*mpNext);
        }


    /////////////////////////////////////////////////////////////////////////////
    //  LinkedList::take
    //
    // Takes an entry from the list.
    //
    // arg: reference to where to put the value taken
    // return: true if a value was taken, false if the list was empty
    ////////////////////////////////////////////////////////////////////////////

    bool take(T *&value)
        {
        T *x;

        x = head;
        if(x == 0)return false;
        head = x->*mpNext;
        if(head == 0)
            {
            tail = &head;
            }
        x->*mpNext = 0;
        value = x;
        return true;
        }


    // reinitialize the list
    void init()
        {
        head = 0;
        tail = &head;
        }

    };



#endif // LINKEDLIST_HPP
