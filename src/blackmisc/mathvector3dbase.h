/*  Copyright (C) 2013 VATSIM Community / contributors
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLACKMISC_MATHVECTOR3DBASE_H
#define BLACKMISC_MATHVECTOR3DBASE_H

#include "blackmisc/basestreamstringifier.h"
#include "blackmisc/mathematics.h"

namespace BlackMisc
{
namespace Math
{

class CMatrix3x3; // forward declaration
class CMatrix3x1; // forward declaration


/*!
 * \brief 3D vector base (x, y, z)
 */
template <class ImplClass> class CVector3DBase : public CBaseStreamStringifier<ImplClass>
{
protected:

    // using own value since Qt QVector3D stores internally as float
    double m_i; //!< Vector data i
    double m_j; //!< Vector data j
    double m_k; //!< Vector data k

    /*!
     * \brief Default constructor
     */
    CVector3DBase() : m_i(0.0), m_j(0.0), m_k(0.0) {}

    /*!
     * \brief Constructor by values
     * \param i
     * \param j
     * \param k
     */
    CVector3DBase(double i, double j, double k) : m_i(i), m_j(j), m_k(k) {}

    /*!
     * \brief Constructor by value
     * \param value
     */
    explicit CVector3DBase(double value) : m_i(value), m_j(value), m_k(value) {}

    /*!
     * \brief Copy constructor
     * \param otherVector
     */
    CVector3DBase(const CVector3DBase &otherVector) : m_i(otherVector.m_i), m_j(otherVector.m_j), m_k(otherVector.m_k) {}

    /*!
     * \brief String for converter
     * \return
     */
    virtual QString stringForConverter() const;

public:

    // getter and setters are implemented in the derived classes
    // as they have different names (x, i, north)

    /*!
     * \brief Virtual destructor
     */
    virtual ~CVector3DBase() {}

    /*!
     * \brief Set zeros
     */
    void setZero();

    /*!
     * \brief Set zeros
     */
    bool isZero() const
    {
        return this->m_i == 0 && this->m_j == 0 && this->m_k == 0;
    }

    /*!
     * \brief Is identity matrix? Epsilon considered.
     * \return
     */
    bool isZeroEpsilon() const
    {
        ImplClass v;
        v += (*this);
        v.round();
        return v.isZero();
    }

    /*!
     * \brief Set zeros
     */
    void fill(double value);

    /*!
     * \brief Get element
     * \param row
     * \return
     */
    double getElement(size_t row) const;

    /*!
     * \brief Set element
     * \param row
     * \param value
     */
    void setElement(size_t row, double value);

    /*!
     * \brief Operator []
     * \param row
     * \return
     */
    double operator[](size_t row) const { return this->getElement(row); }


    /*!
     * \brief Get row element by ()
     * \param row
     * \return
     */
    double operator()(size_t row) const { return this->getElement(row); }

    /*!
     * \brief Equal operator ==
     * \param otherVector
     * \return
     */
    bool operator ==(const CVector3DBase &otherVector) const
    {
        if (this == &otherVector) return true;
        return this->m_i == otherVector.m_i &&
               this->m_j == otherVector.m_j &&
               this->m_k == otherVector.m_k;
    }

    /*!
     * \brief Unequal operator !=
     * \param otherVector
     * \return
     */
    bool operator !=(const CVector3DBase &otherVector) const
    {
        if (this == &otherVector) return false;
        return !((*this) == otherVector);
    }

    /*!
     * \brief Assigment operator =
     * \param otherVector
     * \return
     */
    CVector3DBase &operator =(const CVector3DBase &otherVector)
    {
        if (this == &otherVector) return *this; // Same object?
        this->m_i = otherVector.m_i;
        this->m_j = otherVector.m_j;
        this->m_k = otherVector.m_k;
        return (*this);
    }

    /*!
     * \brief Operator +=
     * \param otherVector
     * \return
     */
    CVector3DBase &operator +=(const CVector3DBase &otherVector)
    {
        this->m_i += otherVector.m_i;
        this->m_j += otherVector.m_j;
        this->m_k += otherVector.m_k;
        return (*this);
    }

    /*!
     * \brief Operator +
     * \param otherVector
     * \return
     */
    ImplClass operator +(const ImplClass &otherVector) const
    {
        ImplClass v;
        v += (*this);
        v += otherVector;
        return v;
    }

    /*!
     * \brief Operator -=
     * \param otherVector
     * \return
     */
    CVector3DBase &operator -=(const CVector3DBase &otherVector)
    {
        this->m_i -= otherVector.m_i;
        this->m_j -= otherVector.m_j;
        this->m_k -= otherVector.m_k;
        return (*this);
    }

    /*!
     * \brief Operator -
     * \param otherVector
     * \return
     */
    ImplClass operator -(const ImplClass &otherVector) const
    {
        ImplClass v;
        v += (*this);
        v -= otherVector;
        return v;
    }

    /*!
     * \brief Operator *=, just x*x, y*y, z*z neither vector nor dot product (like a matrix produc)
     * \param otherVector
     * \return
     */
    CVector3DBase &operator *=(const CVector3DBase &otherVector)
    {
        this->m_i *= otherVector.m_i;
        this->m_j *= otherVector.m_j;
        this->m_k *= otherVector.m_k;
        return (*this);
    }

    /*!
     * \brief Operator, just x*x, y*y, z*z neither vector nor dot product, (like a matrix produc)
     * \param otherVector
     * \return
     */
    ImplClass operator *(const ImplClass &otherVector) const
    {
        ImplClass v;
        v += (*this);
        v *= otherVector;
        return v;
    }

    /*!
     * \brief Multiply with scalar
     * \param factor
     * \return
     */
    CVector3DBase &operator *=(double factor)
    {
        this->m_i *= factor;
        this->m_j *= factor;
        this->m_k *= factor;
        return (*this);
    }

    /*!
     * \brief Multiply with scalar
     * \param factor
     * \return
     */
    ImplClass operator *(double factor) const
    {
        ImplClass v;
        v += (*this);
        v *= factor;
        return v;
    }

    /*!
     * \brief Operator to support commutative multiplication
     * \param factor
     * \param otherVector
     * \return
     */
    friend ImplClass operator *(double factor, const ImplClass &otherVector)
    {
        return otherVector * factor;
    }

    /*!
     * \brief Divide by scalar
     * \param divisor
     * \return
     */
    CVector3DBase &operator /=(double divisor)
    {
        this->m_i /= divisor;
        this->m_j /= divisor;
        this->m_k /= divisor;
        return (*this);
    }

    /*!
     * \brief Divide by scalar
     * \param divisor
     * \return
     */
    ImplClass operator /(double divisor) const
    {
        ImplClass v;
        v += (*this);
        v /= divisor;
        return v;
    }

    /*!
     * \brief Operator /=, just x/x, y/y, z/z
     * \param otherVector
     * \return
     */
    CVector3DBase &operator /=(const CVector3DBase &otherVector)
    {
        this->m_i /= otherVector.m_i;
        this->m_j /= otherVector.m_j;
        this->m_k /= otherVector.m_k;
        return (*this);
    }

    /*!
     * \brief Operator, just x/x, y/y, z/z
     * \param otherVector
     * \return
     */
    ImplClass operator /(const ImplClass &otherVector) const
    {
        ImplClass v;
        v += (*this);
        v /= otherVector;
        return v;
    }

    /*!
     * \brief Dot product
     * \param otherVector
     * \return
     */
    double dotProduct(const ImplClass &otherVector) const;

    /*!
     * \brief Cross product
     * \param otherVector
     * \return
     */
    ImplClass crossProduct(const ImplClass &otherVector) const;

    /*!
     * \brief Matrix * this vector
     * \param matrix
     * \return
     */
    void matrixMultiplication(const CMatrix3x3 &matrix);

    /*!
     * \brief Reciprocal value
     * \return
     */
    ImplClass reciprocalValues() const
    {
        ImplClass v;
        v.m_i = (1 / this->m_i);
        v.m_j = (1 / this->m_j);
        v.m_k = (1 / this->m_j);
        return v;
    }

    /*!
     * \brief Length
     * \return
     */
    double length()const
    {
        return this->m_i * this->m_j + this->m_k;
    }

    /*!
     * \brief Length squared
     * \return
     */
    double lengthSquared()const
    {
        return this->m_i * this->m_i + this->m_j * this->m_j + this->m_k * this->m_k;
    }

    /*!
     * \brief Converted to matrix
     * \return
     */
    CMatrix3x1 toMatrix3x1() const;

    /*!
     * \brief Magnitude
     * \return
     */
    double magnitude() const
    {
        return sqrt(this->lengthSquared());
    }

    /*!
     * \brief Round this vector
     * \return
     */
    ImplClass &round()
    {
        const double epsilon = 1E-10;
        this->m_i = BlackMisc::Math::CMath::roundEpsilon(this->m_i, epsilon);
        this->m_j = BlackMisc::Math::CMath::roundEpsilon(this->m_j, epsilon);
        this->m_k = BlackMisc::Math::CMath::roundEpsilon(this->m_k, epsilon);
        return static_cast<ImplClass &>(*this);
    }
};

} // namespace

} // namespace

#endif // guard
