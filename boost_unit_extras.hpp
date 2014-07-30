// Extra unit definitions
// Copyright 2014 by Brian Davis.

#ifndef boost_unit_extras_HEADER
#define boost_unit_extras_HEADER

namespace jrl
{

namespace bu = boost::units;
namespace busi = boost::units::si;
namespace buus = boost::units::us;

// Helper system + class to simplify use of millimeters.
namespace mm_system
{
  typedef bu::scaled_base_unit<busi::meter_base_unit,
                               bu::scale<10, bu::static_rational<-3> > > mm_base_unit;
  typedef bu::make_system<mm_base_unit>::type system;
  typedef bu::unit<bu::length_dimension, system> length;

  BOOST_UNITS_STATIC_CONSTANT(mm, length);
}

class Millimeters : public bu::quantity<mm_system::length>
{
public:
  Millimeters(const double mm)
    : quantity<mm_system::length>(mm * mm_system::mm)
  {
  }

  double
  value() const
  {
    return quantity<mm_system::length>::value();
  }
};


namespace mil_systemp
{
  typedef bu::make_system<buus::mil_base_unit>::type system;
  typedef bu::unit<bu::length_dimension, system> length;

  BOOST_UNITS_STATIC_CONSTANT(mil, length);
  BOOST_UNITS_STATIC_CONSTANT(mils, length);
}

}

// New system for centimils (the basic unit of length measure in gEDA
// pcb.
//
// 1 centimil = 1 mil / 100
namespace boost
{
namespace units
{
namespace us
{
  typedef scaled_base_unit<mil_base_unit,
                           scale<10, static_rational<-2> > > centimil_base_unit;
}
template<>
struct base_unit_info<us::centimil_base_unit>
{
  static const char *name() { return "centimil"; }
  static const char *symbol() { return "cmil"; }
};
typedef make_system<us::centimil_base_unit>::type system;
typedef unit<length_dimension, system> length;

BOOST_UNITS_STATIC_CONSTANT(centimil, length);
BOOST_UNITS_STATIC_CONSTANT(centimils, length);
}
}

namespace jrl
{

// Helper system + class to simplify the use of centimils.
namespace centimil_system
{
  typedef bu::make_system<buus::centimil_base_unit>::type system;
  typedef bu::unit<bu::length_dimension, system> length;

  BOOST_UNITS_STATIC_CONSTANT(centimil, length);
  BOOST_UNITS_STATIC_CONSTANT(centimils, length);
}

class Centimils : public bu::quantity<centimil_system::length>
{
public:
  Centimils(const int centimils)
    : quantity<centimil_system::length>(centimils * centimil_system::centimil)
  {
  }

  Centimils(const Millimeters mm)
    : bu::quantity<centimil_system::length>(static_cast<bu::quantity<centimil_system::length> >(mm))
  {
  }
};

};

#endif
