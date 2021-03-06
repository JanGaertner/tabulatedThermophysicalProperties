/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2018 Yuusha and tilasoldo
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of tilasoldo and Yuusha contribution to OpenFOAM.
    It is based on chriss85 contribution for OpenFOAM 2.3.x.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "IFstream.H"
#include "openFoamTableReader.H"
#include "Vector.H"

// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

template<class Type>
void Foam::extrapolation2DTable<Type>::readTable()
{
    fileName fName(fileName_);
    fName.expand();

    // Read data from file
    reader_()(fName, *this);

    if (this->empty())
    {
        FatalErrorIn
        (
            "Foam::extrapolation2DTable<Type>::readTable()"
	)   << "table read from " << fName << " is empty" << nl
            << exit(FatalError);
    }

    // Check that the data are in ascending order
    checkOrder();
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::extrapolation2DTable<Type>::extrapolation2DTable()
:
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > >(),
    boundsHandling_(extrapolation2DTable::WARN),
    fileName_("fileNameIsUndefined"),
    reader_(NULL),
    isNull_(true)
{}


template<class Type>
Foam::extrapolation2DTable<Type>::extrapolation2DTable
(
    const List<Tuple2<scalar, List<Tuple2<scalar, Type> > > >& values,
    const boundsHandling bounds,
    const fileName& fName,
    const Switch isNull
)
:
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > >(values),
    boundsHandling_(bounds),
    fileName_(fName),
    reader_(NULL),
    isNull_(isNull)
{}


template<class Type>
Foam::extrapolation2DTable<Type>::extrapolation2DTable(const fileName& fName)
:
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > >(),
    boundsHandling_(extrapolation2DTable::WARN),
    fileName_(fName),
    reader_(new openFoamTableReader<Type>(dictionary())),
    isNull_(false)
{
    readTable();
}


template<class Type>
Foam::extrapolation2DTable<Type>::extrapolation2DTable(const dictionary& dict)
:
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > >(),
    boundsHandling_(wordToBoundsHandling(dict.lookup("outOfBounds"))),
    fileName_(dict.lookup("fileName")),
    reader_(tableReader<Type>::New(dict)),
    isNull_(false)
{
    readTable();
}


template<class Type>
Foam::extrapolation2DTable<Type>::extrapolation2DTable
(
     const extrapolation2DTable& extrapTable
)
:
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > >(extrapTable),
    boundsHandling_(extrapTable.boundsHandling_),
    fileName_(extrapTable.fileName_),
    reader_(extrapTable.reader_),    // note: steals reader. Used in write().
    isNull_(extrapTable.isNull_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Type Foam::extrapolation2DTable<Type>::extrapolateValue
(
    const List<Tuple2<scalar, Type> >& data,
    const scalar lookupValue
) const
{
    label n = data.size();

    scalar minLimit = data.first().first();
    scalar maxLimit = data.last().first();
    if (n == 1)
    {
	return data.first().second();
    }
    if (lookupValue < minLimit)
    {
        switch (boundsHandling_)
	{
	    case extrapolation2DTable::ERROR:
            {
                FatalErrorIn
                (
                    "Foam::extrapolation2DTable<Type>::extrapolateValue"
                    "("
                        "List<Tuple2<scalar, Type> >&, "
                        "const scalar"
                    ")"
                )
		    << "value (" << lookupValue << ") less than lower "
                    << "bound (" << minLimit << ")" << nl
                    << exit(FatalError);
                break;
            }
            case extrapolation2DTable::WARN:
            {
                WarningIn
                (
                    "Foam::extrapolation2DTable<Type>::extrapolateValue"
                    "("
                        "List<Tuple2<scalar, Type> >&, "
                        "const scalar"
                    ")"
                )
		    << "value (" << lookupValue << ") less than lower "
                    << "bound (" << minLimit << ")" << nl
                    << "    extrapolating the first entry"
                    << endl;
                // fall-through to 'EXTRAPOLATE'
            }
            case extrapolation2DTable::EXTRAPOLATE:
            {
		scalar x1 = minLimit;
		scalar x2 = data[1].first();
		Type y1 = data[0].second();
		Type y2 = data[1].second();
		//extrapolation
                return y1 + (lookupValue - x1)/(x2 - x1)*(y2 - y1);
                break;
            }
        }
    }
    else if (lookupValue > maxLimit)
    {
        switch (boundsHandling_)
        {
	    case extrapolation2DTable::ERROR:
            {
                FatalErrorIn
                (
                    "Foam::extrapolation2DTable<Type>::extrapolateValue"
                    "("
                        "List<Tuple2<scalar, Type> >&, "
                        "const scalar"
                    ")"
                )
		    << "value (" << lookupValue << ") greater than upper "
                    << "bound (" << maxLimit << ")" << nl
                    << exit(FatalError);
                break;
            }
            case extrapolation2DTable::WARN:
            {
                WarningIn
                (
                    "Foam::extrapolation2DTable<Type>::extrapolateValue"
                    "("
                        "List<Tuple2<scalar, Type> >&, "
                        "const scalar"
                    ")"
                )
		    << "value (" << lookupValue << ") greater than upper "
                    << "bound (" << maxLimit << ")" << nl
                    << "    Continuing with the last entry"
                    << endl;
                // fall-through to 'EXTRAPOLATE'
            }
            case extrapolation2DTable::EXTRAPOLATE:
            {
		 scalar x1 = maxLimit;
		 scalar x2 = data[data.size() - 2].first();
		 Type y1 = data[data.size() - 1].second();
		 Type y2 = data[data.size() - 2].second();
		 //extrapolation
		 return y1 + (lookupValue - x1)/(x2 - x1)*(y2 - y1);
		 return data.last().second();
		 break;
            }
        }
    }

    // look for the correct range in X
    label lo = 0;
    label hi = 0;

    for (label i = 0; i < n; ++i)
    {
        if (lookupValue >= data[i].first())
        {
            lo = hi = i;
        }
        else
        {
            hi = i;
            break;
        }
    }

    if (lo == hi)
    {
        return data[lo].second();
    }
    else
    {
        Type m =
            (data[hi].second() - data[lo].second())
	   /(data[hi].first() - data[lo].first());

        // normal interpolation
        return data[lo].second() + m*(lookupValue - data[lo].first());
    }
}


template<class Type>
template<class L>
Foam::label Foam::extrapolation2DTable<Type>::Xi
(
    const List<Tuple2<scalar, L> >& t,
    const scalar valueX,
    const bool reverse
) const
{

    label nX = t.size();
    if (reverse)
    {
	for (int i = nX - 1; i > 0; --i)
	{
	    if (t[i].first() <= valueX)
	    {
		if(i == nX - 1) //point lies outwards, return last index
		{
		    return nX - 1;
		}
                else
		{
		    return i + 1;
		}
	    }
	}
	return 1; //point lies outwards, return 2nd index
    }
    else
    {
	for (int i = 0; i < nX - 1; ++i)
	{
	    if (t[i].first() > valueX)
	    {
		if (i == 0)
		{
		    return 0; //point lies outwards, return first index
		}
		else
		{
		    return i - 1;
		}
	    }
	}
	return nX - 2; //point lies outwards, return second last index
    }
}


/* I'm sure it's not very OpenFOAM/C++ style. It has to be improved.
   But it works.*/
template<class Type>
Type Foam::extrapolation2DTable<Type>::Tderivative
(
    const scalar valueX,
    const scalar valueY
) const
{
    const table& t = *this;

    if (t.size() <= 1)
    {
	WarningIn
        (
            "Type Foam::extrapolation2DTable<Type>::Tderivative"
            "("
	        "const scalar, "
  	        "const scalar"
            ") const"
        )
            << "cannot derivate a zero- or one-sized table - returning zero"
	    << endl;
	return pTraits<Type>::zero;
    }

    // have 2-D data, extrapolate
    List<Tuple2<scalar, Type> > row0;
    List<Tuple2<scalar, Type> > row1;
    // find low and high indices in the X range that bound valueX
    label x0i = Xi(t, valueX, false);
    label x1i = Xi(t, valueX, true);
    row0 = t[x0i].second();
    row1 = t[x1i].second();
    //factor for interpolating between both rows
    scalar factor = (valueX - t[x0i].first())/(t[x1i].first() - t[x0i].first());
    if (t.first().second().size() == 1)
    {
	return (row1.first().second() - row0.first().second())
	      /(t[x1i].first() - t[x0i].first());
    }
    // find low and high indices in the Y range that bound valueY
    label y00 = Xi(row0, valueY, false);
    label y01 = Xi(row0, valueY, true);
    label y10 = Xi(row1, valueY, false);
    label y11 = Xi(row1, valueY, true);

    label ymin = min(y00, y10);
    label ymax = max(y01, y11);
    List<vector> points;
    label index = 0;
    for (label i = ymin; i <= ymax; ++i)
    {
	points.append(vector(
			     row0[i].second(),
			     row0[i].first()  + factor
			    *(row1[i].first()  - row0[i].first() ),
			     row1[i].second()
			     ));
	if (points[i - ymin].y() < valueY)
	{
	    index = i - ymin;
	}
    }
    return (points[index + 1].z() - points[index].x())
	  /(t[x1i].first() - t[x0i].first());
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class Type>
Type Foam::extrapolation2DTable<Type>::operator()
(
    const scalar valueX,
    const scalar valueY
) const
{

    // Considers all of the list in Y being equal
    label nX = this->size();
    label nY = this->first().second().size();
    const table& t = *this;
    if (nX == 0)
    {
        WarningIn
        (
            "Type Foam::extrapolation2DTable<Type>::operator()"
            "("
                "const scalar, "
                "const scalar"
            ") const"
        )
            << "cannot extrapolate a zero-sized table - returning zero" << endl;

        return pTraits<Type>::zero;
    }
    else if (nX == 1)
    {
        // only 1 row (in X) - extrapolate to find Y value
        return extrapolateValue(t.first().second(), valueY);
    }
    else
    {
	/* have 2-D data, extrapolate
	   double minRowLimit = t.first().first();
	   double maxRowLimit = t.last().first();
	   idea: use plane-based inter/extrapolation,
	   find the 4 points which surround the wanted point first*/
	List<Tuple2<scalar, Type> > row0;
	List<Tuple2<scalar, Type> > row1;
	// find low and high indices in the X range that bound valueX
	label x0i = Xi(t, valueX, false);
	label x1i = Xi(t, valueX, true);
	row0 = t[x0i].second();
	row1 = t[x1i].second();
	//factor for interpolating between both rows
	scalar factor =
	    (valueX - t[x0i].first())/(t[x1i].first() - t[x0i].first());
	if (nY == 1)
        {
	    return row0.first().second() + factor
		  *(row1.first().second() - row0.first().second());
	}
	//Now we have the data for the first axis.

	// find low and high indices in the Y range that bound valueY
	label y00 = Xi(row0, valueY, false);
	label y01 = Xi(row0, valueY, true);
	label y10 = Xi(row1, valueY, false);
	label y11 = Xi(row1, valueY, true);
	label ymin = min(y00, y10);
	label ymax = max(y01, y11);
	List<vector> points;
	label index = 0;
	for (label i = ymin; i <= ymax; i++)
	{
	    points.append(vector(
				 valueX,
				 row0[i].first()  + factor*(row1[i].first()  - row0[i].first() ),
				 row0[i].second() + factor*(row1[i].second() - row0[i].second())
				 ));
	    if (points[i - ymin].y() < valueY)
	    {
		index = i - ymin;
	    }
	}
	return (points[index] + (valueY - points[index].y())
	      /(points[index + 1].y() - points[index].y())
	      *(points[index + 1] - points[index])).z();
    }
}


template<class Type>
Foam::word Foam::extrapolation2DTable<Type>::boundsHandlingToWord
(
     const boundsHandling& bound
) const
{
    word enumName("warn");

    switch (bound)
    {
        case extrapolation2DTable::ERROR:
        {
            enumName = "error";
            break;
        }
        case extrapolation2DTable::WARN:
        {
            enumName = "warn";
            break;
        }
        case extrapolation2DTable::EXTRAPOLATE:
        {
            enumName = "extrapolate";
            break;
        }
    }

    return enumName;
}


template<class Type>
typename Foam::extrapolation2DTable<Type>::boundsHandling
Foam::extrapolation2DTable<Type>::wordToBoundsHandling
(
    const word& bound
) const
{
    if (bound == "error")
    {
        return extrapolation2DTable::ERROR;
    }
    else if (bound == "warn")
    {
        return extrapolation2DTable::WARN;
    }
    else if (bound == "extrapolate")
    {
        return extrapolation2DTable::EXTRAPOLATE;
    }
    else
    {
        WarningIn
        (
            "Foam::extrapolation2DTable<Type>::wordToBoundsHandling"
            "("
            "    const word&"
            ")"
        )
	    << "bad outOfBounds specifier " << bound << " using 'warn'" << endl;

        return extrapolation2DTable::WARN;
    }
}


template<class Type>
inline Foam::extrapolation2DTable<Type>&
Foam::extrapolation2DTable<Type>::operator=
(
    const extrapolation2DTable<Type>& et
)
{
    boundsHandling_ = et.boundsHandling_;
    fileName_ = et.fileName_;
    reader_ = et.reader_;
    isNull_ = et.isNull_;

    return *this;
}


template<class Type>
inline Foam::extrapolation2DTable<Type> Foam::operator+
(
    const extrapolation2DTable<Type>& et1,
    const extrapolation2DTable<Type>& et2
)
{
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > > etn = et1, ett = et2;

    if (et1.size() != et2.size())
    {
	FatalErrorInFunction
            << "attempt to sum list with different sizes."
            << abort(FatalError);
    }

    if (et1.isNull_)
    {
    	return extrapolation2DTable<Type>
        (
            ett,
    	    et2.boundsHandling_,
    	    et2.fileName_,
    	    et2.isNull_
        );
    }
    else if (et2.isNull_)
    {
    	return extrapolation2DTable<Type>
        (
            etn,
    	    et1.boundsHandling_,
    	    et1.fileName_,
    	    et1.isNull_
        );
    }

    for (int i = 0 ; i < etn.size() ; ++i)
    {
	for (int j = 0 ; j < etn[i].second().size() ; ++j)
	{
	    etn[i].second()[j].second() += ett[i].second()[j].second();
	}
    }

    return extrapolation2DTable<Type>
    (
	etn,
	et1.boundsHandling_,
	et1.fileName_,
	false
    );
}


template<class Type>
inline Foam::extrapolation2DTable<Type> Foam::operator-
(
    const extrapolation2DTable<Type>& et1,
    const extrapolation2DTable<Type>& et2
)
{
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > > etn = et1, ett = et2;

    if (et1.size() != et2.size())
    {
	FatalErrorInFunction
            << "attempt to sum list with different sizes."
            << abort(FatalError);
    }

    for (int i = 0 ; i < etn.size() ; ++i)
    {
	for (int j = 0 ; j < etn[i].second().size() ; ++j)
	{
	    etn[i].second()[j].second() -= ett[i].second()[j].second();
	}
    }

    return extrapolation2DTable<Type>
    (
	 etn,
	 et1.boundsHandling_,
	 et1.fileName_,
	 et1.isNull_
    );
}


template<class Type>
inline Foam::extrapolation2DTable<Type> Foam::operator*
(
    const scalar s,
    const extrapolation2DTable<Type>& et
)
{
    List<Tuple2<scalar, List<Tuple2<scalar, Type> > > > etn = et;

    // I'm not sure that it saves time but I try it.
    if (s == 1 || et.isNull_)
    {
    	return extrapolation2DTable<Type>
        (
    	    etn,
    	    et.boundsHandling_,
    	    et.fileName_,
	    et.isNull_
        );
    }
    else
    {
    	for (int i = 0 ; i < etn.size() ; ++i)
    	{
    	    for (int j = 0 ; j < etn[i].second().size() ; ++j)
    	    {
    		etn[i].second()[j].second() *= s;
    	    }
    	}
    }

    if (s == 0)
    {
    	return extrapolation2DTable<Type>
    	(
    	    etn,
    	    et.boundsHandling_,
    	    et.fileName_,
    	    true
    	);
    }
    else
    {
	return extrapolation2DTable<Type>
	(
	    etn,
	    et.boundsHandling_,
	    et.fileName_,
	    et.isNull_
	);
    }
}


template<class Type>
typename Foam::extrapolation2DTable<Type>::boundsHandling
Foam::extrapolation2DTable<Type>::outOfBounds
(
    const boundsHandling& bound
)
{
    boundsHandling prev = boundsHandling_;
    boundsHandling_ = bound;
    return prev;
}


template<class Type>
void Foam::extrapolation2DTable<Type>::checkOrder() const
{
    label n = this->size();
    const table& t = *this;

    scalar prevValue = t[0].first();

    for (label i=1; i<n; ++i)
    {
        const scalar currValue = t[i].first();

        // avoid duplicate values (divide-by-zero error)
        if (currValue <= prevValue)
        {
            FatalErrorIn
            (
                "Foam::extrapolation2DTable<Type>::checkOrder() const"
            )
		<< "out-of-order value: "
                << currValue << " at index " << i << nl
                << exit(FatalError);
        }
        prevValue = currValue;
    }
}


template<class Type>
void Foam::extrapolation2DTable<Type>::write(Ostream& os) const
{
    os.writeKeyword("fileName")
        << fileName_ << token::END_STATEMENT << nl;
    os.writeKeyword("outOfBounds")
        << boundsHandlingToWord(boundsHandling_) << token::END_STATEMENT << nl;

    *this >> os;
}


// ************************************************************************* //
