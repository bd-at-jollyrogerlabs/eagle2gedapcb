#define CATCH_CONFIG_MAIN
#include <Catch/catch.hpp>

// Standard C library includes
#include <cstdint>

// STL includes
#include <iostream>
#include <list>

// Boost includes
#include <boost/units/io.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/base_units/us/mil.hpp>

// Local includes
#include "boost_unit_extras.hpp"
#include "gedapcb.hpp"

TEST_CASE("tests of objects representing gEDA pcb elements", "[gedapcb]") {
  jrl::geda_pcb::PCB pcb("Test", 10000, 10000);

  SECTION("printing pcb object") {
    std::cout << pcb;
  }
}
