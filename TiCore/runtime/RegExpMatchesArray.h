/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef RegExpMatchesArray_h
#define RegExpMatchesArray_h

#include "TiArray.h"

namespace TI {

    class RegExpMatchesArray : public TiArray {
    public:
        RegExpMatchesArray(TiExcState*, RegExpConstructorPrivate*);
        virtual ~RegExpMatchesArray();

    private:
        virtual bool getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return TiArray::getOwnPropertySlot(exec, propertyName, slot);
        }

        virtual bool getOwnPropertySlot(TiExcState* exec, unsigned propertyName, PropertySlot& slot)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return TiArray::getOwnPropertySlot(exec, propertyName, slot);
        }

        virtual bool getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return TiArray::getOwnPropertyDescriptor(exec, propertyName, descriptor);
        }

        virtual void put(TiExcState* exec, const Identifier& propertyName, TiValue v, PutPropertySlot& slot)
        {
            if (subclassData())
                fillArrayInstance(exec);
            TiArray::put(exec, propertyName, v, slot);
        }

        virtual void put(TiExcState* exec, unsigned propertyName, TiValue v)
        {
            if (subclassData())
                fillArrayInstance(exec);
            TiArray::put(exec, propertyName, v);
        }

        virtual bool deleteProperty(TiExcState* exec, const Identifier& propertyName)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return TiArray::deleteProperty(exec, propertyName);
        }

        virtual bool deleteProperty(TiExcState* exec, unsigned propertyName)
        {
            if (subclassData())
                fillArrayInstance(exec);
            return TiArray::deleteProperty(exec, propertyName);
        }

        virtual void getOwnPropertyNames(TiExcState* exec, PropertyNameArray& arr, EnumerationMode mode = ExcludeDontEnumProperties)
        {
            if (subclassData())
                fillArrayInstance(exec);
            TiArray::getOwnPropertyNames(exec, arr, mode);
        }

        void fillArrayInstance(TiExcState*);
};

}

#endif // RegExpMatchesArray_h
