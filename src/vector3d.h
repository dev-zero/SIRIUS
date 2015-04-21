// Copyright (c) 2013-2014 Anton Kozhevnikov, Thomas Schulthess
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that 
// the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the 
//    following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
//    and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED 
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/** \file vector3d.h
 *   
 *  \brief Contains declaration and implementation of vector3d class.
 */

#ifndef __VECTOR3D_H__
#define __VECTOR3D_H__

#include <assert.h>
#include <cmath>
#include <ostream>

/// Simple implementation of 3d vector.
template <typename T> 
class vector3d
{
    private:

        T vec_[3];

    public:
        
        /// Construct zero vector
        vector3d()
        {
            vec_[0] = vec_[1] = vec_[2] = 0;
        }

        /// Construct vector with the same values
        vector3d(T v0)
        {
            vec_[0] = vec_[1] = vec_[2] = v0;
        }

        /// Construct arbitrary vector
        vector3d(T x, T y, T z)
        {
            vec_[0] = x;
            vec_[1] = y;
            vec_[2] = z;
        }

        /// Construct vector from pointer
        vector3d(T const* ptr)
        {
            for (int i = 0; i < 3; i++) vec_[i] = ptr[i];
        }

        /// Access vector elements
        inline T& operator[](const int i)
        {
            assert(i >= 0 && i <= 2);
            return vec_[i];
        }

        inline T const& operator[](const int i) const
        {
            assert(i >= 0 && i <= 2);
            return vec_[i];
        }

        /// Return vector length
        inline double length() const
        {
            return std::sqrt(vec_[0] * vec_[0] + vec_[1] * vec_[1] + vec_[2] * vec_[2]);
        }

        inline vector3d<T> operator+(const vector3d<T>& b)
        {
            vector3d<T> a = *this;
            for (int x = 0; x < 3; x++) a[x] += b.vec_[x];
            return a;
        }

        inline vector3d<T> operator-(const vector3d<T>& b)
        {
            vector3d<T> a = *this;
            for (int x = 0; x < 3; x++) a[x] -= b.vec_[x];
            return a;
        }
        
        template <typename U>
        inline vector3d<T> operator*(U p)
        {
            vector3d<T> a = *this;
            for (int x = 0; x < 3; x++) a[x] *= p;
            return a;
        }
};

template <typename T, typename U>
inline auto operator*(vector3d<T> const a, vector3d<U> const b) -> decltype(a[0] * b[0])
{
    return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

template <typename T>
std::ostream& operator<<(std::ostream &out, vector3d<T>& v)
{
    out << v[0] << " " << v[1] << " " << v[2];
    return out;
}

#endif // __VECTOR3D_H__

