// Class definitions associated with gEDA pcb board file format.
// Copyright 2014 by Brian Davis

namespace jrl
{
namespace geda_pcb
{

class Printable
{
public:

  // Member functions

  virtual void
  print(std::ostream &strm) const;
};

class PCB : public Printable
{
public:

  // Constructors/destructors

  PCB(const std::string &name,
      const Centimils &width,
      const Centimils &height)
    : name_(name), width_(width), height_(height)
  {
  }

  ~PCB()
  {
  }

  // Member functions

  virtual void
  print(std::ostream &strm) const;

private:

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const std::string &name_;  // Name of the PCB project
  const Centimils &width_;  // Size of the board
  const Centimils &height_;
};

std::ostream &
operator<<(std::ostream &strm, const PCB &pcb)
{
  pcb.print(strm);
  return strm;
}

class Layer
{
public:

  // Constructors/destructors

  Layer(const std::uint8_t number,
	const std::string &name)
    : number_(number), name_(name)
  {
  }

  ~Layer();

  // Member functions

  // TODO: error checking, probably need a class interposed between
  // Printable and Layer to cover the class that are legal to add to a
  // layer.
  void
  addElement(Printable *printable);

  void
  print(std::ostream &strm) const;

private:

  // Types

  typedef std::list<Printable *> Printables;
  typedef std::list<Printable *>::const_iterator Iter;

  // Data members

  const std::uint8_t number_;
  const std::string name_;
  Printables printables_;
};

std::ostream &
operator<<(std::ostream &strm, const Layer &layer);

class HasLineValues
{
protected:

  // Constructors/destructors
  
  HasLineValues(const Centimils &thickness,
		const Centimils &clearance)
    : thickness_(thickness), clearance_(clearance)
  {
  }

  // Member functions

  void
  printLinePortion(std::ostream &stream) const;

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const Centimils thickness_;  // Outer diameter of copper annulus.
  const Centimils clearance_;  // Add to thickness to get clearance
			       // diameter.
};

class HasEndpoints
{
protected:

  // Constructors/destructors

  HasEndpoints(const Centimils &rX1,
	       const Centimils &rY1,
	       const Centimils &rX2,
	       const Centimils &rY2)
  : rX1_(rX1), rY1_(rY1), rX2_(rX2), rY2_(rY2)
  {
  }

  ~HasEndpoints()
  {
  }

  // Member functions

  void
  printEndPoints(std::ostream &strm) const;

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const Centimils rX1_;  // Coordinates of the endpoints of the pad,
			 // relative to the element's mark.
  const Centimils rY1_;
  const Centimils rX2_;
  const Centimils rY2_;
};

class Line : public Printable, private HasEndpoints, private HasLineValues
{
public:

  // Constructors/destructors

  Line(const Centimils &rX1,
       const Centimils &rY1,
       const Centimils &rX2,
       const Centimils &rY2,
       const Centimils &thickness,
       const Centimils &clearance,
       const std::string &flags)
    : HasEndpoints(rX1, rY1, rX2, rY2),
      HasLineValues(thickness, clearance),
      flags_(flags)
  {
  }

  ~Line()
  {
  }

  // Member functions

  virtual void
  print(std::ostream &strm) const;

private:

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const std::string flags_;  // Symbolic or numerical flags.
};

class PadOrPin : protected HasLineValues
{
protected:

  // Constructors/destructors
  
  PadOrPin(const Centimils &thickness,
	   const Centimils &clearance,
	   const Centimils &mask,const std::string &name,
	   const std::uint16_t number,
	   const std::string &flags)
    : HasLineValues(thickness, clearance), mask_(mask), name_(name),
      number_(number), flags_(flags)
  {
  }

  ~PadOrPin()
  {
  }

  // Member functions

  void
  print1(std::ostream &strm) const;

  void
  print2(std::ostream &strm) const;

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const Centimils mask_;  // Diameter of solder mask opening.
  const std::string name_;  // Name of pin.
  const std::uint16_t number_;  // Number of pin.
  const std::string flags_;  // Symbolic or numerical flags.
};

class Pad : public Printable, private PadOrPin
{
public:

  // Constructors/destructors

  Pad(const Centimils &rX1,
      const Centimils &rY1,
      const Centimils &rX2,
      const Centimils &rY2,
      const Centimils &thickness,
      const Centimils &clearance,
      const Centimils &mask,
      const std::string &name,
      const std::uint16_t number,
      const std::string &flags)
    : PadOrPin(thickness, clearance, mask, name, number, flags),
      rX1_(rX1), rY1_(rY1), rX2_(rX2), rY2_(rY2)
  {
  }

  // Member functions

  virtual void
  print(std::ostream &strm) const;

private:

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const Centimils rX1_;  // Coordinates of the endpoints of the pad,
			 // relative to the element's mark.
  const Centimils rY1_;
  const Centimils rX2_;
  const Centimils rY2_;
};

std::ostream&
operator<<(std::ostream &strm, const Pad &pad);

class Pin : public Printable, private PadOrPin
{
public:

  // Constructors/destructors
  
  Pin(const Centimils &rX,
      const Centimils &rY,
      const Centimils &thickness,
      const Centimils &clearance,
      const Centimils &mask,
      const Centimils &drill,
      const std::string &name,
      const std::uint16_t number,
      const std::string &flags)
    : PadOrPin(thickness, clearance, mask, name, number, flags),
      rX_(rX), rY_(rY), drill_(drill)
  {
  }

  ~Pin()
  {
  }

  virtual void
  print(std::ostream &strm) const;

private:

  // Data members

  // NOTE: documentary comments are taken from gEDA pcb manual.
  const Centimils rX_;  // Coordinates of center, relative to the
			// element's mark.
  const Centimils rY_;
  const Centimils drill_;  // Diameter of drill.
};

std::ostream&
operator<<(std::ostream &strm, const Pin &pin);

}
}
