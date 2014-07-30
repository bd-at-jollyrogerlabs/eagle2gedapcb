// Implementations associated with gEDA pcb board file format.
// Copyright 2014 by Brian Davis

#include <iostream>
#include "boost_unit_extras.hpp"
#include "gedapcb.hpp"

using namespace std;
using namespace jrl;
using namespace jrl::geda_pcb;

void
PCB::print(ostream &strm)
{
  strm << "PCB[\"" << name_ << "\" "
       << width_ << " " << height_ << "]" << endl;
}

Layer::~Layer()
{
  for (Iter iter = printables_.begin(); iter != printables_.end(); ++iter) {
    delete *iter;
  }
}

void
Layer::addElement(Printable *printable)
{
  printables_.push_back(printable);
}

void
Layer::print(ostream &strm) const;
{
  strm << "Layer (" << number_ <<  " \"" << name_ << "\")" << endl
       << "(" << endl;
  for (Iter iter = printables_.begin(); iter != printables_.end(); ++iter) {
    strm << "\t" << *iter << endl;
  }
  strm << ")" << endl;
}

ostream &
operator<<(ostream &strm, const Layer &layer)
{
  layer.print(strm);
  return strm;
}

void
HasLineValues::printLinePortion(ostream &strm)
{
  strm << " \"" << thickness_ << "\""
       << " \"" << clearance_ << "\"";
}

void
HasEndpoints::printEndPoints(ostream &strm)
{
  strm << " " << rX1_
       << " " << rY1_
       << " " << rX2_
       << " " << rY2_;
}

void
Line::print(ostream &strm) const
{
  strm << "Line[";
  HasEndpoints::printEndPoints(strm);
  HasLineValues::printLinePortion(strm);
  strm << " " << flags_
       << "]";
}

void
PadOrPin::print1(ostream &strm) const
{
  HasLineValues::printLinePortion(strm);
  strm << " " << mask_;
}

void
PadOrPin::print2(ostream &strm) const
{
  strm << " \"" << name_ << "\""
       << " \"" << number_ << "\""
       << " " << flags_;
}

void
Pad::print(ostream &strm) const
{
  strm << "Pad["
       << rX1_
       << " " << rY1_;
       << " " << rX2_;
       << " " << rY2_;
  PadOrPin::print1(strm);
  PadOrPin::print2(strm);
  strm << "]";
}

void
Pin::print(ostream &strm) const
{
  strm << "Pin["
       << rX_
       << " " << rY_;
  PadOrPin::print1(strm);
  strm << " " << drill_;
  PadOrPin::print2(strm);
  strm << "]";
}

ostream &
operator<<(ostream &strm, const Line &line)
{
  line.print(strm);
  return strm;
}

ostream &
operator<<(ostream &strm, const Pad &pad)
{
  pad.print(strm);
  return strm;
}

ostream &
operator<<(ostream &strm, const Pin &pin)
{
  pin.print(strm);
  return strm;
}
