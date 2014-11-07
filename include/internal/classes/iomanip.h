/*
 * iomanip.h
 *
 *  Created on: Nov 6, 2014
 *      Author: vlad
 */

#ifndef INCLUDE_INTERNAL_CLASSES_IOMANIP_H_
#define INCLUDE_INTERNAL_CLASSES_IOMANIP_H_

namespace qpp
{

// ostream manipulator
template<typename T, typename InputIterator>
class IOManip
{
	enum class Type
	{
		EIGEN_MATRIX = 1, 	//!< Eigen Matrix or complex number
		/*!< Eigen matrix or complex number */
		RANGE,				//!< Range
		/*!< Range specified by begin iterator and end iterator */
		C_POINTER 			//!< C-style pointer
	/*!< C-style <type>* of given size */
	};
	Type _type { };
	cmat _A { };
	double _chop { };
	InputIterator _first { }, _last { };
	const T* _p { };
	std::size_t _n { };
	std::string _separator { }, _start { }, _end { };
public:
	// Eigen matrices
	template<typename Derived>
	explicit IOManip(const Eigen::MatrixBase<Derived>& A, double chop =
			qpp::chop) :
			_type(Type::EIGEN_MATRIX), _A(A.template cast<cplx>()), _chop(chop)
	{
	}
	// Complex numbers
	explicit IOManip(const cplx z, double chop = qpp::chop) :
			_chop(chop), _type(Type::EIGEN_MATRIX)
	{
		// put the complex number inside an Eigen matrix
		_A.resize(1, 1);
		_A(0, 0) = z;
	}
	// Range
	explicit IOManip(InputIterator first, InputIterator last,
			const std::string & separator, const std::string& start = "[",
			const std::string& end = "]") :
			_type(Type::RANGE), _first(first), _last(last), _separator(
					separator), _start(start), _end(end)
	{
	}
	// C-style pointer
	explicit IOManip(const T* p, const std::size_t n,
			const std::string & separator, const std::string& start = "[",
			const std::string& end = "]") :
			_type(Type::C_POINTER), _p(p), _n(n), _separator(separator), _start(
					start), _end(end)
	{
	}

	// to silence -Weffc++ warnings
	IOManip(const IOManip&)
	{
	}
	IOManip& operator=(const IOManip&)
	{
		return *this;
	}

	template<typename charT, typename traits>
	friend std::basic_ostream<charT, traits>&
	operator<<(std::basic_ostream<charT, traits>& os, const IOManip& rhs)
	{
		switch (rhs._type)
		{
		case IOManip::Type::EIGEN_MATRIX:
		{
			if (rhs._A.size() == 0)
			{
				os << "Empty [" << rhs._A.rows() << " x " << rhs._A.cols()
						<< "] matrix";
				return os;
			};

			std::ostringstream ostr;
			ostr.copyfmt(os); // copy os' state

			std::vector<std::string> vstr;
			std::string strA { };

			for (std::size_t i = 0; i < static_cast<std::size_t>(rhs._A.rows());
					i++)
			{
				for (std::size_t j = 0;
						j < static_cast<std::size_t>(rhs._A.cols()); j++)
				{
					strA.clear(); // clear the temporary string
					ostr.clear();
					ostr.str(std::string { }); // clear the ostringstream

					// convert to complex
					double re = static_cast<cplx>(rhs._A(i, j)).real();
					double im = static_cast<cplx>(rhs._A(i, j)).imag();

					if (std::abs(re) < rhs._chop && std::abs(im) < rhs._chop)
					{
						ostr << "0 "; // otherwise segfault on destruction
									  // if using only vstr.push_back("0 ");
									  // bug in MATLAB's libmx
						vstr.push_back(ostr.str());
					}
					else if (std::abs(re) < rhs._chop)
					{
						ostr << im;
						vstr.push_back(ostr.str() + "i");
					}
					else if (std::abs(im) < rhs._chop)
					{
						ostr << re;
						vstr.push_back(ostr.str() + " ");
					}
					else
					{
						ostr << re;
						strA = ostr.str();

						strA += (im > 0 ? " + " : " - ");
						ostr.clear();
						ostr.str(std::string()); // clear
						ostr << std::abs(im);
						strA += ostr.str();
						strA += "i";
						vstr.push_back(strA);
					}
				}
			}

			// determine the maximum lenght of the entries in each column
			std::vector < std::size_t > maxlengthcols(rhs._A.cols(), 0);

			for (std::size_t i = 0; i < static_cast<std::size_t>(rhs._A.rows());
					i++)
				for (std::size_t j = 0;
						j < static_cast<std::size_t>(rhs._A.cols()); j++)
					if (vstr[i * rhs._A.cols() + j].size() > maxlengthcols[j])
						maxlengthcols[j] = vstr[i * rhs._A.cols() + j].size();

			// finally display it!
			for (std::size_t i = 0; i < static_cast<std::size_t>(rhs._A.rows());
					i++)
			{
				os << std::setw(static_cast<int>(maxlengthcols[0]))
						<< std::right << vstr[i * rhs._A.cols()]; // display first column
				// then the rest
				for (std::size_t j = 1;
						j < static_cast<std::size_t>(rhs._A.cols()); j++)
					os << std::setw(static_cast<int>(maxlengthcols[j] + 2))
							<< std::right << vstr[i * rhs._A.cols() + j];

				if (i < static_cast<std::size_t>(rhs._A.rows()) - 1)
					os << std::endl;
			}
			return os;
		} /* case */
		case IOManip::Type::RANGE:
		{
			os << rhs._start;

			auto it = rhs._first;
			auto it_end = rhs._last;

			if (it != it_end)
			{
				// the iterator just before the end, need this for containers
				// that do not have backwards iterators
				decltype(it_end) it_before_end = it;
				while (it_before_end = it, ++it != it_end)
					;

				it = rhs._first;
				for (; it != it_before_end; ++it)
					os << *it << rhs._separator;
				os << *it;
			}
			os << rhs._end;
			return os;
		} /* case */
		case IOManip::Type::C_POINTER:
		{
			os << rhs._start;

			for (std::size_t i = 0; i < rhs._n - 1; i++)
				os << rhs._p[i] << rhs._separator;
			if (rhs._n > 0)
				os << rhs._p[rhs._n - 1];

			os << rhs._end;
			return os;
		} /* case */
		} /* switch */
		return os;
	} /* operator<< */
};
/* class IOManip */

} /* namespace qpp */

#endif /* INCLUDE_INTERNAL_CLASSES_IOMANIP_H_ */
